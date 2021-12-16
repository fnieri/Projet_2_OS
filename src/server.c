#include "server.h"

#include "common.h"

#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int sigint_triggered = 0;

void sigint_handler(int receiver_sig)
{
    if (receiver_sig == SIGINT)
        sigint_triggered = 1;
}

int server_init(server *serv, unsigned short port)
{
    int opt = 1;
    serv->sock = checked(socket(AF_INET, SOCK_STREAM, 0));
    checked(setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)));

    serv->addr.sin_family = AF_INET;
    serv->addr.sin_addr.s_addr = INADDR_ANY;
    serv->addr.sin_port = htons(port);

    checked(bind(serv->sock, (struct sockaddr *)&serv->addr, sizeof(serv->addr)));
    checked(listen(serv->sock, 1024));

    serv->clients_count = 0; // no clients by default

    return EXIT_SUCCESS;
}

/**
 * @brief Send message to all clients
 */
int relay_message(server *const serv, char *const pseudo, message *msg)
{
    for (int i = 0; i < serv->clients_count; ++i) {
        ssend(serv->clients_fd[i], (void *)pseudo, strlen(pseudo) + 1);
        send_message(serv->clients_fd[i], msg);
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Add a new connected client
 *
 * A user shall send a username with its request for connection.
 */
int add_client(server *const serv)
{
    size_t saddrlen = sizeof(serv->addr);

    serv->clients_fd[serv->clients_count] = accept(serv->sock, (struct sockaddr *)&serv->addr, (socklen_t *)&saddrlen);
    checked(receive(serv->clients_fd[serv->clients_count], (void *)&serv->clients_pseudo[serv->clients_count])); // receive the pseudonym

    char buff[1024];
    sprintf(buff, "%s joined", serv->clients_pseudo[serv->clients_count]);

    char usrname[] = "Server";
    size_t len = strlen(buff) + 1;
    message *msg = build_message_struct(len, buff);

    relay_message(serv, usrname, msg);

    ++serv->clients_count;

    return EXIT_SUCCESS;
}

/**
 * @brief Remove a disconnected client
 *
 * The relay function, sends the message to all clients,
 * thus the removed client must be cleared before
 * the quit message is sent because sending it to him
 * is uneccessary.  However the information
 * on this user is needed for the message. Therefore,
 * the quit message is built before the clearing,
 * and is sent after the clearing.
 */
int remove_client(server *const serv, int idx)
{
    char buff[1024];
    sprintf(buff, "%s quit", serv->clients_pseudo[idx]);

    char usrname[] = "Server";
    size_t len = strlen(buff) + 1;
    message *msg = build_message_struct(len, buff);

    close(serv->clients_fd[idx]);
    free(serv->clients_pseudo[idx]);

    serv->clients_fd[idx] = serv->clients_fd[serv->clients_count - 1];
    serv->clients_pseudo[idx] = serv->clients_pseudo[serv->clients_count - 1];

    --serv->clients_count;

    relay_message(serv, usrname, msg);

    return EXIT_SUCCESS;
}

/**
 * @brief Reset the fd_set so it's ready for another request
 */
int reset_fd_set(server *const serv, fd_set *const readfds)
{
    FD_ZERO(readfds);
    FD_SET(serv->sock, readfds);

    int max_fd = serv->sock;

    for (int i = 0; i < serv->clients_count; ++i) {
        FD_SET(serv->clients_fd[i], readfds);
        if (serv->clients_fd[i] > max_fd) {
            max_fd = serv->clients_fd[i];
        }
    }

    return max_fd;
}

int cleanup(server *const serv)
{
    for (int i = 0; i < serv->clients_count; ++i) {
        remove_client(serv, i);
    }

    return EXIT_SUCCESS;
}

void server_loop(server *serv)
{
    fd_set readfds;

    for (;;) {
        int max_fd = reset_fd_set(serv, &readfds);

        // Wait for activity on one of the sockets
        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // Ctrl-C triggered
        if (sigint_triggered) {
            cleanup(serv);
            break;

            // New connection
        } else if (FD_ISSET(serv->sock, &readfds)) {
            add_client(serv);

            // Message from one client
        } else {
            for (int i = 0; i < serv->clients_count; ++i) {
                if (FD_ISSET(serv->clients_fd[i], &readfds)) {
                    message msg;
                    size_t nbytes = receive_message(serv->clients_fd[i], &msg);

                    /* printf("Message from %s\n", serv->clients_pseudo[i]); */

                    // Client sent message
                    if (nbytes > 0) {
                        relay_message(serv, serv->clients_pseudo[i], &msg);
                        free(msg.text);

                        // Client disconnected, remove it
                    } else {
                        remove_client(serv, i);
                    }
                }
            }
        }
    }
}

void check_args(int argc, char **argv)
{
    // Number
    if (argc != 2)
        exit_m(WRONG_USAGE, "Wrong number of arguments. Correct usage is: \n ./server <port> \n");

    // Type and value
    unsigned short port = (unsigned short)strtoul(argv[1], NULL, 0);
    if (!port)
        exit_m(WRONG_USAGE, "Arguments of wrong type.\n");
    else if (port < 1024)
        exit_m(WRONG_USAGE, "System ports cannot be used.\n");
}

int main(int argc, char **argv)
{
    check_args(argc, argv);

    server serv;
    server_init(&serv, (unsigned short)strtoul(argv[1], NULL, 0));

    signal(SIGINT, sigint_handler);

    server_loop(&serv);

    return 0;
}
