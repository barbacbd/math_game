CC = gcc
MYSQLCFLAGS = -I/usr/include/mysql -DBIG_JOINS=1 -fno-strict-aliasing -g
MYSQLLIBS = -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -ldl

testserver: gameserver.c readwrite.c passivesock.c queryFile.c
	$(CC) -o server gameserver.c readwrite.c passivesock.c $(MYSQLCFLAGS) queryFile.c $(MYSQLLIBS) -l pthread
	
	
