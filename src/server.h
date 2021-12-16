#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

// Structure representing server with socket and address
typedef struct {
    int sock;
    struct sockaddr_in addr;
    int clients_fd[1024]; // max clients
    char *clients_pseudo[1024];
    int clients_count;
} server;

void sigint_handler(int receiver_sig);

/**
 * @brief Initialize server with socket, address and port
 */
int server_init(server *serv, unsigned short port);

/**
 * @brief Add a client to client list and its username to username client list
 *
 */
int add_client(server *const serv);

/**
 * @brief Remove a disconnected client
 */
int remove_client(server *const serv, int idx);

/**
 * @brief Send message to all clients
 */
int relay_message(server *const serv, char *const pseudo, message *msg);
