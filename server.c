
#include <sys/socket.h>		/*  socket definitions        */
#include <sys/types.h>		/*  socket types              */
#include <arpa/inet.h>		/*  inet (3) funtions         */
#include <unistd.h>		/*  misc. UNIX functions      */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <getopt.h>
#include<string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/*  Global constants  */

#define MAX_LINE           (1000)
#define LISTENQ        (1024)

#define K 1024
#define M 1048576

static char usage[] =
    "usage: %s [--nodelay] [--help] [--serverport=<port>] [--recvbuffer=<sizer>] [--reqsize <size[K|M]>]  [--verbose=<level>]\n";

static char help[] = "\nTCP Sockets Client Test Options:\n\
--nodelay                  Disable Nagle's Algorithm\n\
--help                     Display this message and exit.\n\
--serverport <port>        Server port to connect to. Default 2000\n\
--recvbuffer <size[K|M]>  Socket receive buffer size. Unix system default\n\
--reqsize <size[K|M]>      Size of a request message. Default 100\n\
--verbose [0 1 2 3]      Verbosity level\n";

static struct option client_options[] = {
	{"nodelay", no_argument, 0, 'n'},
	{"help", no_argument, 0, 'h'},
	{"serverport", required_argument, 0, 'p'},
	{"recvbuffer", required_argument, 0, 'b'},
	{"reqsize", required_argument, 0, 'q'},
	{"verbose", required_argument, 0, 'v'},
	{0, 0, 0, 0}
};

int tcp_nodelay = 0;
unsigned int server_port = 2003;
unsigned long recv_buffer_size = 0;
unsigned long req_size = 100;
char *req_buffer = NULL;
int verb_level = 0;
unsigned long total_bytes_recv = 0;
unsigned long total_recv_calls = 0;

void print_usage(char *exec_name)
{
	printf(usage, exec_name);
}

void print_help(char *exec_name)
{
	print_usage(exec_name);
	printf("%s", help);
}

