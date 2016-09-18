CFLAGS+=-I../include -Wall

all: client

client:main.o
	gcc $(LDFLAGS) -o $@ $^

clean:
	rm -rf client *.o 


