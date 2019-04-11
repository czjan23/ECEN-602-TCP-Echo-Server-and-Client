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

#define MAX_CONNECTION 10 // max connnection
#define BUFF_SIZE 256 //server side buffer size

void print_err_and_exit(char* err_message) {
    fprintf(stdout, "Error: %s\n", err_message);
    exit(1);
}

// zombie process handler
void zombie_handler(int signal) {
   int status = 0;
   while(waitpid(-1, &status, WNOHANG) > 0) {
      //do nothing
   }
}

int main(int argc, char* argv[]) {
   // check the command line is valid or not
	if (argc != 2) {
		print_err_and_exit("usage: echos port_number!");
	}

   // craete father socket, used to accept client connection
   int father_fd = socket(AF_INET, SOCK_STREAM, 0);
   if(father_fd < 0) {
      print_err_and_exit("socket error!");
   }

   // init father socket sockaddr_in
   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(atoi(argv[1]));
   server_addr.sin_addr.s_addr = INADDR_ANY;

   // reuse the port number
   int on = 1;
   if (setsockopt(father_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
       print_err_and_exit("setsockopt error!");
   }

   // bind socket
   if(bind(father_fd, (struct sockaddr *)&server_addr, sizeof server_addr) < 0) {
      print_err_and_exit("bind error!");
   }

   // scoket start listening
   if(listen(father_fd, MAX_CONNECTION) < 0) {
      print_err_and_exit("listen error!");
   }

   fprintf(stdout, "Server is listening on: %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

   // register zombie process handler
   signal(SIGCHLD, zombie_handler);

   struct sockaddr_in connection_addr;
   socklen_t connection_addr_len = sizeof connection_addr;
   int connection_fd;
   pid_t pid;

   while(1) {

      // accept connection
      connection_fd = accept(father_fd, (struct sockaddr *)&connection_addr, &connection_addr_len);
      if(connection_fd < 0) {
         print_err_and_exit("accept error!");
      }

      fprintf(stdout, "Connected with client: %s:%d\n", inet_ntoa(connection_addr.sin_addr), ntohs(connection_addr.sin_port));

      // fork a child process
      pid = fork();

      if (pid == -1) {
         print_err_and_exit("fork error!");
      }

      if(pid == 0) {

         //child process shouldn't accept connection
         close(father_fd);

         // receiving buffer
         char buffer[BUFF_SIZE];

         while(1) {
            memset(buffer, 0, sizeof(buffer));

            // read from client
            ssize_t len = read(connection_fd, buffer, sizeof(buffer));

            if(len > 0) {
               fprintf(stdout, "Received from %s:%d: %s", inet_ntoa(connection_addr.sin_addr), ntohs(connection_addr.sin_port), buffer);
               writen(connection_fd, buffer, len);
            }

            if(len == 0) {
               fprintf(stdout, "Client disconnected: %s:%d\n", inet_ntoa(connection_addr.sin_addr), ntohs(connection_addr.sin_port));
               break;
            }
         }

         close(connection_fd);
         exit(0);
      }

      close(connection_fd);
   }

   return 0;
}
