#ifndef CS_MONGOOSE_SRC_TELNET_H_
#define CS_MONGOOSE_SRC_TELNET_H_

#ifdef MG_ENABLE_TELNET


#include <libtelnet.h>



/*
 * Attach a built-in telnet event handler to the given connection.
 */
void mg_set_protocol_telnet(struct mg_connection *nc);


enum mg_telnet_event_t {
    MG_EV_TELNET_POLL,
    MG_EV_TELNET_ACCEPT,
    MG_EV_TELNET_CLOSE,
    MG_EV_TELNET_DATA,
};

/*
 * Callback function (telnet event handler) prototype for `mg_telnet_set_telnet_handler()`
 *
 * Input:
 * - telnet_handle, used to send data with telnet_printf, telnet_raw_printf or telnet_send function
 * - ev: event type
 * - data: received data, in case of MG_EV_TELNET_DATA event
 * - len: received data len
 * - user_data: custom user data
 * Return value: negative to close the connection
 */
typedef int (telnet_event_handler_cb_t)(telnet_t *telnet_handle,enum mg_telnet_event_t ev,const void *data,int len,void **user_data);


/*
 * Attach a a telnet event handler.
 */
void mg_telnet_set_telnet_handler(struct mg_connection *nc, telnet_event_handler_cb_t user_handler);


void mg_telnet_set_default_telopt(struct mg_connection *nc,const telnet_telopt_t *default_telnet_telopt);


#endif

#endif
