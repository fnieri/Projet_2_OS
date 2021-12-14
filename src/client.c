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

typedef struct {
    WINDOW *output_border, *output_content;
    WINDOW *input_border, *input_content;
} curses_ui;

typedef struct {
    pthread_mutex_t* mutex;
    int server_socket;
    curses_ui *ui;
} thread_args_t;

char *ui_get_input(curses_ui *ui, char * buffer, int size)
{
    noecho();
    /* halfdelay(1); */

    keypad(ui->input_content, TRUE);

    int next;
    int idx = 0;
    fd_set read_fd;

    for (;;) {
        FD_ZERO(&read_fd);
        FD_SET(STDIN_FILENO, &read_fd);

        struct timeval timeout = {
            .tv_usec = 200000
        };

        checked(select(STDIN_FILENO + 1, &read_fd, NULL, NULL, &timeout));

        if (FD_ISSET(STDIN_FILENO, &read_fd)) {
            next = wgetch(ui->input_content);

            if (next == 4 || next == '\n') {
                if (idx != 0) {
                    buffer[idx] = '\0';
                    break;
                } else if (next == 4) {
                    buffer = NULL;
                    break;
                }
            } else if (next == '\n' && idx != 0) {
                buffer[idx] = '\0';
                break;
            } else if (next == KEY_BACKSPACE || next == KEY_DC || next == 127) {
                --idx;
                waddch(ui->input_content, '\b');
                wdelch(ui->input_content);
            } else if (strlen(unctrl(next)) == 1 && idx < size-1) {
                buffer[idx] = (char) next;
                ++idx;
                waddch(ui->input_content, next);
            }
        }

        wrefresh(ui->output_content);
        wrefresh(ui->input_content);
    }

    echo();

    checked(wclear(ui->input_content));
    checked(wprintw(ui->input_content, "> "));

    return buffer;
}

int ui_print_message(curses_ui *ui, char *sender, message *msg)
{
    struct tm *msg_time = localtime(&msg->timestamp);

    wprintw(ui->output_content, "\n");

    wprintw(ui->output_content, "%02d:%02d:%02d", msg_time->tm_hour, msg_time->tm_min, msg_time->tm_sec);

    wprintw(ui->output_content, " [");
    wattron(ui->output_content, A_BOLD);
    wprintw(ui->output_content, sender);
    wattroff(ui->output_content, A_BOLD);
    wprintw(ui->output_content, "] ");

    wprintw(ui->output_content, msg->text);

    /* wrefresh(ui->output_content); */
    return EXIT_SUCCESS;
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
    /* if (fgets(buffer, 1024, stdin) != NULL) { */
      /* wgetstr(arguments->ui->input_content, buffer); */
      if (ui_get_input(arguments->ui, buffer, 1024) != NULL) {

      //Remove /n
      size_t len = strlen(buffer)+1;
      /* buffer[len - 1] = '\0'; */

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
      /* exit_m(0, "End of connection to server"); */
        endwin();
        exit(0);
      return NULL;
    }
  }
  return NULL;
}


void send_nickname_to_server(int socket, char* nickname) {
  nickname[strlen(nickname)] = '\0';
  ssend(socket, nickname, strlen(nickname)+1);
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


void* read_placeholder(void *thread_args)
{
    thread_args_t *arguments = (thread_args_t *)thread_args;
    pthread_mutex_t *mutex = arguments->mutex;
    int server_socket = arguments->server_socket;

    char *sender;
    ssize_t nbytes = 1;

    for (;;) {
        nbytes = receive(server_socket, (void *) &sender);
        if (nbytes > 0) {
            message msg;
            pthread_mutex_lock(mutex);
            nbytes = receive_message(server_socket, &msg);
            pthread_mutex_unlock(mutex);
            if (nbytes > 0) {
                ui_print_message(arguments->ui, sender, &msg);
                free(sender);
                free(msg.text);
            }
        } else {
            break;
        }
    }

    return NULL;
}

void client_loop(curses_ui *ui, int server_socket){
  pthread_t send_thread, receive_thread;

  //Iniitalize threaded functions
  void * (*send_function)(void *) = read_stdin;
  void * (*receive_function)(void *) = read_placeholder;

  //Create thread arguments
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  thread_args_t thread_arg = {
    .mutex = &mutex,
    .server_socket = server_socket,
    .ui = ui
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

int ui_init(curses_ui *ui)
{
    initscr();  // init main window
    cbreak();
    /* noecho(); */

    ui->output_border = subwin(stdscr, LINES-3, COLS, 0, 0);
    ui->output_content = subwin(ui->output_border, LINES-5, COLS-2, 1, 1);

    ui->input_border = subwin(stdscr, 3, COLS, LINES-3, 0);
    ui->input_content = subwin(ui->input_border, 1, COLS-2, LINES-2, 1);

    box(ui->output_border, ACS_VLINE, ACS_HLINE);
    box(ui->input_border, ACS_VLINE, ACS_HLINE);

    scrollok(ui->output_content, TRUE);
    /* keypad(stdscr, TRUE); */

    wprintw(ui->output_content, "Welcome to the ChatBox!");

    wrefresh(ui->output_border);
    wrefresh(ui->input_border);

    /* getch(); */

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    check_args(argc, argv);

    struct sockaddr_in server_address;
    int server_socket = client_init(argv, &server_address);

    curses_ui ui;
    ui_init(&ui);

    client_loop(&ui, server_socket);
    endwin();
}


