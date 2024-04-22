#ifndef __WIFI_H__
#define __WIFI_H__

#include "mbed.h"

nsapi_size_or_error_t send_request(Socket *socket, const char *request);

nsapi_size_or_error_t read_response(Socket *socket, char *buffer,
                                    int buffer_length);

const char *get_nsapi_error_string(nsapi_error_t err);

#endif // __WIFI_H__
