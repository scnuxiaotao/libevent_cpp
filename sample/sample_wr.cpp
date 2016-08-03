#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "event.h"
#include "eventbase.h"

void on_accept(int sock, short event, void* arg) {
	struct sockaddr_in client;
	socklen_t client_addrlength = sizeof(client);

	int connfd = accept(sock, (struct sockaddr*) &client, &client_addrlength);
	if (connfd < 0) {
		printf("errno is: %d\n", errno);
	} else {
		char remote[INET_ADDRSTRLEN];
		printf("connected with ip: %s and port: %d\n",
				inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN),
				ntohs(client.sin_port));
	}
}

int main(int argc, char *argv[]) {
	const char *ip = "127.0.0.1";
	int port = 8868;

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	assert(sock >= 0);

	sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int ret = bind(sock, (struct sockaddr*) &address, sizeof(address));
	assert(ret != -1);

	ret = listen(sock, 5);
	assert(ret != -1);

	event_base ev_base_test;
	event ev_test(sock, EV_READ | EV_PERSIST, on_accept, NULL);
	ev_test.event_base_set(&ev_base_test);
	ev_base_test.event_add(&ev_test, NULL);
	ev_base_test.event_base_loop(0);

	close(sock);

	return 0;
}

