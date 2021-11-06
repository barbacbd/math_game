#ifdef _WIN32
// Include all Windows headers if this is the windows platform
#include <winsock2.h>
#include <Ws2tcpip.h>
#else // APPLE or LINUX
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define MAXSTRLEN 256
#define MAX_BUF 10000

typedef struct sockaddr SA ;

unsigned short portbase = 50000; /*port base for non-root servers */

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
int connectClientSock(const char *host, const char *service, const char *transport)
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


/*****************************************************************************
connectServerSock - allocate and bind a server socket using TCP or UDP



Arguments:
	service - service associated with the desired port
	transport - transport protocol to use ("tcp" or "udp")
	qlen - maximum server request queue length

********************************************************************************/
int connectServerSock(const char* service, const char *transport, int qlen)
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

/**
 * Wrapper function for connecting a socket, where the protocol can ONLY be UDP. As with the
 * connectsock() function, the user must pass in the string representation of the host and service.
 * @param host - name of host to which connection is desired
 * @param service - service associated with the desired port
 * @return socket descriptor number
 */
int connectClientUDP(const char *host, const char *service)
{
    return connectClientSock(host, service, "udp");
}

/**
 * Wrapper function for connecting a socket, where the protocol can ONLY be TCP. As with the
 * connectsock() function, the user must pass in the string representation of the host and service.
 * @param host - name of host to which connection is desired
 * @param service - service associated with the desired port
 * @return socket descriptor number
 */
int connectClientTCP(const char *host, const char *service)
{
	return connectClientSock(host, service, "tcp");
}


/**
 * Wrapper function for connecting a socket, where the protocol can ONLY be UDP. As with the
 * connectServerSock() function, the user must pass in the string representation of the service.
 * @param host - name of host to which connection is desired
 * @param service - service associated with the desired port
 * @return socket descriptor number
 */
int connectServerUDP(const char * service)
{
	return connectServerSock(service, "udp", 0);
}

/**
 * Wrapper function for connecting a socket, where the protocol can ONLY be TCP. As with the
 * connectServerSock() function, the user must pass in the string representation of the service.
 * @param service - service associated with the desired port
 * @return socket descriptor number
 */
int connectServerTCP(const char * service, int qlen)
{
	return connectServerSock(service, "tcp", qlen);
}
