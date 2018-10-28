#include "mongoose.h"



int running = 1;


struct buffer_t {
    char buffer[1000];
    int len;
};


static void mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    if(nc->listener == NULL) {
        return;
    }

    telnet_t *telnet_handle = mg_telnet_get_telnet_handle(nc);
    assert(telnet_handle);
    struct buffer_t* buffer;

    switch(ev) {
    case MG_EV_TELNET_ACCEPT:
        buffer = (struct buffer_t*)malloc(sizeof(struct buffer_t));
        memset(buffer, 0, sizeof(struct buffer_t));
        nc->user_data = buffer;
        break;
    case MG_EV_TELNET_DATA:

        buffer = (struct buffer_t*)nc->user_data;
        assert(buffer);

        const void *data = mg_telnet_get_recv_data(nc);

        for (int i = 0; i < mg_telnet_get_recv_size(nc); i++) {
            if (((char*)data)[i] == '\n') {
                buffer->buffer[buffer->len] = 0;
                if (strcmp(buffer->buffer, "quit") == 0) {
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                    return;
                } else if (strcmp(buffer->buffer, "stop") == 0) {
                    running = 0;
                    nc->flags |= MG_F_SEND_AND_CLOSE;
                    return;
                } else if (strcmp(buffer->buffer, "help") == 0) {
                    telnet_printf(telnet_handle, "quit:\tdisconnect\nstop:\tstop server\n");
                    buffer->len =0;
                } else {
                    telnet_printf(telnet_handle, "Unknown cmd. help to dispay available commands\n");
                    buffer->len = 0;
                }
            } else {
                if (((char*)data)[i] != '\r') {
                    buffer->buffer[buffer->len] = ((char*)data)[i];
                    buffer->len++;
                }
            }
        }
        break;
    case MG_EV_TELNET_CLOSE:
        buffer = (struct buffer_t*)nc->user_data;
        assert(buffer);
        free(buffer);
        break;
    case MG_EV_TELNET_POLL:
        break;
    default:
        printf ("Telnet server %p: other event %i", nc, ev);
        break;
    }
}


static const char* MG_LISTEN_ADDR = "2323";


static const telnet_telopt_t my_telopts[] = {
    { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
    { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
    { -1, 0, 0 }
};


int main(void) {
    struct mg_connection *nc;
    static struct mg_mgr mgr;
    printf("Starting telnet-server on port %s\n", MG_LISTEN_ADDR);
    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, MG_LISTEN_ADDR, mg_ev_handler);
    printf("Starting telnet-server on port %s\n", MG_LISTEN_ADDR);
    printf("Telnet server %p\n", nc);
    assert(nc);
    mg_set_protocol_telnet(nc);
    mg_telnet_set_default_telopt(nc, my_telopts);
    assert(nc->proto_handler);
    assert(nc->proto_data);
    while (running) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    return 0;
}
