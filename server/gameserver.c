/*Brent Barbachem and Ryan Wolter */

/* 


    how to compile the server

    gcc -o testserver testserver2.c readwrite.c passivesock.c $(mysql_config --cflags) queryFile.c $(mysql_config --libs) -l pthread

    ./testserver


 */

#include <unistd.h>
#include <fcntl.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <netinet/in.h>

#define QLEN		32
#define BUFSIZE		4096
#define MAXSTRLEN	256
#define INROUND 1
#define AFTERROUND 0

#define ROUND_DUR 30

/* create a global object to collect statistics*/
struct statistics
{
	unsigned int clients_fin;   // Total finished clients
	unsigned int correct_total; // Total correct answers
	unsigned int wrong_total;   // Total wrong answers
	long long total_time;   // Total time the the clients took to answer
	long long fast_time;    // The current fastest time for the round
	char *leader;
	pthread_mutex_t st_mutex; // a mutex to protect an object of this struct
} roundStats;

/* a global object to share information between threads during a round */
struct roundInfo
{
	int roundStatus; 	// Either 1 for inside a round, or 0 for after
	char *question; 	// Pointer to the question
	char *answer; 		// Pointer to the answer
	long long globaltime;

	pthread_mutex_t ri_mutex; // a mutex to protect an object of this struct
} rI;

typedef struct sockaddr SA;

void * prstats(void * );
void * threadEntry( void * );
int passiveTCP(const char *service, int qlen);
void Round(int, char *);
void afterRound(int, char *);
void * masterFunc(void *);

void err_sys(const char * x);
void err_quit(const char * x);
long long current_timestamp();

/*-------------------------------------------------------------------
 * main - concurrent TCP server for Game service
 *--------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	pthread_t	th, master;
	char	*service = "ftp"; 	//service name or port number
	struct sockaddr_in fsin;	//address of client
	unsigned int alen;			//length of clients address
	int msock;					//master server socket
	int ssock;					//slave server socket
	char msg[MAXSTRLEN];		//used with snprintf
	int test = 0;

	switch(argc)
	{
	case 1:
		break;
	case 2:
		service = argv[1];
		break;
	default:
		snprintf(msg, MAXSTRLEN, "usage: %s [port]\n", argv[0]);
		err_quit(msg);
	}

	//Log all users out by setting status to 0
	logoutAll();

	memset((char *) &roundStats, 0, sizeof(roundStats)-sizeof(pthread_mutex_t));

	memset((char *) &rI, 0, sizeof(rI) - sizeof(pthread_mutex_t));

	msock = passiveTCP(service, QLEN);



	if(pthread_mutex_init(&roundStats.st_mutex, NULL))
		err_quit("pthread_mutex_init failed");

	if(pthread_mutex_init(&rI.ri_mutex, NULL))
		err_quit("pthread_mutex_init failed");

	if(pthread_create(&master, NULL, masterFunc, (void *) test))
		err_quit("pthread_create(server_thread) failed");


	while(1)
	{
		alen = sizeof(fsin);
		/*wait for text tcp echo client to connect*/
		ssock = accept(msock, (SA *) &fsin, &alen);
		if (ssock < 0)
		{
			if (errno == EINTR)
				continue;
			err_sys("accept");
		}
		/* create a thread to serve the client*/
		if(pthread_create(&th, NULL, threadEntry, (void *) ssock))
			err_quit("pthread_create(server_thread) failed"); 	
	}
}

/*
 * Gets the current time in milliseconds.
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
 * Spawns the master thread which maintains state information,
 * queries the database for new questions, and keeps track of
 * time and round leaders.
 */
void* masterFunc(void *vfd)
{
	char answer[20];
	char query[100];
	while (1)
	{
		// Retrieve the question and answer
		questionQuery(query, answer, sizeof(query), sizeof(answer));
		pthread_mutex_lock(&rI.ri_mutex);
			rI.globaltime = current_timestamp();
			rI.question = query;
			rI.answer = answer;
			rI.roundStatus = INROUND; // Round in progress
		pthread_mutex_unlock(&rI.ri_mutex);
		
		// Sleep while clients are answering
		while((current_timestamp() - rI.globaltime) < ROUND_DUR * 1000000)
		{
			sleep(1);
		}
		
		pthread_mutex_lock(&rI.ri_mutex);
			rI.roundStatus = AFTERROUND; //Not in round, after round
		pthread_mutex_unlock(&rI.ri_mutex);
		//Update the leader
		updatePoints(roundStats.leader, 5);
		
		//Sleep for 10 seconds (busy)
		sleep(10);
		
		//Round reset
		//(no need to use mutex because other threads are sleeping.)
			roundStats.clients_fin = 0;
			roundStats.correct_total = 0;
			roundStats.wrong_total = 0;
			roundStats.total_time = 0;
			roundStats.fast_time = 0;
			roundStats.leader = 0;	
	}
}

/*
 * Entry point for new connection threads.  Each client gets a 
 * thread to serve it.  This thread is responsible for 
 * interaction during the rounds, and updating the client on
 * the current state of the game.
 */
