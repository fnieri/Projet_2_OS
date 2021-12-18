#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // close()
#include <time.h>
#include <errno.h>

// Return values
const int WRONG_USAGE = 1;

// Return values

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

typedef struct {
    size_t length;
    time_t timestamp;
    char *text;
} message;

/**
 * @brief Send message
 *
 * Return:
 *  If successful, the number of bytes sent. Otherwise
 *  -1 in case of an error.
 */
int send_message(int sock, message *msg)
{
    int total = 0;
    total += write(sock, &msg->length, sizeof(msg->length));
    total += write(sock, &msg->timestamp, sizeof(msg->timestamp));
    total += write(sock, msg->text, msg->length);
    return errno == EPIPE ? -1 : total;
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


message * build_message_struct(size_t buffer_lenght, char* buffer ) {
    time_t current_time = time(&current_time);
    message* message_to_build = malloc(sizeof(message));
    message_to_build->length = buffer_lenght;
    message_to_build->timestamp = current_time;
    message_to_build->text = buffer;
    return message_to_build;
}

#endif  // __COMMON_H
