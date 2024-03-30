#include "wifi.h"
#include "mbed.h"

nsapi_size_or_error_t send_request(Socket *socket, const char *request) {
  if (socket == nullptr || request == nullptr) {
    printf("Invalid function parameters\n");
    return NSAPI_ERROR_PARAMETER;
  }

  // The request might not be fully sent in one go,
  // so keep track of how much we have sent
  nsapi_size_t bytes_to_send = strlen(request);
  nsapi_size_or_error_t bytes_sent = 0;

  printf("Sending message: \n%s", request);

  // Loop as long as there are more data to send
  while (bytes_to_send) {
    // Try to send the remaining data.
    // send() returns how many bytes were actually sent
    bytes_sent = socket->send(request + bytes_sent, bytes_to_send);

    if (bytes_sent < 0) {
      // Negative return values from send() are errors
      return bytes_sent;
    } else {
      printf("Sent %d bytes\n", bytes_sent);
    }

    bytes_to_send -= bytes_sent;
  }

  printf("Complete message sent\n");
  
  return bytes_to_send;
}

nsapi_size_or_error_t read_response(Socket *socket, char *buffer,
                                    int buffer_length) {

  if (socket == nullptr || buffer == nullptr || buffer_length < 1) {
    printf("Invalid function parameters\n");
    return NSAPI_ERROR_PARAMETER;
  }

  memset(buffer, 0, buffer_length);

  int remaining_bytes = buffer_length;
  int received_bytes = 0;

  // Loop as long as there are more data to read,
  // we might not read all in one call to recv()
  while (remaining_bytes > 0) {
    nsapi_size_or_error_t result =
        socket->recv(buffer + received_bytes, remaining_bytes);

    // If the result is 0 there are no more bytes to read
    if (result == 0) {
      break;
    }

    // Negative return values from recv() are errors
    if (result < 0) {
      return result;
    }

    received_bytes += result;
    remaining_bytes -= result;
  }

  printf("\nReceived %d bytes:\n%.*s\n", received_bytes,
         strstr(buffer, "\n") - buffer, buffer);

  return received_bytes;
}

const char *get_nsapi_error_string(nsapi_error_t err) {
  switch (err) {
  case NSAPI_ERROR_OK:
    return "NSAPI_ERROR_OK";
  case NSAPI_ERROR_WOULD_BLOCK:
    return "NSAPI_ERROR_WOULD_BLOCK";
  case NSAPI_ERROR_UNSUPPORTED:
    return "NSAPI_ERROR_UNSUPPORTED";
  case NSAPI_ERROR_PARAMETER:
    return "NSAPI_ERROR_PARAMETER";
  case NSAPI_ERROR_NO_CONNECTION:
    return "NSAPI_ERROR_NO_CONNECTION";
  case NSAPI_ERROR_NO_SOCKET:
    return "NSAPI_ERROR_NO_SOCKET";
  case NSAPI_ERROR_NO_ADDRESS:
    return "NSAPI_ERROR_NO_ADDRESS";
  case NSAPI_ERROR_NO_MEMORY:
    return "NSAPI_ERROR_NO_MEMORY";
  case NSAPI_ERROR_NO_SSID:
    return "NSAPI_ERROR_NO_SSID";
  case NSAPI_ERROR_DNS_FAILURE:
    return "NSAPI_ERROR_DNS_FAILURE";
  case NSAPI_ERROR_DHCP_FAILURE:
    return "NSAPI_ERROR_DHCP_FAILURE";
  case NSAPI_ERROR_AUTH_FAILURE:
    return "NSAPI_ERROR_AUTH_FAILURE";
  case NSAPI_ERROR_DEVICE_ERROR:
    return "NSAPI_ERROR_DEVICE_ERROR";
  case NSAPI_ERROR_IN_PROGRESS:
    return "NSAPI_ERROR_IN_PROGRESS";
  case NSAPI_ERROR_ALREADY:
    return "NSAPI_ERROR_ALREADY";
  case NSAPI_ERROR_IS_CONNECTED:
    return "NSAPI_ERROR_IS_CONNECTED";
  case NSAPI_ERROR_CONNECTION_LOST:
    return "NSAPI_ERROR_CONNECTION_LOST";
  case NSAPI_ERROR_CONNECTION_TIMEOUT:
    return "NSAPI_ERROR_CONNECTION_TIMEOUT";
  case NSAPI_ERROR_ADDRESS_IN_USE:
    return "NSAPI_ERROR_ADDRESS_IN_USE";
  case NSAPI_ERROR_TIMEOUT:
    return "NSAPI_ERROR_TIMEOUT";
  case NSAPI_ERROR_BUSY:
    return "NSAPI_ERROR_BUSY";
  default:
    return "NSAPI_ERROR_UNKNOWN";
  }
}
