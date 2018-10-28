#ifndef CS_MONGOOSE_SRC_TELNET_H_
#define CS_MONGOOSE_SRC_TELNET_H_

#ifdef MG_ENABLE_TELNET


#include <libtelnet.h>



/*
 * Attach a built-in telnet event handler to the given connection.
 * The user-defined event handler will receive following extra events:
 *
 * - MG_EV_TELNET_POLL: Sent to all connections on each invocation of mg_mgr_poll()
 * - MG_EV_TELNET_ACCEPT:  a new telnet server connection is accepted by a listening connection
 * - MG_EV_TELNET_CLOSE: a  telnet server connection is closing
 * - MG_EV_TELNET_DATA: new data is received. Use mg_telnet_get_recv_data and mg_telnet_get_recv_size to get it.
 * - MG_EV_TELNET_OTHER: other libtelnet receiving events
 *
 * Data must be sent with telnet_send & co functions.
 */
void mg_set_protocol_telnet(struct mg_connection *nc);


#define MG_EV_TELNET_POLL     100
#define MG_EV_TELNET_ACCEPT   101
#define MG_EV_TELNET_CLOSE    102
#define MG_EV_TELNET_DATA     103
#define MG_EV_TELNET_OTHER  104


/* Set the telopt support table given to the telnet_init function. */
void mg_telnet_set_default_telopt(struct mg_connection *nc, const telnet_telopt_t *default_telnet_telopt);

/* Helper function to get the telnet_t* handle inside the event MG_EV_TELNET_* handler function. Needed to send data with the telnet_send & co functions. */
telnet_t* mg_telnet_get_telnet_handle(struct mg_connection *nc);

/* Helper function to get the receiving data  inside the event MG_EV_TELNET_DATA handler function. */
const char* mg_telnet_get_recv_data(struct mg_connection *nc);

/* Helper function to get the receiving data  inside the event MG_EV_TELNET_DATA handler function. */
int mg_telnet_get_recv_size(struct mg_connection *nc);

/* Helper function to get the telnet event, if needed, inside the event MG_EV_TELNET_DATA or MG_EV_TELNET_OTHER handler function. */
telnet_event_t* mg_telnet_get_event(struct mg_connection *nc);

/* Debug function. Print a libtelnet event content. */
void mg_telnet_dump_event(telnet_t *telnet, telnet_event_t *ev);


#endif

#endif
