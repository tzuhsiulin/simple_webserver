#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3000
#define MAX 1024

# ifdef _USE_SIGCHLD_
void sigchld_handler() {
	int child_process_status;

	wait(&child_process_status);

	if (WIFEXITED(child_process_status)) {
		printf("child process exited normally, and its exit code is %d\n", WEXITSTATUS(child_process_status));
	}
	else {
		printf("child process exited abnormally\n");
	}
}
# else
void *zombie_process_handler(void *argu) {
	int child_process_status;

	while (1) {
		wait(&child_process_status);

		if (WIFEXITED(child_process_status)) {
			printf("child process exited normally, and its exit code is %d\n", WEXITSTATUS(child_process_status));
		}
		else {
			printf("child process exited abnormally\n");
		}
	}
}
# endif

void request_handler(int sockfd) {
	char msg[MAX + 4];
	char *response = "Hello World";

	if (sockfd == -1) {
		perror(NULL);
		exit(1);
	}
	printf("Connection Accepted!\n");
	
	read(sockfd, msg, sizeof(msg));
	printf("sent from client: %s\n", msg);

	write(sockfd, response, sizeof(response));

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

# ifdef _USE_SIGCHLD_
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);
# else
	pthread_create(&zph_thread, NULL, &zombie_process_handler, NULL);
# endif

	while (1) {	
		printf("Listening for connections...\n");
		listen(sockfd, 5);
	
		len = sizeof(struct sockaddr_in);
		client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);

		child_pid = fork();
		if (child_pid == 0) {
			request_handler(client_sockfd);
			exit(1);
		}
	}
}


