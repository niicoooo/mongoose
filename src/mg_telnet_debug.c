#ifdef MG_ENABLE_TELNET

#include "mg_internal.h"
#include "mg_util.h"
#include <assert.h>
#include <libtelnet.h>

//#include <errno.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <ctype.h>

#include "libtelnet.h"


static const char *get_cmd(unsigned char cmd) {
    static char buffer[4];

    switch (cmd) {
    case 255:
        return "IAC";
    case 254:
        return "DONT";
    case 253:
        return "DO";
    case 252:
        return "WONT";
    case 251:
        return "WILL";
    case 250:
        return "SB";
    case 249:
        return "GA";
    case 248:
        return "EL";
    case 247:
        return "EC";
    case 246:
        return "AYT";
    case 245:
        return "AO";
    case 244:
        return "IP";
    case 243:
        return "BREAK";
    case 242:
        return "DM";
    case 241:
        return "NOP";
    case 240:
        return "SE";
    case 239:
        return "EOR";
    case 238:
        return "ABORT";
    case 237:
        return "SUSP";
    case 236:
        return "xEOF";
    default:
        snprintf(buffer, sizeof(buffer), "%d", (int)cmd);
        return buffer;
    }
}

static const char *get_opt(unsigned char opt) {
    switch (opt) {
    case 0:
        return "BINARY";
    case 1:
        return "ECHO";
    case 2:
        return "RCP";
    case 3:
        return "SGA";
    case 4:
        return "NAMS";
    case 5:
        return "STATUS";
    case 6:
        return "TM";
    case 7:
        return "RCTE";
    case 8:
        return "NAOL";
    case 9:
        return "NAOP";
    case 10:
        return "NAOCRD";
    case 11:
        return "NAOHTS";
    case 12:
        return "NAOHTD";
    case 13:
        return "NAOFFD";
    case 14:
        return "NAOVTS";
    case 15:
        return "NAOVTD";
    case 16:
        return "NAOLFD";
    case 17:
        return "XASCII";
    case 18:
        return "LOGOUT";
    case 19:
        return "BM";
    case 20:
        return "DET";
    case 21:
        return "SUPDUP";
    case 22:
        return "SUPDUPOUTPUT";
    case 23:
        return "SNDLOC";
    case 24:
        return "TTYPE";
    case 25:
        return "EOR";
    case 26:
        return "TUID";
    case 27:
        return "OUTMRK";
    case 28:
        return "TTYLOC";
    case 29:
        return "3270REGIME";
    case 30:
        return "X3PAD";
    case 31:
        return "NAWS";
    case 32:
        return "TSPEED";
    case 33:
        return "LFLOW";
    case 34:
        return "LINEMODE";
    case 35:
        return "XDISPLOC";
    case 36:
        return "ENVIRON";
    case 37:
        return "AUTHENTICATION";
    case 38:
        return "ENCRYPT";
    case 39:
        return "NEW-ENVIRON";
    case 70:
        return "MSSP";
    case 85:
        return "COMPRESS";
    case 86:
        return "COMPRESS2";
    case 93:
        return "ZMP";
    case 255:
        return "EXOPL";
    default:
        return "unknown";
    }
}

#define NBCAR 12

static void print_dump(const char *buffer, size_t size) {
    size_t i;
    unsigned char buff[17];
    if (size == 0) {
        printf("libtelnetevent       ZERO LENGTH\n");
        return;
    }
    if (size < 0) {
        printf("libtelnetevent       NEGATIVE LENGTH: %zd\n", size);
        return;
    }
    for (i = 0; i < size; i++) {
        if ((i % NBCAR) == 0) {
            if (i != 0)
                printf ("libtelnetevent         %s\nlibtelnetevent       \t\t", buff);
        }
        printf (" %02x", buffer[i]);
        if ((buffer[i] < 0x20) || (buffer[i] > 0x7e))
            buff[i % NBCAR] = '.';
        else
            buff[i % NBCAR] = buffer[i];
        buff[i % NBCAR + 1] = '\0';
    }
    while ((i % NBCAR) != 0) {
        printf ("   ");
        i++;
    }
    printf ("  %s\n", buff);
}


