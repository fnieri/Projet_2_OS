#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"

/* #include <netinet/in.h> */
/* #include <stdbool.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h>  //strlen */
/* #include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros */

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

    checked(bind(serv->sock, (struct sockaddr *) &serv->addr, sizeof(serv->addr)));
    checked(listen(serv->sock, 3));

    return EXIT_SUCCESS;
}

void server_loop(server *serv)
{
    size_t addrlen = sizeof(serv->addr);
    fd_set readfds;

    int clients[1024];  // max clients
    int nclients = 0;

    for (;;) {
        FD_ZERO(&readfds);
        FD_SET(serv->sock, &readfds);

        int max_fd = serv->sock;
        for (int i = 0; i<nclients; ++i) {
            FD_SET(clients[i], &readfds);
            if (clients[i] > max_fd) {
                max_fd = clients[i];
            }
        }

        // Wait for activity on one of the sockets
        select(max_fd + 1, &readfds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(serv->sock, &readfds)) {
            clients[nclients] = accept(serv->sock, (struct sockaddr *) &serv->addr, (socklen_t *) &addrlen);
            ++nclients;

        // Message from one client
        } else {
            for (int i = 0; i < nclients; ++i) {
                if (FD_ISSET(clients[i], &readfds)) {
                    message msg;
                    size_t nbytes = receive_message(clients[i], &msg);

                    // Client sent message
                    if (nbytes > 0) {
                        for (int j = 0; j<nclients; ++j)
                            send_message(clients[j], &msg);
                        free(msg.text);

                    // Client disconnected, remove it
                    } else {
                        close(clients[i]);
                        clients[i] = clients[nclients - 1];
                        --nclients;
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
    else if (port<1024)
        exit_m(WRONG_USAGE, "System ports cannot be used.\n");
}

int main(int argc, char **argv)
{
    check_args(argc, argv);

    server serv;
    server_init(&serv, atoi(argv[1]));

    server_loop(&serv);

    return 0;
}
