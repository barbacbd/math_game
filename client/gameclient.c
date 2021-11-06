/* Author - Brent Barbachem and Ryan Wolter*/


/*
Here is how to compile the client


gcc -o gameclient tcpgameclient.c readwrite.c connectsock.c -l pthread

./gameclient localhost 50021

 */
#include <sys/types.h>
#include <sys/time.h> // for time
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include "../common/socket.h"
#include "../common/readwrite.h"

//State options
#define UNANSWERED 0
#define ANSWERED 1
#define AFTER_ROUND 2
#define LOGGING_OUT 3

#define LINELEN 256
#define BUFSIZE 4096


int	connectTCP(const char *host, const char *service);
int TCPgame(const char *host, const char *service);
int waitForUserInput(int fd);
void Play(int fd);
void err_sys(const char* x);
ssize_t readrecord(int fd, void *vptr, size_t maxlen, char delim);
void myHandler(int dummy);

int currentState; //Global state information
int answered; //Global answered status
long long globalTime; //Time
int globalfd; //File Descriptor

/*-------------------------------------------------------------------

 * main - TCP game client
 *--------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	char *host = "localhost"; 	    /* default if none provided */
	char *service = "ftp";		    /* default service name */

	switch(argc)
	{
	case 1:
		break;
	case 3:
		service = argv[2];
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: %s [host[port]]\n", argv[0]);
		exit(1);
	}

	TCPgame(host, service);
	exit(0);
}

/*

 * This function handles the ctrl^c and exits gracefully
 * after telling the server to log out.
 */
void myHandler(int dummy)
{
	char buf[BUFSIZE];
	snprintf(buf, sizeof(buf), "e:exit;");
	writen(globalfd, buf, strlen(buf));
	printf("Logging out...\nThanks for playing!\n");
	exit(1);
}

/*
 * This function strips the newline character off the
 * end of a char*
 */
void stripnl(char * line)
{
	size_t ln = strlen(line) - 1;
	if (line[ln] == '\n')
		line[ln] = '\0';
}

/*

 * This function gets the current time in milliseconds.
 */
long long current_timestamp() {
	struct timeval te; 
	gettimeofday(&te, NULL); // get current time
	// caculate milliseconds
	long long milliseconds = te.tv_sec*1000000LL + te.tv_usec; 
	// printf("milliseconds: %lld\n", milliseconds);
	return milliseconds;
}

/*
 * This function reads all data from the server, as well
 * as dictate the flow of the game.
 */
void Play(int fd)
{
	char buf[BUFSIZE];
	int n;
    long long lastTime = 0;
	signal(SIGINT, myHandler);

	while (1) //Forever, read and ask for input
	{
		n = readrecord(fd, (void *)buf, sizeof(buf), ';');
		if (n > 0)
		{
			char firstChar = buf[0];
			buf[strlen(buf)-1] = '\0'; //take away the semi!
			//The &buf[2] basically ignores the first two characters
			//which are the option and a : delimeter.  For example,
			//q:What is 2 * 3?'
			lastTime = current_timestamp();
			switch(firstChar)
			{
			case 'q': //Received a question
				printf("%s\n", &buf[2]);
				currentState = UNANSWERED;
				globalTime = current_timestamp();
				break;
			case 'r'://Received a status update
				printf("\r%s", &buf[2]);
				currentState = ANSWERED;
				fflush(stdout);
				break;
			case 'h'://Received a high score
				printf("%s", &buf[2]);
				currentState = AFTER_ROUND;
				break;
			case 'u'://Received a personal statistic
				printf("%s", &buf[2]);
				currentState = AFTER_ROUND;
				break;
			case 'o'://Received a global statistic
				printf("%s", &buf[2]);
				currentState = AFTER_ROUND;
				break;
			case 'f'://Received feedback if the question was correct.
			    printf("%s", &buf[2]);
			    //fflush(stdout);
                break;
			}	 
			fflush(stdout);
		}
		else
		{
			//Only ask for input if we are in the correct state.
			if (currentState == UNANSWERED || currentState == AFTER_ROUND)
			    waitForUserInput(fd);
			
			long long now = current_timestamp();
			if ((now - lastTime) > 40 * 1000000 && lastTime != 0)
			{
			    //Server unresponsive.  Quit.
			    printf("\n\nServer is unresponsive.\nQuiting...\n");
			    return;
			}
		}
	}	
	 
}

/*

 * This function is a non-blocking call that polls
 * the stdin for data.  If it is empty, then
 * it simply returns.  Otherwise, it handles the data

 * appropriately, writing the the server if needed.
 */
