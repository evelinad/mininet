
#include <sys/socket.h>		/*  socket definitions        */
#include <sys/types.h>		/*  socket types              */
#include <arpa/inet.h>		/*  inet (3) funtions         */
#include <unistd.h>		/*  misc. UNIX functions      */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <linux/tcp.h>
#include <getopt.h>

/*  Global constants  */

#define TIME_PORT          (12865)
#define MAX_LINE           (1000)
#define LISTENQ        (1024)

#define K 1024
#define M 1048576

static char usage[] =
    "usage: %s [--nodelay] [--help] [--serverip=<ip>] [--serverport=<port>] [--recvbuffer=<sizer>] [--verbose=<level>]\n";

static char help[] = "\nTCP Sockets Client Test Options:\n\
--nodelay                  Disable Nagle's Algorithm\n\
--help                     Display this message and exit.\n\
--serverip <ip>            Server ip to connect to. Default 127.0.0.1\n\
--serverport <port>        Server port to connect to. Default 2000\n\
--recv_buffer <size[K|M]>  Socket receive buffer size. Unix system default\n\
--verbosity [0 1 2 3]      Verbosity level\n";

static struct option client_options[] = {
	{"nodelay", no_argument, 0, 'n'},
	{"help", no_argument, 0, 'h'},
	{"serverip", required_argument, 0, 'i'},
	{"serverport", required_argument, 0, 'p'},
	{"recvbuffer", required_argument, 0, 'b'},
	{"verbosity", required_argument, 0, 'v'},
	{0, 0, 0, 0}
};

int tcp_nodelay = 0;
char *server_ip = NULL;
unsigned int server_port = 12865;
unsigned long recv_buffer_size = 100;
int verb_level = 0;
char *recv_buffer = NULL;
unsigned long total_bytes_recv = 0;
unsigned long total_recv_calls = 0;
int list_s;			/*  listening socket          */

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
	server_ip = calloc(40, sizeof(char));
	strcpy(server_ip, "127.0.0.1");
	while ((opt = getopt_long(argc, argv, "nhi:p:r:b:q:t:e:",
				  client_options, &long_index)) != -1) {
		switch (opt) {
		case 'n':
			tcp_nodelay = 1;
			break;
		case 'i':
			strcpy(server_ip, optarg);
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

			if (optarg[strlen(optarg) - 1] == 'M') {
				scale = M;
			} else if (optarg[strlen(optarg) - 1] == 'K') {
				scale = K;
			}
			optarg[strlen(optarg) - 1] = '\0';
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

		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 'v':
			if (atoi(optarg) > 0) {
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
	recv_buffer = calloc(recv_buffer_size, sizeof(char));
	printf("%s\n", server_ip);

}

int establish_connection()
{

	int conn_s;		/*  connection socket         */
	struct sockaddr_in servaddr;	/*  socket address structure  */
	char *endptr;		/*  for strtol()              */
	int n;
	/*  Get port number from the command line, and
	   set to default port if no arguments were supplied  */

	/*  Create the listening socket  */

	if ((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,
			"[ERROR] socket: Failed to create listening socket.\n");
		exit(EXIT_FAILURE);
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully created listening socket\n");

	}

	/*  Set all bytes in socket address structure to
	   zero, and fill in the relevant data members   */

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = server_port;

	int optval = 1;
	if (setsockopt
	    (list_s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
		fprintf(stderr,
			"[ERROR] setsockopt: Failed to set SO_REUSEADDR socket option\n");
		exit(EXIT_FAILURE);
	}

	/*  Bind our socket addresss to the 
	   listening socket, and call listen()  */

	if (bind(list_s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		fprintf(stdout,
			"[ERROR] bind: Failed to bind to endpoint ip = %s port %u\n",
			server_ip, server_port);
		exit(EXIT_FAILURE);
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully binded endpoint ip = %s port %u\n",
				server_ip, server_port);

	}

	if (listen(list_s, LISTENQ) < 0) {
		fprintf(stdout, "[ERROR] listen: Failed to listen on socket\n");
		exit(EXIT_FAILURE);
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully started to listen for new connections\n");

	}

	/*  Enter an infinite loop to respond
	   to client requests and echo input  */

	/*  Wait for a connection, then accept() it  */
	if ((conn_s = accept(list_s, NULL, NULL)) < 0) {
		fprintf(stdout,
			"[ERROR] accept: Failed to accept new connection\n");
		exit(EXIT_FAILURE);
	} else {
		if (verb_level == 3)
			fprintf(stdout,
				"[INFO] Successfully accepted a new connection\n");

	}
	struct protoent *tcp_proto = getprotobyname("tcp");
	printf("proto no %d\n", tcp_proto->p_proto);
	if (setsockopt
	    (conn_s, tcp_proto->p_proto, TCP_NODELAY, &tcp_nodelay,
	     sizeof(tcp_nodelay)) != 0) {
		fprintf(stdout,
			"[ERROR] setsockopt: Failed to set TCP_NODELAY socket option\n");
		exit(EXIT_FAILURE);
	}
	optval = 800 * 1024;
	if (setsockopt
	    (conn_s, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size,
	     sizeof(recv_buffer_size))!= 0) {
		fprintf(stdout,
			"[ERROR] setsockopt: Failed to set SO_RCVBUF socket option\n");
		exit(EXIT_FAILURE);
	}

	if (verb_level >= 1) {
		fprintf(stdout,
			"TCP_NODELAY = %d\nserver ip = %s\nserver port = %u\nrecv buffer size = %lu\nverbosity level = %d\n",
			tcp_nodelay, server_ip, server_port, recv_buffer_size,
			verb_level);

	}

	return conn_s;

}

void recv_traffic(int conn_s)
{

	fprintf(stdout, "Started receving traffic\n");
	int n = 0;
	while ((n += read(conn_s, recv_buffer, sizeof(recv_buffer))) > 0) {

		printf("tot primesc %d\n", n);
		memset(recv_buffer, 0, sizeof(recv_buffer));
	}
	if (n < 0) {
		printf("\n Read error \n");
	}
	//Writeline(conn_s, buffer, strlen(buffer));

	/*  Close the connected socket  */

	if (close(conn_s) < 0) {
		fprintf(stderr, "ECHOSERV: Error calling close()\n");
		exit(EXIT_FAILURE);
	} else {
		fprintf(stdout, "Finished receving traffic\n");
	}
}

void remove_connection()
{
	if (close(list_s) < 0) {
		fprintf(stderr, "ECHOSERV: Error calling close()\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{

	sanity_checks(argc, argv);
	int conn_s = establish_connection();
	recv_traffic(conn_s);
	remove_connection();
}
