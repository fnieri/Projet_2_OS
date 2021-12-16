#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "string.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>


// Windows used for printing incoming messages and reading user input
typedef struct {
    WINDOW *output_border, *output_content;
    WINDOW *input_border, *input_content;
} curses_ui;


//Thread arguments for threaded functions
typedef struct {
    pthread_mutex_t* mutex;
    int server_socket;
    curses_ui *ui;
} thread_args_t;

int ui_init(curses_ui *ui);

/**
* @brief Use ncurses to read characters being input by user in chatbox
*/
char *ui_get_input(curses_ui *ui, char * buffer, int size);

/**
*   @brief Print out messages sent by all users and server
*/
int ui_print_message(curses_ui *ui, char *sender, message *msg);

/**
*   @brief Send nickname to server upon connection, it is only sent once
*/
void send_nickname_to_server(int socket, char* nickname);

/**
*  @brief Establish a connection to server, returns socket 
*/
int establish_connection(struct sockaddr_in *server_address, const char* server_ip, unsigned short int port, char* nickname); 

/**
* @brief Receive messages from server and displays them in GUI
*/
void* receive_other_users_messages(void *thread_args);

/**
* @brief Start threaded functions for sending and receiving messages
*/
void client_loop(curses_ui *ui, int server_socket);  

//Check if given command line arguments are correct
void check_args(int argc, char **argv);


int main(int argc, char **argv);