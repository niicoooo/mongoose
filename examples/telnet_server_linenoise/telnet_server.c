#define _GNU_SOURCE
#include <unistd.h>
#include <sys/time.h>
#include "pt.h"
#include "mongoose.h"
#include <stdio.h>
#include "linenoise.h"


int running = 1;


#define PRINTLINE    printf("Debug: %s:%i\n", __func__, __LINE__);

#define LINENOISE_BUFSIZE 100
#define TELNETRX_BUFSIZE 1000

struct buffer_t {
    char rx_buffer[TELNETRX_BUFSIZE];
    int rx_len;
    struct pt pt;
    FILE* file;
    telnet_t *telnet_handle;
    struct mg_connection *nc;

    char buf[LINENOISE_BUFSIZE];
    struct linenoiseState* l;
    char c;
};



static int protothread(struct pt *pt, struct buffer_t* buffer) {
    PT_BEGIN(pt);
    while(1) {
        buffer->l = linenoiseEdit(buffer->file, buffer->buf, LINENOISE_BUFSIZE, "hello> ");
        buffer->c = 0;
        while (linenoiseHandleInput(buffer->l, buffer->c) == 0) {
            while (buffer->rx_len == 0) {
                PT_YIELD(pt);
            }
            buffer->c = fgetc(buffer->file);
        }
        free(buffer->l);
        buffer->l = 0;
        fprintf(buffer->file, "\necho: '%s'\n", buffer->buf);
        linenoiseHistoryAdd(buffer->buf);
        if (strcmp(buffer->buf, "exit") == 0) {
            buffer->nc->flags |= MG_F_SEND_AND_CLOSE;
        }
        if (strcmp(buffer->buf, "stop") == 0) {
            running = 0;
            buffer->nc->flags |= MG_F_SEND_AND_CLOSE;
        }

    }
    PT_END(pt);
    return 0;
}



ssize_t telnetfile_write(void *cookie, const char *buf, size_t size) {
    struct buffer_t* buffer = (struct buffer_t*)cookie;
    telnet_send_text(buffer->telnet_handle, buf, size);
    return size;
}

ssize_t telnetfile_read(void *cookie, char *buf, size_t size) {
    struct buffer_t* buffer = (struct buffer_t*)cookie;
    size = (size > buffer->rx_len ? buffer->rx_len : size);
    memcpy(buf, &buffer->rx_buffer[0], size);
    memmove(&buffer->rx_buffer[0], &buffer->rx_buffer[size], 1000 - size);
    buffer->rx_len -= size;
    return size;
}


void mg_telnet_dump_event(telnet_t *telnet, telnet_event_t *ev);


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
        buffer->nc = nc;
        buffer->l = 0;
        PT_INIT(&buffer->pt);
        cookie_io_functions_t  telnetfilefile_func = {
            .read  = telnetfile_read,
            .write = telnetfile_write,
            .seek  = NULL,
            .close = NULL
        };
        buffer->file = fopencookie(buffer,"w+", telnetfilefile_func);
        setbuf(buffer->file, NULL);
        buffer->telnet_handle = telnet_handle;
//        telnet_negotiate(telnet_handle, TELNET_DONT, TELNET_TELOPT_LINEMODE);
//        telnet_negotiate(telnet_handle, TELNET_WONT, TELNET_TELOPT_ECHO);
        telnet_negotiate(telnet_handle, TELNET_DO, TELNET_TELOPT_LINEMODE);
        static const char sb_buffer[] = { 34, 1, 0 };
        telnet_subnegotiation(telnet_handle, TELNET_TELOPT_LINEMODE, sb_buffer, 3); /* IAC SB LINEMODE MODE 0 IAC SE */
        telnet_negotiate(telnet_handle, TELNET_WILL, TELNET_TELOPT_ECHO);
        fprintf(buffer->file, "Hello!\n");
        protothread(&buffer->pt, buffer);
        break;
    case MG_EV_TELNET_DATA:
        buffer = (struct buffer_t*)nc->user_data;
        assert(buffer);
        memcpy(&buffer->rx_buffer[buffer->rx_len], mg_telnet_get_recv_data(nc), mg_telnet_get_recv_size(nc));
        buffer->rx_len += mg_telnet_get_recv_size(nc);
        protothread(&buffer->pt, buffer);
        break;
    case MG_EV_TELNET_CLOSE:
        buffer = (struct buffer_t*)nc->user_data;
        fclose(buffer->file);
        if(buffer->l) {
            free(buffer->l);
        }
        assert(buffer);
        free(buffer);
        break;
    case MG_EV_TELNET_POLL:
        buffer = (struct buffer_t*)nc->user_data;
        assert(buffer);
        protothread(&buffer->pt, buffer);
        break;
    case MG_EV_TELNET_OTHER:
        assert(mg_telnet_get_event(nc));
        mg_telnet_dump_event(telnet_handle, mg_telnet_get_event(nc));
        break;
    default:
        printf ("Telnet server %p: other event %i\n", nc, ev);
        assert(0);
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
    { TELNET_TELOPT_LINEMODE,  TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_ECHO,      TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DONT },
    { -1, 0, 0 }
};


int main(void) {
    struct mg_connection *nc;
    static struct mg_mgr mgr;
    printf("Starting telnet-server on port %s\n", MG_LISTEN_ADDR);
    mg_mgr_init(&mgr, NULL);

    struct mg_bind_opts opts;
    memset(&opts, 0, sizeof(opts));
    const char* error;
    opts.error_string = &error;
    nc = mg_bind_opt(&mgr, MG_LISTEN_ADDR, mg_ev_handler, opts);
    if (nc == NULL) {
        printf("Starting telnet-server error: %s\n", error);
        exit(-1);
    }

    printf("Starting telnet-server on port %s\n", MG_LISTEN_ADDR);
    printf("Telnet server %p\n", nc);

    mg_set_protocol_telnet(nc);
    mg_telnet_set_default_telopt(nc, my_telopts);
    assert(nc->proto_handler);
    assert(nc->proto_data);
    while (running) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    linenoiseFreeHistory();

    return 0;
}
