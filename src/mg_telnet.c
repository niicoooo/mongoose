#ifdef MG_ENABLE_TELNET

#include "mg_internal.h"
#include "mg_util.h"
#include <assert.h>
#include <libtelnet.h>



struct mg_telnet_proto_data {
    const telnet_telopt_t *default_telnet_telopt;
    telnet_t *telnet_handle;
    void *user_data;
    int (*user_handler)(telnet_t *telnet_handle, enum mg_telnet_event_t ev, const void *data, int len, void **user_data);
};


static char *libtelnetEventToString(telnet_event_type_t type) {
    switch(type) {
    case TELNET_EV_COMPRESS:
        return "TELNET_EV_COMPRESS";
    case TELNET_EV_DATA:
        return "TELNET_EV_DATA";
    case TELNET_EV_DO:
        return "TELNET_EV_DO";
    case TELNET_EV_DONT:
        return "TELNET_EV_DONT";
    case TELNET_EV_ENVIRON:
        return "TELNET_EV_ENVIRON";
    case TELNET_EV_ERROR:
        return "TELNET_EV_ERROR";
    case TELNET_EV_IAC:
        return "TELNET_EV_IAC";
    case TELNET_EV_MSSP:
        return "TELNET_EV_MSSP";
    case TELNET_EV_SEND:
        return "TELNET_EV_SEND";
    case TELNET_EV_SUBNEGOTIATION:
        return "TELNET_EV_SUBNEGOTIATION";
    case TELNET_EV_TTYPE:
        return "TELNET_EV_TTYPE";
    case TELNET_EV_WARNING:
        return "TELNET_EV_WARNING";
    case TELNET_EV_WILL:
        return "TELNET_EV_WILL";
    case TELNET_EV_WONT:
        return "TELNET_EV_WONT";
    case TELNET_EV_ZMP:
        return "TELNET_EV_ZMP";
    default:
        return "Unknown type";
    }
    return "Unknown type";
}










static void mg_telnet_data_destructor(void *proto_data) {
    MG_FREE(proto_data);
}

static void mg_telnet_handler(struct mg_connection *nc, int ev, void *ev_data);

static struct mg_telnet_proto_data *mg_telnet_get_proto_data(struct mg_connection *nc) {
    assert(nc);
    assert(nc->proto_handler == mg_telnet_handler);
    if (nc->proto_data == NULL) {
        nc->proto_data = MG_CALLOC(1, sizeof(struct mg_telnet_proto_data));
        nc->proto_data_destructor = mg_telnet_data_destructor;
    }
    return (struct mg_telnet_proto_data *) nc->proto_data;
}

void mg_telnet_set_default_telopt(struct mg_connection *nc, const telnet_telopt_t *default_telnet_telopt) {
    assert(nc);
    assert(nc->proto_handler == mg_telnet_handler);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    proto_data->default_telnet_telopt = default_telnet_telopt;
}

void mg_telnet_set_telnet_handler(struct mg_connection *nc, int (*user_handler)(telnet_t *telnet_handle, enum mg_telnet_event_t ev, const void *data, int len, void **user_data)) {
    assert(nc);
    assert(nc->proto_handler == mg_telnet_handler);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    proto_data->user_handler = user_handler;
}



static void libtelnet_handler(telnet_t *telnet_handle, telnet_event_t *event, void *userData) {
    assert(telnet_handle);
    assert(userData);
    struct mg_connection *nc = (struct mg_connection*)userData;
    LOG(LL_DEBUG, ("Telnet server %p: telnet event %s", nc, libtelnetEventToString(event->type)));
    switch(event->type) {
    case TELNET_EV_SEND:
        mg_send(nc, event->data.buffer, event->data.size);
        break;
    case TELNET_EV_DATA:
        LOG(LL_DEBUG, ("Telnet server %p: received data, len=%lu", nc, event->data.size));
        assert(nc->listener);
        struct mg_telnet_proto_data *proto_data_listener = mg_telnet_get_proto_data(nc->listener);
        if(proto_data_listener->user_handler != NULL) {
            if (proto_data_listener->user_handler(telnet_handle, MG_EV_TELNET_DATA, event->data.buffer, event->data.size, &nc->user_data) < 0) {
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
        }
        break;
    default:
        break;
    }
}


static void mg_telnet_handler(struct mg_connection *nc, int ev, void *ev_data MG_UD_ARG(void *user_data)) {
    if(nc->listener != NULL) {
        struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
        struct mg_telnet_proto_data *proto_data_listener = mg_telnet_get_proto_data(nc->listener);
        switch (ev) {
        case MG_EV_ACCEPT: { /* New connection accepted. union socket_address * */
            char addr[32];
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            LOG(LL_INFO, ("Telnet server %p: connection from %s", nc, addr));
            telnet_t *telnet_handle = telnet_init(proto_data_listener->default_telnet_telopt, libtelnet_handler, 0, nc);
            if (telnet_handle == NULL) {
                LOG(LL_INFO, ("Telnet server %p: telnet_init failed", nc));
                const char* msg = "Server internal error. Connection closed";
                mg_send(nc, msg, strlen(msg));
                nc->flags |= MG_F_SEND_AND_CLOSE;
                break;
            }
            proto_data->telnet_handle = telnet_handle;
            if(proto_data_listener->user_handler != NULL) {
                if (proto_data_listener->user_handler(telnet_handle, MG_EV_TELNET_ACCEPT, NULL, 0, &nc->user_data) < 0) {
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                }
            }
            telnet_negotiate(telnet_handle, TELNET_WONT, TELNET_TELOPT_ECHO);
            telnet_negotiate(telnet_handle, TELNET_DONT, TELNET_TELOPT_LINEMODE);
        }
        break;
        case MG_EV_RECV: { /* Data has been received. int *num_bytes */
            struct mbuf *io = &nc->recv_mbuf;
            assert(proto_data->telnet_handle);
            telnet_recv(proto_data->telnet_handle, (char *)io->buf, io->len);
            mbuf_remove(io, io->len);
        }
        break;
        case MG_EV_CLOSE: { /* Connection is closed. NULL */
            if(nc->listener) {
                telnet_t *telnet_handle = proto_data->telnet_handle;
                LOG(LL_INFO, ("Telnet server %p: connection closed", nc));
                assert(proto_data->telnet_handle);
                if(proto_data_listener->user_handler != NULL) {
                    proto_data_listener->user_handler(telnet_handle, MG_EV_TELNET_CLOSE, NULL, 0, &nc->user_data);
                }
                telnet_free(telnet_handle);
            }
        }
        break;
        case MG_EV_POLL: { /* Sent to each connection on each mg_mgr_poll() call */
            telnet_t *telnet_handle = proto_data->telnet_handle;
            assert(proto_data->telnet_handle);
            if(proto_data_listener->user_handler != NULL) {
                if (proto_data_listener->user_handler(telnet_handle, MG_EV_TELNET_POLL, NULL, 0, &nc->user_data) < 0) {
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                }
            }
        }
        break;
        case MG_EV_SEND: /* Data has been written to a socket. int *num_bytes */
            break;
        default:
            LOG(LL_INFO, ("Telnet server %p: other event %i", nc, ev));
            if (nc->handler != NULL) {
                nc->handler(nc, ev, ev_data);
            }
            break;
        }
    } else {
        LOG(LL_INFO, ("Telnet server %p: other event %i (no connection)", nc, ev));
        if (nc->handler != NULL) {
            nc->handler(nc, ev, ev_data);
        }
    }
}


void mg_set_protocol_telnet(struct mg_connection *nc) {
    nc->proto_handler = mg_telnet_handler;
}


#endif
