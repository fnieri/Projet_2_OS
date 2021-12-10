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

typedef struct {
  pthread_mutex_t* mutex;
  int server_socket;
} thread_args_t;


message * build_message_struct(size_t buffer_lenght, char* buffer ) {
  time_t current_time = time(&current_time);
  message* message_to_build = malloc(sizeof(message));
  message_to_build->length = buffer_lenght;
  message_to_build->timestamp = current_time;
  message_to_build->text = buffer;
  return message_to_build;
}

/* @brief Allow user to send messages via stdin
*
*/
void * read_stdin(void * thread_args) {
  
  thread_args_t * arguments = (thread_args_t * ) thread_args;
  pthread_mutex_t *mutex = arguments->mutex;
  int server_socket = arguments->server_socket;
  
  char buffer[1024];
  ssize_t nbytes = 1;
  
  while (nbytes > 0) {
    if (fgets(buffer, 1024, stdin) != NULL) {
      
      //Remove /n
      size_t len = strlen(buffer);
      buffer[len - 1] = '\0';
      
      //Send message to server
      message *new_message = build_message_struct(len, buffer);
      
      //Don't allow messages to be read
      pthread_mutex_lock(mutex);
      nbytes = send_message(server_socket, new_message);
      pthread_mutex_unlock(mutex);
      
      free(new_message);
      }
    
    //Ctrl - D BEWARE UNDEFINED BEHAVIOUR FOR NOW, SERVER CONNECTION RESET BY PEER  
    else {
      exit_m(0, "End of connection to server");
      return NULL;
    } 
  }   
  return NULL;
}


void send_nickname_to_server(int socket, char* nickname) {
  nickname[strlen(nickname)] = '\0';
  ssend(socket, nickname, strlen(nickname));
}


int establish_connection(struct sockaddr_in *server_address, const char* server_ip, unsigned short int port, char* nickname) {
  //Build server address from port
  int server_socket = checked(socket(AF_INET, SOCK_STREAM, 0));
  server_address->sin_family = AF_INET;
  server_address->sin_port = htons(port);
  
  //Connect to server
  checked(inet_pton(AF_INET, server_ip, &server_address->sin_addr));
  checked(connect(server_socket, (struct sockaddr *)server_address, sizeof(*server_address)));
  
  send_nickname_to_server(server_socket, nickname);

  return server_socket;
}

int client_init(char **argv, struct sockaddr_in *server_address) {
  //Initialize argv arguments
  char* nickname = argv[1];
  const char* server_ip = argv[2];
  unsigned short int port = (unsigned short) strtoul(argv[3], NULL, 0); 
  
  //Return server socket from connection 
  int server_socket = establish_connection(server_address, server_ip, port, nickname);
  return server_socket;
}


void* read_placeholder(void *thread_args) {}

void client_loop(int server_socket){
  pthread_t send_thread, receive_thread;
  
  //Iniitalize threaded functions
  void * (*send_function)(void *) = read_stdin;
  void * (*receive_function)(void *) = read_placeholder;
  
  //Create thread arguments
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  thread_args_t thread_arg = {
    .mutex = &mutex,
    .server_socket = server_socket
  };

  void * argument=&thread_arg;

  //Create and join threads
  if (pthread_create(&send_thread, NULL, send_function, argument) != 0 ||
      pthread_create(&receive_thread, NULL, receive_function, argument) != 0) {
    exit_m(1, "Unable to create threads\n");
  }

  if (pthread_join(send_thread, NULL) != 0 ||
      pthread_join(receive_thread, NULL) != 0) {
    exit_m(1, "Unable to join threads\n");
  }
}


void check_args(int argc, char **argv)
{
    // Number
    if (argc != 4)
        exit_m(WRONG_USAGE, "Wrong number of arguments. Correct usage is: \n ./client <pseudo> <ip_server> <port> \n");

    // Type and value
    unsigned short int port = (unsigned short) strtoul(argv[3], NULL, 0);
    if (!port)
        exit_m(WRONG_USAGE, "Arguments of wrong type.\n");
    else if (port<1024)
        exit_m(WRONG_USAGE, "System ports cannot be used.\n");
}


int main(int argc, char **argv) {
    check_args(argc, argv);
    struct sockaddr_in server_address;
    int server_socket = client_init(argv, &server_address);
    client_loop(server_socket);
}
    