int waitForUserInput(int fd)
{
	char outbuf[BUFSIZE];
	char answerbuf[BUFSIZE];
	long long time0, time1, deltatime;
	struct pollfd stdin_poll;
	stdin_poll.fd = STDIN_FILENO;
	stdin_poll.events = POLLRDNORM;

	if (poll(&stdin_poll, 1, 0) == 1) 
	{
		/* Data waiting on stdin. Process it. */

		time0 = globalTime; //Get the time when a question was received
		fgets(answerbuf, sizeof(answerbuf), stdin);
		if (currentState == AFTER_ROUND)
		{
			printf("Please wait until the next round...\n");
			return 0;
		}
		if (currentState == ANSWERED)
		{
			printf("You have already answered...\n");
			return 0;
		}
		//Get the time when an answer was sent
		time1 = current_timestamp();
		//Prepare and write the answer to the server.
		stripnl(answerbuf);
		/* change any colon or semicolon to a period to avoid
		confusion on the server side, since the server delimates
		by colons and semicolons*/
		int i;
		for (i = 0; i < strlen(answerbuf); i++)
		{
			if(answerbuf[i] == ':' || answerbuf[i] == ';')
			{
				memcpy(&answerbuf[i],".", 1);
			}
		}
		
		deltatime = time1 - time0;
		snprintf(outbuf, sizeof(outbuf), "a:%s:%lld;", answerbuf, 
				deltatime); 
		writen(fd, outbuf, strlen(outbuf));
		currentState = ANSWERED;
	}
}

/*
 * This function logs the user into the server, as well as
 * start the connection.
 */
int TCPgame(const char *host, const char *service)
{
	int 	s, n;					/*socket descriptor, read count*/
	char    temp[5];                /*holds the answer to the first question*/
	char buf[LINELEN+1]; /*buffer for one line of text */

	s = connectClientTCP(host, service);
	globalfd = s;
	
	printf("Welcome to the TCP Math Game!\n\n"
			"Is this your first login [yes/no]? ");
	fgets(temp, sizeof(temp), stdin);
	temp[4] = '\0';

	char user[100];
	char password[100];
	char * newold = "old";
	char * new = "";
	char login[205];
	if(temp[0] == 'y')
	{
		newold = "new";
		new = "new ";
	}
	printf("Please enter your %susername: ", new);
	fgets(user, sizeof(user), stdin);
	printf("Please enter your %spassword: ", new);
	fgets(password, sizeof(password), stdin);

	stripnl(user);
	stripnl(password);

	/* change an colon or semicolon to a period
	to avoid confusion on the server side, since
	we use colon and semicolon as delimiters*/
	int i, changed = 0;
	for (i = 0; i < strlen(user); i++)
	{
		if(user[i] == ':' || user[i] ==';')
		{
			memcpy(&user[i], ".", 1);
			changed = 1;
		}
	}
	
	for (i = 0; i < strlen(password); i++)
	{
		if(password[i] == ':' || password[i] ==';')
		{
			memcpy(&password[i], ".", 1);
			changed = 1;
		}
	}
	
	if (changed)
	{
		printf("\nYour username or password contained bad characters.\nUsername: %s\tPassword: %s\n", user, password);
	}
	
	snprintf(login, sizeof(login), "%s:%s:%s;", newold, user, password);

	writen(s, login, strlen(login));
	printf("\n");

	//Receive a response from the server indicating
	//if the login was successful.
	n = readrecord(s, buf, BUFSIZE, ';');

	if (buf[0] == 'n' && buf[1] == ':')
	{
		buf[strlen(buf) - 1] = '\0';
		switch(atoi(&buf[2]))
		{
		case 0:
			printf("Login Failure: This username is already taken.\n");
			exit(1);
			break;
		case 1:
			break;
		default:
			break;
		}
	}
	if (buf[0] == 'o' && buf[1] == ':')
	{
		buf[strlen(buf) - 1] = '\0';
		switch(atoi(&buf[2]))
		{
		case -1:
			printf("Login Failure: Incorrect Username/Password.\n");
			exit(1);
			break;
		case 0:
			printf("Login Failure: This user is already logged in.\n");
			exit(1);
			break;
		case 1:
			break;
		default:
			break;
		}
	}   
	printf("Please be patient.\nThe game will start soon...\n\n");

	// Set the socket to non-blocking
	int	saved = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, saved | O_NONBLOCK);

	Play(s);
	exit(0);
}
