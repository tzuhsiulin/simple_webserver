#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 3001
#define MAX 1024

#define HTTP_GET 0x1
#define CR	'\r'
#define	LF	'\n'

# ifdef __USE_SIGCHLD__
void sigchld_handler() 
{
	int child_process_status;

	wait(&child_process_status);
}
# else
void *zombie_process_handler(void *argu) 
{
	int child_process_status;

	while (1) {
		wait(&child_process_status);

		/*
		if (WIFEXITED(child_process_status)) {
			printf("child process exited normally, and its exit code is %d\n", WEXITSTATUS(child_process_status));
		}
		else {
			printf("child process exited abnormally\n");
		}
		*/
	}
}
# endif

int http_parser(char *s) {
	if (s[0] == CR && s[1] == LF) {
		return 0;
	}
	else if (strncmp(s, "GET", 3) == 0) {
		return HTTP_GET;
	}

	return -1;
}

char *get_line(int fd) {
	char netread[MAX], readch;
	ssize_t n;
	int len;

	len = 0;
	netread[0] = CR;
	netread[1] = LF;

	while (len < MAX) {
		n = read(fd, &readch, 1);
		if (n <= 0) {
			break;
		}

# ifdef __USER_SIGCHLD__
		if (n < 0 && errno == EINTER) {
			perror("Exception(read)");
			continue;
		}
# endif
		
		netread[len++] = readch;
		printf("%d\n", readch);
		if (readch == LF) {
			break;
		}
	}

	netread[len] = '\0';
	return netread;	
}

void request_handler(int sockfd) {
	int is_get = 0;
	char *response = "<html>\n<h3>Hello World</h3>\n</html>\n\n";
	char *request;

	request = get_line(sockfd);	
	is_get = http_parser(request) == HTTP_GET ? 1:0;
	
	if (is_get == 1) {
		write(sockfd, "<HTML>\n", 7);
		write(sockfd, "<H3>HELLO!</H3>\n", 16);
		write(sockfd, "</HTML>\n\n", 8);
		write(sockfd, '\0', 1);
	} 

	close(sockfd);	
}

int main() 
{
	pthread_t zph_thread;
	pid_t self, child_pid;
	int child_count = 0;
	int sockfd;
	int client_sockfd, len;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	struct sigaction sa;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror(NULL);
		exit(1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0) {
		perror(NULL);
		exit(1);
	}

# ifdef __USE_SIGCHLD__
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);
# else
	pthread_create(&zph_thread, NULL, &zombie_process_handler, NULL);
# endif

		
REPEAT_LISTEN:
	printf("Listening for connections...\n");
	listen(sockfd, 1);

# ifdef __USE_SIGCHLD__
	if (errno == EINTR) {
		perror("Execption(listen)");
		goto REPEAT_LISTEN; 
	}
# endif
	
	len = sizeof(struct sockaddr_in);

REPEAT_ACCEPT:	
	client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);
	
# ifdef __USER_SIGCHLD__
	if (client_sockfd < 0 && errno == EINTR) {
		perror("Exception(accept)");
		goto REPEAT_LISTEN;
	}
# endif

	/*
	if (client_sockfd < 0) {
		perror("Exception(client socket error)");
		goto REPEAT_ACCEPT;
	}
	*/

	child_pid = fork();
	if (child_pid == 0) {
		request_handler(client_sockfd);
		printf("123\n");
		exit(1);
	}
	else {
		printf("%d\n", child_pid);
		goto REPEAT_LISTEN;
	}
}
