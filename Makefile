
all: 
	gcc -D __USE_SIGCHLD__ -o server server.c
	#gcc -o server server.c -lpthread
