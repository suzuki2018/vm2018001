GCC=g++
CFLAGS= -g -rdynamic -Wall -std=c++11

PRG=libatb_wss.a

SRC = $(wildcard *.cc)
OBJS = $(SRC:%.cc=%.o)

#INC = -I ../http/deps/json-cpp/include/ -I ../http/output/include/ -I ../adapter_server/ -I /usr/local/include
#LIBS = -L ../http/deps/json-cpp/output/lib/  -L /usr/lib -L /usr/local/lib -L ../http/output/lib/ -std=c++11 -lpthread  -lmysqlcppcnn -ljson_libmt -lwebsockets -lsimpleserver -ladapterserver
INC = -I ../atb_sak/src/ -I ../atb_adapter/ -I /usr/local/include
LIBS = -L ../atb_sak/ -L /usr/lib -L /usr/local/lib  -lpthread  -lmysqlcppcnn -lwebsockets -latb_sak -latb_adapter

all: $(PRG)

$(PRG):$(OBJS)
	 ar -rcs $(PRG) *.o

%.o: %.cc
	$(GCC) -c  $(CFLAGS) $(INC) $(LIBS) $< -o $@

tags:
	ctags -R *

clean :
	rm -f $(OBJS) $(TARGET) $(PRG)

