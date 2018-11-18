/* passivesock.c - passivesock */
/* Modefied by Brent from original Comer's code */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void err_sys(const char* x)
{
	fflush(stderr);
	perror(x);
	exit(1);
}

void err_quit(const char * x)
{
	fflush(stderr);
	fputs(x, stderr);
	exit(1);
}

#define MAXSTRLEN 256
#define MAX_BUF 10000
typedef struct sockaddr SA ;

unsigned short portbase = 50000; /*port base for non-root servers */

/*__________________________________________________________________
* passivesock - allocate and bind a server socket using TCP or UDP
*___________________________________________________________________
*/
/*
Arguments:
	service - service associated with the desired port
	transport - transport protocol to use ("tcp" or "udp")
	qlen - maximum server request queue length

*/

int passivesock(const char* service, const char *transport, int qlen)
{
	struct servent *pse;                /*pointer to service information entry*/
	struct protoent ppe;                /*pointer to protocol information */
	struct protoent *result; 
	struct sockaddr_in sin;             /*internet endpoint address */
	int s, type,count, proto, error;    /*socket descriptor and socket type*/
	char msg[MAXSTRLEN];                /*used with snprintf */
	struct addrinfo hints, *res;        /*used for the getaddrinfo */ 
	char buff[MAX_BUF];
	int buflen = 1024;
	
	memset ((void *) &sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	
	/*map service to port number*/
	/*getservbyname returns ptr to static struct not thread safe */
	/* should use getaddrinfo instead */
	if ((error = getaddrinfo(NULL, service, &hints, &res)) == 0)
	{
		sin.sin_port = htons(portbase + ntohs( (unsigned short)
		        (((struct sockaddr_in *)(res->ai_addr))->sin_port)));
	}
	else
	{	/*maybe service is a valid string of numerals*/
		sin.sin_port = htons((unsigned short) atoi(service));
		if (sin.sin_port == 0)
		{
			snprintf(msg, MAXSTRLEN, "can't get %s service entry\n", service);
			err_quit(msg);
		}
	}
	
	/*map protocol name to number 
		this is not thread safe as it returns results in static variables
		chand the code to use getprotobyname_r instead
	*/
	count = 0;
	/* try the current size of the buffer and see if it needs to be increased*/
	do {
	    proto = getprotobyname_r(transport, &ppe, buff, buflen, &result);
	    if (proto == ERANGE)
	    {
	        if(count == 0)
	            printf("ERANGE! retrying with a larger buffer\n");
	        count++;
	        buflen++;
	        
	        if (buflen>MAX_BUF){
	            printf("Exceeded buffer limit %d \n", MAX_BUF);
	            exit(EXIT_FAILURE);
	        }
	    }
	} while(proto == ERANGE);
	
	
	if (proto < 0)
	{
	    snprintf(msg, MAXSTRLEN, "can't get %s protocol entry\n", transport);
		err_quit(msg);
	}
	
	/* use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else if(strcmp(transport, "tcp") == 0)
		type = SOCK_STREAM;
	else
	{
		snprintf(msg, MAXSTRLEN, "passivesock() not-supported transport %s\n", transport);
		err_quit(msg);
	}
	
	/*Allocate a socket */
	s = socket(AF_INET, type, ppe.p_proto); /*changed -> to . */
	if (s < 0)
		err_sys("passivesock() can't create socket");
	
	/*bind the socket*/
	if (bind(s, (SA *)&sin, sizeof(sin)) < 0)
	{
		snprintf(msg, MAXSTRLEN, "passivesock() can't bind to %s port", service);
		err_sys(msg);
	}
	
	/* if tcp server, start listening at the socket */
	if ((type == SOCK_STREAM) && (listen(s, qlen) < 0))
	{
		snprintf(msg, MAXSTRLEN, "passivesock() can't listen on %s port\n", service);
		err_sys(msg);
	}
	
	return s;
}

/*___________________________________________________________________
passiveUDP - create a passive socket for use in a UDP server
*___________________________________________________________________
*/
/*
Arguments
	service - service associated with the desired port
*/
int passiveUDP(const char * service)
{
	return passivesock(service, "udp", 0);
}

/*___________________________________________________________________
passiveTCP - create a passive socket for use in a TCP server
*___________________________________________________________________
*/
/*
Arguments
	service - service associated with the desired port
	qlen - maximum server request queue length
*/
int passiveTCP(const char * service, int qlen)
{
	return passivesock(service, "tcp", qlen);
}
