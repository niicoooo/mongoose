#include "mongoose.h"



int running = 1;

static int mg_telnet_custom_handler(telnet_t *telnet_handle, enum mg_telnet_event_t ev, const void *data, int len, void **user_data) {
    assert(telnet_handle);
    switch(ev) {
    case MG_EV_TELNET_ACCEPT:
        *user_data = malloc(1024);
        break;
    case MG_EV_TELNET_DATA:
        assert(*user_data);
        uint8_t buffer_len = (*((uint8_t**)user_data))[0];
        char* buffer = &(*((char**)user_data))[1];
        if (len == 0) {
            return 0;
        }
        if (((char*)data)[0] == '\n') {
            if (memcmp(buffer, "quit", 4) == 0) {
                return -1;
            } else if (memcmp(buffer, "stop", 4) == 0) {
                running = 0;
                return -1;
            } else if (memcmp(buffer, "help", 4) == 0) {
                telnet_printf(telnet_handle, "quit:\tdisconnect\nstop:\tstop server\n");
                (*((uint8_t**)user_data))[0] = 0;
                return mg_telnet_custom_handler(telnet_handle, ev, data + 1, len - 1, user_data);
            } else {
                telnet_printf(telnet_handle, "Unknown cmd. help to dispay available commands\n");
                (*((uint8_t**)user_data))[0] = 0;
                return mg_telnet_custom_handler(telnet_handle, ev, data + 1, len - 1, user_data);
            }
        } else {
            if (((char*)data)[0] != '\r') {
                (*((uint8_t**)user_data))[0] = buffer_len + 1;
                buffer[buffer_len] = ((char*)data)[0];
            }
            return mg_telnet_custom_handler(telnet_handle, ev, data + 1, len - 1, user_data);
        }
        break;
    case MG_EV_TELNET_CLOSE:
        assert(*user_data);
        free(*user_data);
        break;
    case MG_EV_TELNET_POLL:
        break;
    default:
        break;
    }
    return 0;
}


static const char* MG_LISTEN_ADDR = "2323";

static void mg_ev_handler(struct mg_connection *nc, int ev, void *p) {
	(void)nc;
	(void)ev;
	(void)p;
}

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
    mg_telnet_set_telnet_handler(nc, mg_telnet_custom_handler);
    assert(nc->proto_handler);
    assert(nc->proto_data);
    while (running) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    return 0;
}
