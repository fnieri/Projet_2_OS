  // Client side C/C++ program to demonstrate Socket programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "string.h"
#include "common.h"
#include <stdlib.h>                                                             
#include <time.h>    

void build_message_struct(message *current_message_struct,  size_t buffer_lenght, char* buffer ) {
  time_t current_time = time(NULL);
  current_message_struct->length =  buffer_lenght;
  current_message_struct->timestamp = current_time;
  current_message_struct->text = buffer;
}



void read_stdin(struct sockaddr_in *server_address, int port, int server_socket, const char* nickname) {
  checked(connect(server_socket, (struct sockaddr *)server_address, sizeof(*server_address)));
  char buffer[1024];
  ssize_t nbytes = 1;
    while (nbytes > 0 && fgets(buffer, 1024, stdin)) {
      // Supprimer le \n
      size_t len = strlen(buffer);
      buffer[len - 1] = '\0';
      message message_to_send;
      build_message_struct(&message_to_send, len, buffer);
      nbytes = send_message(server_socket, &message_to_send);
      
      if (nbytes > 0) {
        char *recvbuffer;
        nbytes = receive(server_socket, (void *)&recvbuffer);
        if (nbytes > 0) {
          printf("%s: %s\n", nickname, recvbuffer);
          free(recvbuffer);
        }
      }
    }
}




int establish_connection(struct sockaddr_in *server_address, const char* server_ip, unsigned short int port) {
  int server_socket = checked(socket(AF_INET, SOCK_STREAM, 0));
  server_address->sin_family = AF_INET;
  server_address->sin_port = htons(port);
  checked(inet_pton(AF_INET, server_ip, &server_address->sin_addr));

  return server_socket;
}

int main(int argc, char const *argv[]) {
    
    if (argc != 4) {
      printf("Wrong number of arguments was introduced, correct usage is: \n ./client <pseudo> <ip_serveur> <port>");
    }
    else {
      struct sockaddr_in server_address;
      const char* nickname = argv[1];
      unsigned short int port = (unsigned short) strtoul(argv[3], NULL, 0); 
      int server_socket = establish_connection(&server_address, argv[2], port);
      read_stdin(&server_address, port, server_socket, nickname);
      }
    }

