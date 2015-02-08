/*
   server.c,
   a socket server program.
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>

#define PORT 2578
#define MAX 1024

#ifdef __USE_SIGCHLD__
void sigchld_handler()
{
   int status;
   
   wait(&status); 
}
#else
void *manager_thread(void *argu)
{
   int status;
   
   while (1) {
      wait(&status);                                                                                                                                                  
   }
}
#endif

#define  HTTP_END          0x1
#define  HTTP_PROTO_GET    0x2

#define  CR    '\r'
#define  LF    '\n'

char *GetLine(int fd)
{
   char netread[MAX], readch;
   ssize_t n;
   int len;

   len = 0;
   netread[0] = CR;
   netread[1] = LF;

   while (len < MAX) {
      n = read(fd, &readch, 1);
      if (n <= 0) break;

#ifdef __USE_SIGCHLD__
      if(n < 0 && errno == EINTR) {
         perror("Exception(read)");
         continue;
      }
#endif
      
      netread[len++] = readch;
      if (readch == LF) break;
   }

   netread[len] = '\0';
   return netread;
}

int http_parser(char *s)
{
    if (s[0] == CR && s[1] == LF)
	   return HTTP_END;
    else if (strncmp(s, "GET", 3) == 0)
	   return HTTP_PROTO_GET;
       
    return 0;
}

int main(int argc, char *argv[])
{
   pid_t child;
   int sockfd;
   int client_sockfd, len;
   int r;
   int ret;
   int http_status;
   char msg[MAX+4];
   struct sockaddr_in server_addr;
   struct sockaddr_in client_addr;
   pthread_t main_thread;

   /* 1. Create a socket. */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd == -1) {
      perror(NULL);
      exit(1);
   }

   /* 2. Bind an address to the socket. */
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(PORT);

   if (bind(sockfd, &server_addr, sizeof(struct sockaddr_in)) < 0) {
      perror(NULL);
      exit(1);
   }
   
#ifdef __USE_SIGCHLD__
   struct sigaction sa;
   
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = sigchld_handler;
   
   sigaction(SIGCHLD, &sa, NULL);
#else
   pthread_create(&main_thread, NULL, &manager_thread, NULL);
#endif
   
repeat:
   /* 3. Listen for connections. */
   printf("Listening for connections...\n");
   ret = listen(sockfd, 1);

#ifdef __USE_SIGCHLD__
   if(ret < 0 && errno == EINTR) {
      perror("Exception(listen)");
      goto repeat;
   }
#endif
   
repeat1:
   /* 4. Accept connections. */
   len = sizeof(struct sockaddr_in);
   client_sockfd = accept(sockfd, &client_addr, &len);

#if 0
   if (client_sockfd < 0) {
      if (errno == EINTR) {
          printf("accept restarted\n");
          goto repeat;
      }
      perror("accept");
      return repeat1;
   }
#endif
   
#ifdef __USE_SIGCHLD__
   if(client_sockfd < 0 && errno == EINTR) {
      perror("Exception(accept)");
      goto repeat;
   }
#endif
   
   child = fork();
   if (child != 0)
      goto repeat;

   /* Child */
   printf("Connection Accepted!\n\n");

   /* Receiving and handling messages. */
   int http_get = 0;
   int http_end = 0;
   
   r = 0;
   while (1) {
      strcpy(msg, GetLine(client_sockfd));
      printf("Received[%d]: %s", strlen(msg), msg);
      
      http_status = http_parser(msg);
      
      switch (http_status) {
      case HTTP_PROTO_GET:
         http_get = 1;
         break;
      case HTTP_END:
         printf("Send...\n");
         write(client_sockfd, "<HTML>\n", 7);
         write(client_sockfd, "<H3>HELLO!</H3>\n", 16);
         write(client_sockfd, "</HTML>\n\n", 8);
         write(client_sockfd, '\0', 1);
         
         // child process terminated
         close(client_sockfd);
         exit(1);
      }
   }
}
