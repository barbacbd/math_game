/* Simple C program that connects to MySQL Database server*/
#include <mysql/mysql.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <netinet/in.h>

/*********************************************************
*
* sudo apt-get install mysql
*
* sudo apt-get install libmysqlclient-dev
*
* This next command will compile the program and 
* produce an executable named output-file
*
* gcc -o output-file $(mysql_config --cflags) queryFile.c $(mysql_config --libs)
**********************************************************
*/



#define MAX 40472 
/*Total entries in the questionInfo table*/

#define MAXSTRLEN 60

/*------------------------------------------------------------
* randomQuestion - randomly pick a number between the maximum
					entries in the table and 1.
* return -
					the number
*
*-------------------------------------------------------------
*/
int randomQuestion()
{    
    return (random() % (MAX + 1));
}


/*-------------------------------------------------------------
* connection - the function is used to establish a connection
*				to the database. It saves code due to repetition.
*
* returns -
*				MYSQL struct that has the connection information
*--------------------------------------------------------------
*/
MYSQL * connection( MYSQL *conn, MYSQL_RES *res) 
{
   	char *server = "localhost";
   	char *user = "root";
   	char *password = "password";
   	char *database = "project";
   	conn = mysql_init(NULL);
   
   	/* Connect to database */
   	if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) 
   	{
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   	}
   	return conn;
}


/*-------------------------------------------------------------
* Query - 	The function is used to query the database
*				It saves code due to repetition.
*
* returns -
*				MYSQL_Res struct that has the query information
*--------------------------------------------------------------
*/
MYSQL_RES * Query(MYSQL *conn, MYSQL_RES *res, char * query)
{
    /* send SQL query */
   	if (mysql_query(conn, query))
   	{
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   	}
   
   	res = mysql_store_result(conn);
   	return res;
}


/*-------------------------------------------------------------
* questionQuery - the function is used to query the database and
*					get a question. The question is stored into 
*					the parameter, question, that is passed in.
*--------------------------------------------------------------
*/
void questionQuery(char *question, char * answer, int sizeone, int sizetwo)
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char query[200];
   
   	conn = connection(conn, res);   
   
   	int number = randomQuestion();
   	snprintf(query, sizeof(query), "%s%s%d", "SELECT op_one, operator, op_two,",
   	" answer FROM QuestionInfo WHERE questionNumber = ", number);
   
   	res = Query(conn, res, query);
   
   	while ((row = mysql_fetch_row(res)) != NULL)
  	{
      snprintf(question, sizeone, "%s%s%s%s%s%s%s", "What is ", row[0],
            " ", row[1], " ", row[2], "?");
      snprintf(answer, sizetwo, "%s", row[3]);
   	}
    
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*-------------------------------------------------------------
* numRet - the function queries the database to find the number
*			of results returned
*
* returns -
*				number of results returned
*--------------------------------------------------------------
*/
int numRet(char *query)
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;

   	conn = connection(conn, res);
   	res = Query(conn, res, query);
   	return mysql_num_rows(res);
}      


/*-------------------------------------------------------------
* highscoreQuery - the function is passed a 2d array. The 
*					function will store the result of the 
* 					query into the array. It will hold the 
*					top 5 players based on their score.
*--------------------------------------------------------------
*/
void highscoreQuery(char **queryArray)
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *query = "SELECT * from userScore ORDER BY totalPointsEarned DESC LIMIT 5";
    int numRet;
   
    conn = connection(conn, res);   
    res = Query(conn, res, query);
    numRet = mysql_num_rows(res);
    int i;
    for (i =0; i < numRet; i++)
        queryArray[i] = malloc(MAXSTRLEN * sizeof(char *));
    
    i = 0;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      snprintf(queryArray[i], MAXSTRLEN, "%s%s%s", row[0], " - ", row[1]);
      i++;
    }
    
    /* free the memory and close the connection*/ 
    mysql_free_result(res);
    mysql_close(conn);   
}


