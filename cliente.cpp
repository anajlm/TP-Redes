#include "common.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

#define BUFSZ 500

bool erro = false;

void *recv_thread(void *arg){
	int s = *((int *)arg);

	while(1){
		char buf_recv[BUFSZ];
		memset(buf_recv, 0, BUFSZ);
		size_t count = recv(s, buf_recv, BUFSZ, 0);
		if (count == -1) {
			erro = true;
		}
		printf("\r%s",buf_recv);
	}
}

void *send_thread(void *arg){
	int s = *((int *)arg); 
	
	while(1){
		char buf_send[BUFSZ];
		memset(buf_send, 0, BUFSZ);
		fgets(buf_send, BUFSZ-1, stdin);
		size_t count = send(s, buf_send, strlen(buf_send)+1, 0);
		if (count != strlen(buf_send)+1) {
			erro = true;
		}
	}
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);

	int *arg = &s; //passa o socket como argumento para as funções das threads
				   //para ser usado na chamada das funções send() e recv()

	pthread_t send_msg;
    pthread_create(&send_msg, NULL, send_thread, arg);
	
	pthread_t recv_msg;
    pthread_create(&recv_msg, NULL, recv_thread, arg);

	while(1){ //mantém a thread primária (main) em execução para impedir o fechamento das threads secundárias 
		if(erro){
			break;
		}
	}

	exit(EXIT_SUCCESS);
}