FLAGS = -Wall -Wextra -MMD -lpthread -lncurses

all : server.out  client.out

-include *d

%.out : %.c
	gcc -o $@ $^ $(FLAGS)

.PHONY:clean
clean :
	rm *.o 
