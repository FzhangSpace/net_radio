CFLAGS+=-pthread -I../include
LDFLAGS+=-pthread

all:server

server: main.o thr_channel.o thr_list.o medialib.o mytbf.o
	$(CC) $(LDFLAGS) -o $@ $^

#server: main.o  medialib.o mytbf.o thr_channel.o
	
clean:
	rm -rf *.o server
