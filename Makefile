#Makefile

CC = gcc
CFLAGS = -W -Wall -pthread
TARGET = tfind
DTARGET = tfind_debug
OBJECTS = tfind.c
all = $(TARGET)
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^
$(DTARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -DDEBUG -o $@ $^
clean :
	rm tfind tfind_debug tasks results
