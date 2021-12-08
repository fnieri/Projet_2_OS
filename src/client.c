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

message * build_message_struct(size_t buffer_lenght, char* buffer ) {
  time_t current_time = time(&current_time);
  message* message_to_build = malloc(sizeof(message));
  message_to_build->length = buffer_lenght;
  message_to_build->timestamp = current_time;
  message_to_build->text = buffer;
  return message_to_build;
}

void read_stdin(int server_socket) {
  char buffer[1024];
  ssize_t nbytes = 1;
  
  while (nbytes > 0) {
    if (fgets(buffer, 1024, stdin) != NULL) {
    
      //Remove /n
      size_t len = strlen(buffer);
      buffer[len - 1] = '\0';
      
      //Send message to server
      message *new_message = build_message_struct(len, buffer);
      nbytes = send_message(server_socket, new_message);
      free(new_message);
  
    } 
    else {
      exit_m(0, "End of connection to server");
      return;
    } 
  }   
}


void send_nickname_to_server(int socket, char* nickname) {
  nickname[strlen(nickname)] = '\0';
  ssend(socket, nickname, strlen(nickname));
}


int establish_connection(struct sockaddr_in *server_address, const char* server_ip, unsigned short int port, char* nickname) {
  
  int server_socket = checked(socket(AF_INET, SOCK_STREAM, 0));
  server_address->sin_family = AF_INET;
  server_address->sin_port = htons(port);
  
  checked(inet_pton(AF_INET, server_ip, &server_address->sin_addr));
  
  checked(connect(server_socket, (struct sockaddr *)server_address, sizeof(*server_address)));
  
  send_nickname_to_server(server_socket, nickname);

  return server_socket;
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


int client_init(char **argv, struct sockaddr_in *server_address) {
  char* nickname = argv[1];
  const char* server_ip = argv[2];
  unsigned short int port = (unsigned short) strtoul(argv[3], NULL, 0); 
  int server_socket = establish_connection(server_address, server_ip, port, nickname);
  return server_socket;
}

void client_loop(){

}

int main(int argc, char **argv) {
    check_args(argc, argv);
    struct sockaddr_in server_address;
    int server_socket = client_init(argv, &server_address);
    read_stdin(server_socket);
}
    

