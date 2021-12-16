#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"


//Structure representing server with socket and address
typedef struct {
    int sock;
    struct sockaddr_in addr;
} server;

void sigint_handler(int receiver_sig);

/**
 * @brief Initialize server with socket, address and port    
 */
int server_init(server *serv, int port);

/**
 * @brief Add a client to client list and its username to username client list
 * 
 */
int add_client(server *const serv, int *const clist, int *const ccount, char **cpseudo);

/**
 * @brief Remove a disconnected client
 */
int remove_client(int *const clist, int *const ccount, int *const index, char **cpseudo);

/**
 * @brief Send message to all clients
 */
int relay_message(int *const clist, int *const ccount, char *pseudo, message *msg);
