#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXRECORD 256

static int read_cnt = 0;
static char *next_ptr;
static char record_buf[MAXRECORD];

/**
 * If the buffer is empty pull characters from the socket given the file descriptor.
 * @param fd - file descriptor of the socket to read from
 * @param ptr - pointer to buffer
 * @return -1 on error, 0 when EOF is reached, or 1 and the buffer is incremented to the next character
 */
ssize_t read_nextchar(int fd, char *ptr)
{
	if (read_cnt <= 0)
	{	/*buffer is empty so pre-fetch some from socket */
		do
		{ /* if interrupted by a caught signal, retry */
			read_cnt = read(fd, record_buf, sizeof(record_buf));
		}while (read_cnt < 0 && errno == EINTR);
		
		if (read_cnt < 0 && errno != EINTR)
			return -1;
		else if (read_cnt == 0)
			return 0;
		
		next_ptr = record_buf; /*buffer refilled. reset next_ptr */
	}
	
	read_cnt--;
	*ptr = *next_ptr++;
	return(1);
}

/***************************************************
Read a whole record (delimited by the delim character)
up to and including the delimitercharacter itself, but not to 
exceed a total of maxlen-1 characters. 
Returns number of successfully read characters
or -1 on an error. The errno variable is set accordingly
****************************************************/
ssize_t readrecord (int fd, void *vptr, size_t maxlen, char delim)
{
	ssize_t n, rc;
	char c, *ptr;
	
	ptr = vptr;
	for(n = 1; n < maxlen; n++)
	{
		rc = read_nextchar(fd, &c);
		if (rc == 1)
		{
			*ptr++ = c;
			if(c == delim)
				break; /* delimiter is also stored */
		}
		else if(rc == 0)
		{ /*EOF encountered, n-1 bytes were read */
			*ptr = 0; /*null terminate the callers buffer */
			return (n -1);
		}
		else
			return (-1); /* errno previously set by read() */
	}
	
	*ptr = 0;	/* null terminate the callers buffer */
	return (n); /* n counts the delim but not the \0 */
}


/**************************************************************************************/
ssize_t readrecordbuf(void **vptrptr)
{
	if (read_cnt)
		*vptrptr = next_ptr;
	return (read_cnt);
}

/*************************************************************************************/
ssize_t readline(int fd, void *ptr, size_t maxlen)
{
	return (readrecord(fd, ptr, maxlen, '\n'));
}

/**
 * Attempt to read n bytes from the socket represented with the file descriptor fd.
 * @param fd - file descriptor to read from
 * @param vptr - buffer that data will be added to.
 * @param n - nummber of bytes to read
 * @return - number of bytes actually read
 */
ssize_t readn (int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;
	
	ptr = vptr;
	nleft = n;
	while(nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) < 0)
		{
			if (errno == EINTR)
				nread = 0;
			else
				return (-1);
		}else if (nread == 0)
			break;
		
		nleft -= nread;
		ptr += nread;
	}
	return (n-nleft);
}

/**
 * Attempt to write n bytes to the socket with file descriptor fd
 * @param fd - file descriptor of the socket
 * @param vptr - string of data that we will attempt to write
 * @param n - number of bytes to attempt to write
 * @return - number of bytes written, -1 on error
 */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if((nwritten = write(fd, ptr, nleft)) <= 0)
		{
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0; /*and call write() again */
			else
				return -1; /* error */
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n);
}



