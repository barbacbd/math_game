all: client 

# common definitions
CC = gcc

INC_DIR = ../common

CFLAGS = -I$(INC_DIR)

# find all header files from the include directory
_DEPS := $(shell find $(INC_DIR) -name '*.h')
DEPS = $(patsubst %, $(INC_DIR)/%,$(_DEPS))
OBJ = $(patsubst %, %, gameclient.o)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# stop make from doing anything file a file named clean
.PHONY: clean

# Remove:
# Object files
# Executable(s) - look at using $(ALL)
# remnants of old files (~)
clean:
	rm -f *.o *~ client