void mg_telnet_dump_event(telnet_t *telnet, telnet_event_t *ev) {
    switch (ev->type) {

    case TELNET_EV_DATA: /* data received */
        printf("libtelnetevent     <= DATA [%zi]:\t", ev->data.size);
        print_dump(ev->data.buffer, ev->data.size);
        break;
    case TELNET_EV_SEND: /* data must be sent */
        printf("libtelnetevent     => SEND [%zi]:\t", ev->data.size);
        print_dump(ev->data.buffer, ev->data.size);
        break;

    case TELNET_EV_IAC:
        printf("libtelnetevent     <= IAC %d (%s)\n", (int)ev->iac.cmd, get_cmd(ev->iac.cmd));
        break;
    case TELNET_EV_WILL:
        printf("libtelnetevent     <= WILL %d (%s)\n", (int)ev->neg.telopt, get_opt(ev->neg.telopt));
        break;
    case TELNET_EV_WONT:
        printf("libtelnetevent     <= WONT %d (%s)\n", (int)ev->neg.telopt, get_opt(ev->neg.telopt));
        break;
    case TELNET_EV_DO:
        printf("libtelnetevent     <= DO %d (%s)\n", (int)ev->neg.telopt, get_opt(ev->neg.telopt));
        break;
    case TELNET_EV_DONT:
        printf("libtelnetevent     <= DONT %d (%s)\n", (int)ev->neg.telopt, get_opt(ev->neg.telopt));
        break;

    case TELNET_EV_SUBNEGOTIATION:
        switch (ev->sub.telopt) {
        case TELNET_TELOPT_ENVIRON:
        case TELNET_TELOPT_NEW_ENVIRON:
        case TELNET_TELOPT_TTYPE:
        case TELNET_TELOPT_ZMP:
        case TELNET_TELOPT_MSSP:
        default:
            printf("libtelnetevent     <= SUBNEGOTIATION %d (%s) [%zi]\n", (int)ev->sub.telopt, get_opt(ev->sub.telopt), ev->sub.size);
            break;
        }
        break;
    case TELNET_EV_ZMP:
        printf("libtelnetevent     <= ZMP (%s) [%zi]\n", ev->zmp.argv[0], ev->zmp.argc);
        break;
    case TELNET_EV_TTYPE:
        printf("libtelnetevent     <= TTYPE %s %s\n", ev->ttype.cmd ? "SEND" : "IS", ev->ttype.name ? ev->ttype.name : "");
        break;
    case TELNET_EV_ENVIRON:
        printf("libtelnetevent     <= ENVIRON [%zi parts] ==> %s", ev->environ.size, ev->environ.cmd == TELNET_ENVIRON_IS ? "IS" : (ev->environ.cmd == TELNET_ENVIRON_SEND ? "SEND" : "INFO"));
        for (int i = 0; i != ev->environ.size; ++i) {
            printf(" %s \"", ev->environ.values[i].type == TELNET_ENVIRON_VAR ? "VAR" : "USERVAR");
            if (ev->environ.values[i].var != 0) {
                print_dump(ev->environ.values[i].var, strlen(ev->environ.values[i].var));
            }
            printf("\"");
            if (ev->environ.cmd != TELNET_ENVIRON_SEND) {
                printf("=\"");
                if (ev->environ.values[i].value != 0) {
                    print_dump(ev->environ.values[i].value, strlen(ev->environ.values[i].value));
                }
                printf("\"");
            }
        }
        printf("\n");
        break;
    case TELNET_EV_MSSP:
        printf("libtelnetevent     MSSP [%zi] ==>", ev->mssp.size);
        for (int i = 0; i != ev->mssp.size; ++i) {
            printf(" \"");
            print_dump(ev->mssp.values[i].var, strlen(ev->mssp.values[i].var));
            printf("\"=\"");
            print_dump(ev->mssp.values[i].value, strlen(ev->mssp.values[i].value));
            printf("\"");
        }
        printf("\n");
        break;
    case TELNET_EV_COMPRESS:
        printf("libtelnetevent     COMPRESSION %s\n", ev->compress.state ? "ON" : "OFF");
        break;
    case TELNET_EV_WARNING:
        printf("libtelnetevent     WARNING: %s\n", ev->error.msg);
        break;

    case TELNET_EV_ERROR:
        printf("libtelnetevent     ERROR: %s\n", ev->error.msg);
        break;

    default:
        printf("libtelnetevent     <= ???\n");
        break;
    }
}

#endif
