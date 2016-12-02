/* A server that receives a packet and print its content using raw sockets */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/ip.h> /* for ipv4 header */
#include <linux/udp.h> /* for upd header */

#define ADDR_TO_BIND "127.0.0.1"
#define PORT_TO_BIND 8080

#define MSG_SIZE 256
#define HEADER_SIZE (sizeof(struct iphdr) + sizeof(struct udphdr))

int main (void) {
	int raw_socket;
	struct sockaddr_in sockaddr;
	socklen_t socklen;

	char msg[MSG_SIZE];
	ssize_t msglen; /* return value from recv() */

	int retval = 0;

	/* do not use IPPROTO_RAW to receive packets */
	if ((raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("socket");
		return 1;
	}

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(PORT_TO_BIND);
	sockaddr.sin_addr.s_addr = inet_addr(ADDR_TO_BIND);
	socklen = (socklen_t) sizeof(sockaddr);

	if (bind(raw_socket, (struct sockaddr*) &sockaddr, socklen) == -1) {
		perror("bind");
		retval = 1;
		goto _go_close_socket;
	}

	memset(msg, 0, MSG_SIZE);

	if ((msglen = recv(raw_socket, msg, MSG_SIZE, 0)) == -1) {
		perror("recv");
		retval = 1;
		goto _go_close_socket;
	}

	if (msglen <= HEADER_SIZE) /* msg  can't be lesser than header! */
		printf("No msg!\n");
	else {
		msg[msglen - 1] = '\0'; /* we need a null character at the end*/
		printf("Your msg _plus_ headers's size is: %s\n",
		msg + HEADER_SIZE);
	}

_go_close_socket:
	close(raw_socket);

	return retval;
}
