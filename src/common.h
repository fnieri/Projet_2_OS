#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // close()

// Return values
const int WRONG_USAGE = 1;

int _checked(int ret, char* calling_function) {
  if (ret < 0) {
    perror(calling_function);
    exit(EXIT_FAILURE);
  }
  return ret;
}

// The macro allows us to retrieve the name of the calling function
#define checked(call) _checked(call, #call)

/**
 * @brief Send data under the form <size_t len><...data>
 * Function name is 'ssend' instead of 'send' because the latter already exists.
 */
int ssend(int sock, void* data, size_t len) {
    checked(write(sock, &len, sizeof(len)));
    return checked(write(sock, data, len));
}

typedef struct message {
    size_t length;
    time_t timestamp;
    char *text;
} message;

/**
 * @brief Send message
 */
int send_message(int sock, message *msg)
{
    checked(write(sock, &msg->length, sizeof(msg->length)));
    checked(write(sock, &msg->timestamp, sizeof(msg->timestamp)));
    return checked(write(sock, msg->text, msg->length));
}

/**
 * @brief Receive message under the form <size_t len><time_t timestamp><data...>.
 */
int receive_message(int sock, message *msg)
{
    size_t nbytes_to_receive;

    // Whether the connection was closed
    if (checked(read(sock, &nbytes_to_receive, sizeof(nbytes_to_receive))) == 0) {
        return 0;
    };

    // Read the timestamp
    checked(read(sock, &msg->timestamp, sizeof(msg->timestamp)));

    // Allocate to receive the message
    char* buffer = malloc(nbytes_to_receive);
    if (buffer == NULL) {
        fprintf(stderr, "malloc could not allocate %zd bytes", nbytes_to_receive);
        perror("");
        exit(1);
    }

    // Receive the message
    size_t total_received = 0;
    while (nbytes_to_receive > 0) {
        int received = checked(read(sock, &buffer[total_received], nbytes_to_receive));
        if (received < 0) {
            break;
        }
        total_received += received;
        nbytes_to_receive -= received;
    }

    msg->length = total_received;
    msg->text = buffer;

    return total_received;
}

/**
 * @brief Receive data under the form <size_t len><data...>.
 */
size_t receive(int sock, void** dest)
{
    size_t nbytes_to_receive;

    // Whether the connection was closed
    if (checked(read(sock, &nbytes_to_receive, sizeof(nbytes_to_receive))) == 0) {
        return 0;
    };

    // Allocate to receive message
    char* buffer = malloc(nbytes_to_receive);
    if (buffer == NULL) {
        fprintf(stderr, "malloc could not allocate %zd bytes", nbytes_to_receive);
        perror("");
        exit(1);
    }

    size_t total_received = 0;
    while (nbytes_to_receive > 0) {
        int received = checked(read(sock, &buffer[total_received], nbytes_to_receive));
        if (received < 0) {
            return total_received;
        }
        total_received += received;
        nbytes_to_receive -= received;
    }

    *dest = buffer;

    return total_received;
}

/**
 * @brief Exit with a msg printed out
 */
void exit_m(int return_code, char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(return_code);
}


#endif  // __COMMON_H