void* threadEntry(void *vfd)
{
	int fd;
	time_t start;
	char buf[BUFSIZE];
	char outbuf[BUFSIZE];
	int n;
	fd = (int ) vfd; /*only works if sizeof(int) <= sizeof(void *)*/

	int	saved = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, saved | O_NONBLOCK); // make fd non blocking
	pthread_detach(pthread_self()); //parent will not join this thread

	// Used to store a local copy of the username and password
	char username[43]; 
	char password[43];

	int i;
	char *token, *myquery;
	// Read the username and password from the client.
	do {
		n = readrecord(fd, (void *)buf, sizeof(buf), ';');
	} while(n <= 0); //busy wait
	char option = buf[0];

	int temp; //This is used for login status
	//Login
	//first character is either n or o
	switch(option)
	{
	case 'n': //New account
		buf[strlen(buf)-1] = '\0'; 	//This takes away the terminating
									//semicolon
		token = strtok(buf, ":");
		i = 0;
		while(token)
		{
			if (i == 1)
				snprintf(username, sizeof(username), "%s" , token);
			if (i == 2)
				snprintf(password, sizeof(password), "%s" , token);
			token = strtok(NULL, ":");
			i ++;
		}
		temp = loginNew(username, password);
		snprintf(outbuf, sizeof(outbuf), "n:%d;", temp);
		writen(fd, outbuf, strlen(outbuf));
		break;
	case 'o': //Old account
		buf[strlen(buf)-1] = '\0'; //take away the semi!
		token = strtok(buf, ":");
		i = 0;
		while(token)
		{
			if (i == 1)
				snprintf(username, sizeof(username), "%s" , token);
			if (i == 2)
				snprintf(password, sizeof(password), "%s" , token);
			token = strtok(NULL, ":");
			i ++;
		}
		temp = loginOld(username, password);
		snprintf(outbuf, sizeof(outbuf), "o:%d;", temp);
		writen(fd, outbuf, strlen(outbuf));
		break;
	default:
		snprintf(outbuf, sizeof(outbuf), "?:;");
		writen(fd, outbuf, strlen(outbuf));
		break;
	}

	Round(fd, username);
	printf("User has logged out: %s\n", username);
	close(fd);
	pthread_exit(NULL);
}

void Round(int fd, char* username)
{

	char *answer;
	char *query;
	char buf[BUFSIZE];
	char outbuf[BUFSIZE];
	int n; // Reading

	int i, userAns;
	char *token, *myquery;

	char screenbuf[BUFSIZE + 1];
	long long time, timed, temptime;

	int questionSent = 0;
	int answered = 0; // 0 means not yet answered


	while (1) // This happens once per round. It IS the round.
	{
		//Wait until the round has started
		while(rI.roundStatus == AFTERROUND)
		{
			sleep(1);
		}
		pthread_mutex_lock(&rI.ri_mutex);
			query = rI.question;
			answer = rI.answer;
		pthread_mutex_unlock(&rI.ri_mutex);

		//Write a question.
		snprintf(outbuf, sizeof(outbuf), "q:%s;", query);
		writen(fd, outbuf, strlen(outbuf));

		while (1)
		{
			//Get their answer
			n = readrecord(fd, (void *)buf, sizeof(buf), ';');
			if (n <= 0)
			{
				//No Answer yet, and it's too late.
				if (rI.roundStatus == AFTERROUND) // Not in a round
				{
					answered = 0;
					break;
				}
				else if (rI.roundStatus == INROUND) // In a round
				{
					//No response, try again.
					sleep(1);
				}
			}
			else
			{
				//Client has answered
				answered = 1;
				break;
			}
		}

		if (answered)
		{
			char option = buf[0];
			int answerInt = atoi(answer);
			//We got an answer back
			if (buf[0] == 'a' && buf[1] == ':')
			{
				// we have to seperate the a:answer:time that is sent
				i = 0;
				buf[strlen(buf) - 1] = '\0';
				token = strtok(buf, ":");
				while(token)
				{
					if (i == 1) 
					{
					    userAns = atoi(token);
					    if (answerInt == 0 && token[0] != '0')
					        userAns = -1;
					}
					if (i == 2) time = atoi(token);

					token = strtok(NULL, ":");
					i++;
				}
				//Test the correctness of the answer.
				if(answerInt == userAns)
				{
					//Update the database
					updatePoints(username, 5);
					updateCorrect(username);
                    
                    snprintf(outbuf, sizeof(outbuf), "f:You have answered correctly!\n\n;");
				    writen(fd, outbuf, strlen(outbuf));
                    //Update the global stats.  This is
				    //a critical section, so a mutex is 
				    //absolutely required.
					pthread_mutex_lock(&roundStats.st_mutex);
					roundStats.correct_total++; //update correct total for the round
					if(time < roundStats.fast_time || roundStats.fast_time == 0)
					{
						roundStats.fast_time = time;
						roundStats.leader = username;
					}
					pthread_mutex_unlock(&roundStats.st_mutex);

				}
				else
				{
					pthread_mutex_lock(&roundStats.st_mutex);
						roundStats.wrong_total++; //update the wrong total for the round
					pthread_mutex_unlock(&roundStats.st_mutex);
					
					snprintf(outbuf, sizeof(outbuf), 
					    "f:Sorry, you have answered incorrectly.\n\n;");
				    writen(fd, outbuf, strlen(outbuf));
				}
				updateAttempts(username);   //update the number of attempts for the user
				
				pthread_mutex_lock(&roundStats.st_mutex);
					roundStats.total_time += time;
					roundStats.clients_fin++;   //update clients finished
				pthread_mutex_unlock(&roundStats.st_mutex);

				answered = 1;
			}
			else if (buf[0] == 'e' && buf[1] == ':')
			{
				//Recieved an exit from the client.
				logout(username);
				return;
			}
			
			// Now we have answered, so keep printing the updates
			// until the round is over.
			while (rI.roundStatus == INROUND)
			{
				//Make a local copy of the roundStats
				pthread_mutex_lock(&roundStats.st_mutex);
					struct statistics *rs;
					rs = malloc(sizeof(struct statistics));
					memcpy(rs, &roundStats, sizeof(struct statistics));
				pthread_mutex_unlock(&roundStats.st_mutex);
				//change it to be like lab 9 where the information is a copy of the global
				int percent = rs->correct_total * 100 / rs->clients_fin;
				pthread_mutex_lock(&rI.ri_mutex);
				    int timeLeft = ROUND_DUR - 
				        (current_timestamp() - rI.globaltime) / 1000000;
				pthread_mutex_unlock(&rI.ri_mutex); 
				snprintf(outbuf, sizeof(outbuf), 
				    "r:\rFinished: %d   Correct: %d%%   Fastest = %.2f (sec)   Time left: %d  ;",
				    rs->clients_fin, percent, rs->fast_time/1000000.0, timeLeft);
			
				writen(fd, outbuf, strlen(outbuf));
				free(rs);
				sleep(1); // let the thread sleep for 1 seconds after sending the data
			}
		}
		//Print the after round statistics
		afterRound(fd, username);
		answered = 0;
	}	
	
}

