#ifdef MG_ENABLE_TELNET

#include "mg_internal.h"
#include "mg_util.h"
#include <assert.h>
#include <libtelnet.h>



#define PRINTLINE    printf("Debug: %s:%i\n", __func__, __LINE__);



struct mg_telnet_proto_data {
    const telnet_telopt_t *default_telnet_telopt;
    telnet_t *telnet_handle;

    const char* recvdata_buffer;
    int recvdata_size;
    telnet_event_t *event;
    int event_id;
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

telnet_t* mg_telnet_get_telnet_handle(struct mg_connection *nc) {
    assert(nc);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    return proto_data->telnet_handle;
}

const char* mg_telnet_get_recv_data(struct mg_connection *nc) {
    assert(nc);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    assert(proto_data->event);
    return proto_data->event->data.buffer;
}

int mg_telnet_get_recv_size(struct mg_connection *nc) {
    assert(nc);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    assert(proto_data->event);
    return proto_data->event->data.size;
}

telnet_event_t* mg_telnet_get_event(struct mg_connection *nc) {
    assert(nc);
    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    assert(proto_data->event);
    return proto_data->event;
}


static void libtelnet_handler(telnet_t *telnet_handle, telnet_event_t *event, void *userData) {
    assert(telnet_handle);
    assert(userData);
    struct mg_connection *nc = (struct mg_connection*)userData;

    struct mg_telnet_proto_data *proto_data = mg_telnet_get_proto_data(nc);
    assert(proto_data);

    switch(event->type) {
    case TELNET_EV_SEND:
        LOG(LL_DEBUG, ("Telnet server %p (libtelnet): TELNET_EV_SEND sending data, len=%lu", nc, event->data.size));
        assert(proto_data->event_id);
        mg_send(nc, event->data.buffer, event->data.size);
        break;
    case TELNET_EV_DATA:
        LOG(LL_DEBUG, ("Telnet server %p (libtelnet): TELNET_EV_DATA received data, len=%lu", nc, event->data.size));
        assert(nc->listener);
        assert(proto_data->telnet_handle);
        proto_data->event = event;
        proto_data->event_id = MG_EV_TELNET_DATA;
        mg_call(nc, nc->handler, nc->user_data, MG_EV_TELNET_DATA, NULL);
        proto_data->event = 0;
        proto_data->event_id = 0;
        break;
    default:
        LOG(LL_DEBUG, ("Telnet server %p (libtelnet): other event %s", nc, libtelnetEventToString(event->type)));
        proto_data->event = event;
        proto_data->event_id = MG_EV_TELNET_OTHER;
        mg_call(nc, nc->handler, nc->user_data, MG_EV_TELNET_OTHER, NULL);
        proto_data->event = 0;
        proto_data->event_id = 0;
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
            LOG(LL_DEBUG, ("Telnet server %p (mg event): MG_EV_ACCEPT connection from %s", nc, addr));
            telnet_t *telnet_handle = telnet_init(proto_data_listener->default_telnet_telopt, libtelnet_handler, 0, nc);
            if (telnet_handle == NULL) {
                LOG(LL_ERROR, ("Telnet server %p (mg event): telnet_init failed", nc));
                const char* msg = "Server internal error. Connection closed";
                mg_send(nc, msg, strlen(msg));
                nc->flags |= MG_F_SEND_AND_CLOSE;
                break;
            }
            proto_data->telnet_handle = telnet_handle;
            proto_data->event_id = MG_EV_TELNET_ACCEPT;
            mg_call(nc, nc->handler, nc->user_data, MG_EV_TELNET_ACCEPT, NULL);
            proto_data->event_id = 0;

//            telnet_negotiate(telnet_handle, TELNET_WONT, TELNET_TELOPT_ECHO);
//            telnet_negotiate(telnet_handle, TELNET_DONT, TELNET_TELOPT_LINEMODE);
        }
        break;
        case MG_EV_RECV: { /* Data has been received. int *num_bytes */
            LOG(LL_DEBUG, ("Telnet server %p (mg event): MG_EV_RECV", nc));
            struct mbuf *io = &nc->recv_mbuf;
            assert(proto_data->telnet_handle);
            proto_data->event_id = MG_EV_RECV;
            telnet_recv(proto_data->telnet_handle, (char *)io->buf, io->len);
            proto_data->event_id = 0;
            mbuf_remove(io, io->len);
        }
        break;
        case MG_EV_CLOSE: { /* Connection is closed. NULL */
            telnet_t *telnet_handle = proto_data->telnet_handle;
            LOG(LL_DEBUG, ("Telnet server %p (mg event): connection closed", nc));
            assert(proto_data->telnet_handle);
            proto_data->event_id = MG_EV_TELNET_CLOSE;
            mg_call(nc, nc->handler, nc->user_data, MG_EV_TELNET_CLOSE, NULL);
            proto_data->event_id = 0;
            telnet_free(telnet_handle);
        }
        break;
        case MG_EV_POLL: { /* Sent to each connection on each mg_mgr_poll() call */
            LOG(LL_DEBUG, ("Telnet server %p (mg event): MG_EV_POLL", nc));
            proto_data->event_id = MG_EV_TELNET_POLL;
            mg_call(nc, nc->handler, nc->user_data, MG_EV_TELNET_POLL, NULL);
            proto_data->event_id = 0;
        }
        break;
        case MG_EV_SEND: /* Data has been written to a socket. int *num_bytes */
            break;
        default:
            LOG(LL_DEBUG, ("Telnet server %p (mg event): other event %i", nc, ev));
            mg_call(nc, nc->handler, nc->user_data, ev, ev_data);
            break;
        }
    } else {
        LOG(LL_DEBUG, ("Telnet server %p (mg event): other mg event %i (no connection)", nc, ev));
        mg_call(nc, nc->handler, nc->user_data, ev, ev_data);
    }
}


void mg_set_protocol_telnet(struct mg_connection *nc) {
    nc->proto_handler = mg_telnet_handler;
}


#endif