/*-------------------------------------------------------------
* loginOld - the function is used to attempt to log the user
*			into the system. The user will state whether they 
*			are a returning or new user and this function is
*			used for returning users.
*
* returns -
*				1 - Successful Log in
*				0 - Already Logged in
*				-1 - Login Error
*--------------------------------------------------------------
*/
int loginOld(char *user, char *pass)
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%s%s%s%s", 
    	"SELECT * FROM userpass WHERE username = '", user, 
   	 	"' AND password = '", pass, "'");
 
   	res = Query(conn, res, myquery); 
 
   	//res = mysql_store_result(conn);
    if (mysql_num_rows(res))
    {
        while ((row = mysql_fetch_row(res)) != NULL) 
        {
            if(atoi(row[2]))
                return 0;
            else
            {
                snprintf(myquery, sizeof(myquery), "%s%s%s%s%s", 
                    "UPDATE userpass SET status = 1 WHERE username = '", user, 
                    "' AND password = '", pass, "'");
                
                res = Query(conn, res, myquery);
				return 1;
            }
        }
    }
    else
    {
        return -1;
    }
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*-------------------------------------------------------------
* loginNew - the function is used to attempt to log the user
*			into the system. The user will state whether they 
*			are a returning or new user and this function is
*			used for new users.
*
* returns -
*				1 - Successful Log in and new database entry
*				0 - Login Error
*--------------------------------------------------------------
*/
int loginNew(char *user, char *pass)
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%s%s", 
    	"SELECT * FROM userpass WHERE username = '", user,
    	"'");
 
   	res = Query(conn, res, myquery); 
 

 
   	//res = mysql_store_result(conn);
    if (!mysql_num_rows(res))
    {
         snprintf(myquery, sizeof(myquery), "%s%s%s%s%s", 
            "INSERT INTO userpass VALUES('", 
                user, "','", pass,"',1)");
                
        res = Query(conn, res, myquery);
		return 1;
    }
    else
    {
        return 0;
    }
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*-------------------------------------------------------------
* logout - the function is used to log the user out of the 
* 			game by changing their status from 1 to 0.
*--------------------------------------------------------------
*/
void logout(char *user)
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%s%s", 
    "UPDATE userpass SET status = 0 WHERE username = '", user, "'");
 
   	res = Query(conn, res, myquery); 

   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*-------------------------------------------------------------
* logoutAll - the function is used to log all users out of the 
* 				game by changing their status from 1 to 0.
*--------------------------------------------------------------
*/
void logoutAll()
{
   	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s", 
    "UPDATE userpass SET status = 0");
 
   	res = Query(conn, res, myquery); 

   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*----------------------------------------------------------------
* updatePoints - the function is used to update the user's 
*               totalPointsEarned column of the table.
*
*------------------------------------------------------------------
*/
void updatePoints(char *user, int points)
{
	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%d%s%s%s", 
    "UPDATE userScore SET totalPointsEarned = totalPointsEarned + ", points, 
    " WHERE username = '", user, "'");
 
   	res = Query(conn, res, myquery);
   	
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}

/*----------------------------------------------------------------
* updateAttempts - the function is used to update the user's 
*               attempts column of the table.
*
*------------------------------------------------------------------
*/
void updateAttempts(char *user)
{
	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%d%s%s%s", 
    "UPDATE userScore SET numberAttempts = numberAttempts + ", 1, 
    " WHERE username = '", user, "'");
 
   	res = Query(conn, res, myquery);
   	
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}

/*----------------------------------------------------------------
* updateCorrect - the function is used to update the user's 
*               correct answers column of the table.
*
*------------------------------------------------------------------
*/
void updateCorrect(char *user)
{
	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   
   	conn = connection(conn, res); 
   
   	snprintf(myquery, sizeof(myquery), "%s%d%s%s%s", 
    "UPDATE userScore SET numberCorrect = numberCorrect + ", 1, 
    " WHERE username = '", user, "'");
 
   	res = Query(conn, res, myquery);
   	
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*----------------------------------------------------------------
* getStats - the function is used to get the statistics for the 
*               entire game.
*
*------------------------------------------------------------------
*/
void getStats(int *tpe, int *na, int *nc, int *nic)
{
	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   	
   	conn = connection(conn, res); 
   
    snprintf(myquery, sizeof(myquery), "%s%s%s","SELECT sum(totalPointsEarned),"
    ," sum(numberAttempts), sum(numberCorrect), (sum(numberAttempts) - ",
    "sum(numberCorrect)) as numberIncorrect FROM userScore");
 
   	res = Query(conn, res, myquery);
   	
   	while ((row = mysql_fetch_row(res)) != NULL) 
    {
        *tpe = atoi(row[0]);
        *na = atoi(row[1]);
        *nc = atoi(row[2]);
        *nic = atoi(row[3]);
   	}
   	
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}


/*----------------------------------------------------------------
* getuserStats - the function is used to get the statistics for the 
*               user that is entered.
*
*------------------------------------------------------------------
*/
void getuserStats(int *tpe, int *na, int *nc, int *nic, char *user)
{
	MYSQL *conn;
   	MYSQL_RES *res;
   	MYSQL_ROW row;
   	char myquery[200];
   	
   	conn = connection(conn, res); 
   
    snprintf(myquery, sizeof(myquery), "%s%s%s%s%s","SELECT totalPointsEarned,"
    ," numberAttempts, numberCorrect, (numberAttempts - ",
    "numberCorrect) as numberIncorrect FROM userScore WHERE username = '",
    user, "'");
 
   	res = Query(conn, res, myquery);
   	
   	while ((row = mysql_fetch_row(res)) != NULL) 
    {
        *tpe = atoi(row[0]);
        *na = atoi(row[1]);
        *nc = atoi(row[2]);
        *nic = atoi(row[3]);
   	}
   	
   	/* free the memory and close the connection*/ 
   	mysql_free_result(res);
   	mysql_close(conn);
}
   
