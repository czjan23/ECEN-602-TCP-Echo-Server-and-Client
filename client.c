#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#include "utils.h"

#define SEND_BUFF_SIZE 256 // sender buffer size
#define RECV_BUFF_SIZE 10 // receiver buffer size

//socket file descriptor
static int socket_fd;

 //child process id
static int child_pid;

//send this message when disconecting
static char *disconnect_message = "close\n";

void print_err_and_exit(char* err_message) {
    fprintf(stdout, "Error: %s\n", err_message);
    exit(1);
}

// zombie process hanlder
void zombie_handler(int signal) {
   int status = 0;
   while(waitpid(-1, &status, WNOHANG) > 0) {
      //do nothing
   }
}

// disconnection handler
void disconnection_handler(int sig) {
   if(child_pid == getpid()) {
      _exit(0);
   }
   // send the disconnection message
   writen(socket_fd, disconnect_message, strlen(disconnect_message));
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		print_err_and_exit( "usage: echos ip_address port_number\n");
	}

   // zombie process handler
   signal(SIGCHLD, zombie_handler);

   // disconnection handler
   signal(SIGINT, disconnection_handler);

   socket_fd = socket(AF_INET, SOCK_STREAM, 0);
   if(socket_fd < 0) {
      print_err_and_exit("socket error");
   }

   struct sockaddr_in client_addr;
   client_addr.sin_family = AF_INET;
   inet_pton(AF_INET, argv[1], &client_addr.sin_addr);
   client_addr.sin_port = htons(atoi(argv[2]));

   // connect to server
   int connection_fd = connect(socket_fd, (struct sockaddr *)&client_addr, sizeof client_addr);
   if(connection_fd < 0) {
      print_err_and_exit("connect error");
   }

   fprintf(stdout, "Connecting to: %s:%s\n", argv[1], argv[2]);

   //create child process
   pid_t pid = fork();

   // father process dealing with receiving message
   if(pid != 0) {
      char buf[RECV_BUFF_SIZE + 1];

      while (1) {
         memset(buf, 0, sizeof(buf));

         // receive message from server
         ssize_t len = readline(socket_fd, buf, sizeof(buf) - 1);

         if(len < 0) {
            print_err_and_exit("readline error");
         }
         if(len > 0) {
            fprintf(stdout, "Received from server: %s\n", buf);
            if(memcmp(buf, disconnect_message, strlen(disconnect_message)) == 0) {
               break;
            }
         }
         if (len == 0) {
            puts("closed");
            break;
         }
      }

      close(socket_fd);
      exit(1);
   }

   // child process handles sending message
   child_pid = getpid();

   while (1) {
      char buf[SEND_BUFF_SIZE];

      memset(buf, 0, sizeof(buf));

      // get message from terminal
      char *inputs = my_fgets(buf, sizeof(buf), stdin);

      // handle EOF
      if(strcmp(inputs, "EOF") == 0) {
         writen(socket_fd, disconnect_message, strlen(disconnect_message));
         exit(1);
      }

      if(inputs != NULL) {
         // send message
         writen(socket_fd, inputs, strlen(inputs));
      }
   }

   close(socket_fd);
}
