/* connectsock.c - connectsock */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void err_sys(const char *x)
{
	fflush(stderr);
	perror(x);
	exit(1);
}

void err_quit(const char *x)
{
	fflush(stderr);
	fputs(x, stderr);
	exit(1);
}

/*****************************************************************************
Connectsock - allocate & connect a socket using TCP or UDP



Arguements:
	host		- name of the host to which the connection is desired
	service		- service associated with the desired port
	transport	- name of transport protocol to use (tcp or udp)

********************************************************************************/

#define MAXSTRLEN 256

int connectsock(const char *host, const char *service, const char *transport)
{
	struct protoent	*ppe; 	/* pointer to protocol information entry */
	struct addrinfo	*ainfo; /*pointer to address information entry */
	struct addrinfo	hints, 	/*used to filter gai search results */
			*rp;		/*used to iterate gai search results */
	//struct servent *pse		/*pointer to service information entry */
	int s, type;			/*socket descriptor and socket type */
	char msg[MAXSTRLEN];
	int errCode;

	/*Map transport to protocol number */
	ppe = getprotobyname(transport);
	if (ppe == NULL)
	{
		snprintf(msg, MAXSTRLEN, "can't get %s protocol entry\n", transport);
		err_quit(msg);
	}

	/*use transport name to choose a socket type */
	if(strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else if(strcmp(transport, "tcp") == 0)
		type = SOCK_STREAM;
	else
	{
		snprintf(msg, MAXSTRLEN, "connectsock() non-supported transport %s", transport);
		err_quit(msg);
	}

	/* allocate a socket */
	s = socket(AF_INET, type, ppe->p_proto);
	if (s < 0)
		err_sys("conectsock() can't create socket");

	/*prepare hints to filer results of getaddrinfo() */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = type;
	hints.ai_flags = 0;
	hints.ai_protocol = ppe->p_proto;

	/* map host name to ip address */
	errCode = getaddrinfo(host, service, &hints, &ainfo);
	if(errCode)
	{
		snprintf(msg, MAXSTRLEN, "getaddrinfo failed: %s\n", gai_strerror(errCode));
		err_quit(msg);
	}
	else
	{
		for(rp = ainfo; rp != NULL; rp = rp->ai_next)
		    if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
				break;

		if(rp == NULL)
			err_quit("Could not connect; No gai() address succeeded\n");
	}
	return s;
}

/********************************************************************
connectUDP - connect to a specified UDP service on a specified host
********************************************************************/
/*
*Arguments:
*	host 	 - name of host to which the connection is desired
*	services - service associated with the desired port 
*/ 
int connectUDP(const char *host, const char *service)
{
	return connectsock(host, service, "udp");
}

/********************************************************************
connectTCP - connect to a specified TCP service on a specified host
********************************************************************/
/*
*Arguments:
*	host 	 - name of host to which the connection is desired
*	services - service associated with the desired port 
*/
int connectTCP(const char *host, const char *service)
{
	return connectsock(host, service, "tcp");
}