void sanity_checks(int argc, char *argv[])
{
	int opt = 0;
	int long_index = 0;
	int scale = 1;
	while ((opt = getopt_long(argc, argv, "nhp:b:q:v:",
				  client_options, &long_index)) != -1) {
		switch (opt) {
		case 'n':
			tcp_nodelay = 1;
			break;
		case 'p':
			if (atoi(optarg) > 0) {
				server_port = atoi(optarg);
			} else {
				fprintf(stderr,
					"[ERROR] Invalid server port\n");
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;

		case 'b':
			if (optarg[strlen(optarg) - 1] == 'M'
			    || optarg[strlen(optarg) - 1] == 'm') {
				scale = M;
				optarg[strlen(optarg) - 1] = '\0';
			} else if (optarg[strlen(optarg) - 1] == 'K'
				   || optarg[strlen(optarg) - 1] == 'k') {
				scale = K;
				optarg[strlen(optarg) - 1] = '\0';
			}

			if (atol(optarg) > 0) {
				recv_buffer_size = atol(optarg) * scale;
				scale = 1;
			} else {
				fprintf(stderr,
					"[ERROR] Invalid request buffer size\n");
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'q':
			if (optarg[strlen(optarg) - 1] == 'M'
			    || optarg[strlen(optarg) - 1] == 'm') {
				scale = M;
				optarg[strlen(optarg) - 1] = '\0';
			} else if (optarg[strlen(optarg) - 1] == 'K'
				   || optarg[strlen(optarg) - 1] == 'k') {
				scale = K;
				optarg[strlen(optarg) - 1] = '\0';
			}

			if (atol(optarg) > 0) {
				req_size = atol(optarg) * scale;
				scale = 1;
			} else {
				fprintf(stderr,
					"[ERROR] Invalid request buffer size\n");
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 'v':
			if (atoi(optarg) >= 0) {
				verb_level = atoi(optarg);
			} else {
				fprintf(stderr,
					"[ERROR] Invalid verbosity level\n");
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	req_buffer = calloc(req_size, sizeof(char));

}

int start_listening()
{
	int list_socket;
	struct sockaddr_in servaddr;
	if ((list_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,
			"[ERROR] socket: Failed to create listening socket.\n");
		exit(EXIT_FAILURE);
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully created listening socket\n");

	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(server_port);

	int optval = 1;
	if (setsockopt
	    (list_socket, SOL_SOCKET, SO_REUSEADDR, &optval,
	     sizeof(optval)) != 0) {
		fprintf(stderr,
			"[ERROR] setsockopt: Failed to set SO_REUSEADDR socket option\n");
		return -1;
	}

	if (bind(list_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) <
	    0) {
		fprintf(stdout,
			"[ERROR] bind: Failed to bind to endpoint to port %u\n",
			server_port);
		return -1;
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully binded endpoint to port %u\n",
				server_port);

	}

	if (listen(list_socket, LISTENQ) < 0) {
		fprintf(stdout, "[ERROR] listen: Failed to listen on socket\n");
		return -1;
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully started to listen for new connections\n");

	}
	return list_socket;
}

int establish_connection(int list_socket)
{

	int conn_socket;

	if ((conn_socket = accept(list_socket, NULL, NULL)) < 0) {
		fprintf(stdout,
			"[ERROR] accept: Failed to accept new connection\n");
		return -1;
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully accepted a new connection\n");

	}
	struct protoent *tcp_proto = getprotobyname("tcp");
	if (setsockopt
	    (conn_socket, tcp_proto->p_proto, TCP_NODELAY, &tcp_nodelay,
	     sizeof(tcp_nodelay)) != 0) {
		fprintf(stdout,
			"[ERROR] setsockopt: Failed to set TCP_NODELAY socket option\n");
		return -1;
	}
	if (recv_buffer_size > 0)
		if (setsockopt
		    (conn_socket, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size,
		     sizeof(recv_buffer_size)) != 0) {
			fprintf(stdout,
				"[ERROR] setsockopt: Failed to set SO_RCVBUF socket option\n");
			exit(EXIT_FAILURE);
		}

	if (verb_level >= 1) {
		char *temp = calloc(50, sizeof(char));
		sprintf(temp, "%lu", recv_buffer_size);
		fprintf(stdout,
			"\ntcp nodelay = %d\nserver port = %u\nrecv buffer size = %s\nverbosity level = %d\n",
			tcp_nodelay, server_port,
			(recv_buffer_size == 0) ? "default" : temp, verb_level);
		free(temp);
	}

	return conn_socket;

}

void recv_traffic(int conn_socket)
{
	
	if(verb_level>=1)
		fprintf(stdout, "[INFO] Started receving traffic\n");
	int n = 0;
	struct tcp_info tcpinfo;
	socklen_t len = sizeof(tcpinfo);
	int success;
	while ((n = read(conn_socket, req_buffer, req_size)) > 0) {

		memset(req_buffer, 0, sizeof(req_buffer));
		total_recv_calls++;
		total_bytes_recv += n;
		if (verb_level == 2)
			fprintf(stdout,
				"[INFO] bytes recv = %d\ntotal bytes recv = %lu\ntotal recv calls = %lu\n",
				n, total_bytes_recv, total_recv_calls);
		//usleep(70000);
	}
	success = getsockopt(conn_socket, SOL_TCP, TCP_INFO, &tcpinfo, &len);
	if (success != -1) {
		printf("RTT  %u %u\n", (tcpinfo.tcpi_rtt),
		       tcpinfo.tcpi_rcv_rtt);
	} else {
		fprintf(stderr, "[ERROR] getsockopt: Failed to retrieve TCP_INFO socket option\n");
	}

	if (n < 0) {
		fprintf(stderr, "[ERROR] Read error\n");
	}

	if (close(conn_socket) < 0) {
		fprintf(stderr, "[ERROR] close: Failed to close socket\n");

	} else {
		if (verb_level >= 1)
		fprintf(stdout, "[INFO ] Finished receving traffic\n");
	}
}

void remove_connection(int list_socket)
{
	if (close(list_socket) < 0) {
		fprintf(stderr, "[ERROR] close: Failed to close socket\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{

	sanity_checks(argc, argv);
	int conn_socket;
	int list_socket = start_listening();
	if (list_socket == -1)
		exit(EXIT_FAILURE);
	while (1) {
		conn_socket = establish_connection(list_socket);
		if (conn_socket == -1)
		{
			remove_connection(list_socket);
			exit(EXIT_FAILURE);
		}		
		recv_traffic(conn_socket);

	}
	remove_connection(list_socket);
	return 0;
}