/*
 * This function sends the statistics for the client's statistics,
 * global statistics for everyone, as well as says who was the fastest.
 */
void afterRound(int fd, char *username)
{
	char outbuf[BUFSIZE];

	//total points earned, number attempts, number correct, number incorrect
	int tpe, na, nc, nic;
	char **result; //used for the highscore query

	char * myquery = "SELECT * from userScore ORDER "
			"BY totalPointsEarned DESC LIMIT 5";
	int hold = numRet(myquery);

	result = malloc(hold * sizeof(char *));
	int i;
	for (i =0; i < hold; i++)
		result[i] = malloc(MAXSTRLEN * sizeof(char *));
	
	//This section loops through up to five users who are the top
	//scoring users of all time, and writes them to clients.
	highscoreQuery(result);
	int nextLocation = 0;
	char * hs_head;
	hs_head = "\n\nHigh Scores\n---------------------------------------\n";
	snprintf(&outbuf[nextLocation], sizeof(outbuf), "h:%s%s;", hs_head, result[0]);
	for( i = 1; i < hold; i++)
	{
		nextLocation = strlen(outbuf) - 1;
		snprintf(&outbuf[nextLocation], sizeof(outbuf), "\n%s;", result[i]);
	}
	char * hs_foot;
	hs_foot = "\n---------------------------------------\n\n";
	snprintf(&outbuf[strlen(outbuf) - 1], sizeof(outbuf), "%s;", hs_foot);
	writen(fd, outbuf, strlen(outbuf));
	
	//This gives individual statistics for clients.
    hs_head = "These are your statistics.\n"
        "---------------------------------------\n";
    hs_foot = "---------------------------------------\n";
	getuserStats(&tpe, &na, &nc, &nic, username);
	snprintf(outbuf, sizeof(outbuf), "u:%s%s%d\n%s%d\n%s%d\n%s%d\n%s\n;",
			hs_head, "Total Points Earned:          ", tpe,
			"Number of questions answered: ", na,
			"Number of correct answers:    ", nc,
			"Number of incorrect answers:  ", nic, hs_foot);
	writen(fd, outbuf, strlen(outbuf));

	//This gives the persistant statistics for all users combined
	hs_head = "These are the total player statistics.\n"
                "---------------------------------------\n";
    hs_foot = "---------------------------------------\n";
	getStats(&tpe, &na, &nc, &nic);
	snprintf(outbuf, sizeof(outbuf), "o:%s%s%d\n%s%d\n%s%d\n%s%d\n%s\n;",
			hs_head, "Total Points Earned:          ", tpe,
			"Number of questions answered: ", na,
			"Number of correct answers:    ", nc,
			"Number of incorrect answers:  ", nic, hs_foot);
	writen(fd, outbuf, strlen(outbuf));

	//Say who was the fastest, if any.
	if (roundStats.fast_time == 0)
	    snprintf(outbuf, sizeof(outbuf), 
					    "o:No one answered correctly this round.\n;");
	else
	{
	    snprintf(outbuf, sizeof(outbuf), 
			"o:The winner for the round is... %s\n;", roundStats.leader);
		
	}
	writen(fd, outbuf, strlen(outbuf));
}
