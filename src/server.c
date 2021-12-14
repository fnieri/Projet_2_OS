#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

/* #include <netinet/in.h> */
/* #include <stdbool.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h>  //strlen */
/* #include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros */

int sigint_triggered = 0;

void sigint_handler(int receiver_sig)
{
    if (receiver_sig == SIGINT)
        sigint_triggered = 1;
}

typedef struct {
    int sock;
    struct sockaddr_in addr;
} server;

int server_init(server *serv, int port)
{
    int opt = 1;
    serv->sock = checked(socket(AF_INET, SOCK_STREAM, 0));
    checked(setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)));

    serv->addr.sin_family = AF_INET;
    serv->addr.sin_addr.s_addr = INADDR_ANY;
    serv->addr.sin_port = htons(port);

    checked(bind(serv->sock, (struct sockaddr *)&serv->addr, sizeof(serv->addr)));
    checked(listen(serv->sock, 1024));

    return EXIT_SUCCESS;
}

/**
 * @brief Send message to all clients
 */
int relay_message(int *const clist, int *const ccount, char *pseudo, message *msg)
{
    for (int i = 0; i < *ccount; ++i) {
        /* printf("Sending to %s %ld\n", pseudo, strlen(pseudo)+1); */
        ssend(clist[i], (void *)pseudo, strlen(pseudo) + 1);
        send_message(clist[i], msg);
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Add a new connected client
 *
 * A user shall send a username with its request for connection.
 */
int add_client(server *const serv, int *const clist, int *const ccount, char **cpseudo)
{
    size_t saddrlen = sizeof(serv->addr);

    clist[*ccount] = accept(serv->sock, (struct sockaddr *)&serv->addr, (socklen_t *)&saddrlen);
    checked(receive(clist[*ccount], (void *)&cpseudo[*ccount])); // receive the pseudonym

    char buff[1024];
    sprintf(buff, "%s joined", cpseudo[*ccount]);

    char usrname[] = "Server";
    size_t len = strlen(buff)+1;
    message *msg = build_message_struct(len, buff);

    relay_message(clist, ccount, usrname, msg);
    /* printf("%s joined.", cpseudo[*ccount - 1]); */

    ++(*ccount);

    return EXIT_SUCCESS;
}

/**
 * @brief Remove a disconnected client
 */
int remove_client(int *const clist, int *const ccount, int *const index, char **cpseudo)
{
    printf("%s quit.", cpseudo[*index]);

    close(clist[*index]);
    free(cpseudo[*index]);

    clist[*index] = clist[*ccount - 1];
    cpseudo[*index] = cpseudo[*ccount - 1];

    --(*ccount);

    return EXIT_SUCCESS;
}

int reset_fd_set(fd_set *const readfds, server *const serv, int *const clist, int *const ccount)
{
    FD_ZERO(readfds);
    FD_SET(serv->sock, readfds);

    int max_fd = serv->sock;

    for (int i = 0; i < *ccount; ++i) {
        FD_SET(clist[i], readfds);
        if (clist[i] > max_fd) {
            max_fd = clist[i];
        }
    }

    return max_fd;
}

int cleanup(int *const clients_list, int *const clients_count, char **const clients_pseudo)
{
    for (int i = 0; i < *clients_count; ++i) {
        remove_client(clients_list, clients_count, &i, clients_pseudo);
    }

    return EXIT_SUCCESS;
}

void server_loop(server *serv)
{
    fd_set readfds;

    int clients[1024]; // max clients
    char *clients_pseudo[1024];
    int nclients = 0;

    for (;;) {
        int max_fd = reset_fd_set(&readfds, serv, clients, &nclients);

        // Wait for activity on one of the sockets
        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // Ctrl-C triggered
        if (sigint_triggered) {
            cleanup(clients, &nclients, clients_pseudo);
            break;

            // New connection
        } else if (FD_ISSET(serv->sock, &readfds)) {
            add_client(serv, clients, &nclients, clients_pseudo);

            // Message from one client
        } else {
            for (int i = 0; i < nclients; ++i) {
                if (FD_ISSET(clients[i], &readfds)) {
                    message msg;
                    size_t nbytes = receive_message(clients[i], &msg);

                    printf("Message from %s\n", clients_pseudo[i]);

                    // Client sent message
                    if (nbytes > 0) {
                        relay_message(clients, &nclients, clients_pseudo[i], &msg);
                        free(msg.text);

                        // Client disconnected, remove it
                    } else {
                        remove_client(clients, &nclients, &i, clients_pseudo);
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
        exit_m(WRONG_USAGE, "Wrong number of arguments.\n");

    // Type and value
    int port = atoi(argv[1]);
    if (!port)
        exit_m(WRONG_USAGE, "Arguments of wrong type.\n");
    else if (port < 1024)
        exit_m(WRONG_USAGE, "System ports cannot be used.\n");
}

int main(int argc, char **argv)
{
    check_args(argc, argv);

    server serv;
    server_init(&serv, atoi(argv[1]));

    signal(SIGINT, sigint_handler);

    server_loop(&serv);

    return 0;
}
