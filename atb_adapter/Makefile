GCC=g++
CFLAGS= -g -rdynamic -Wall -std=c++11

PRG=libatb_adapter.a

SRC = $(wildcard *.cpp)
OBJS = $(SRC:%.cpp=%.o)

#INC = -I ../atb_sak/src/ -I ../http/output/include/
#LIBS = -L ../http/deps/json-cpp/output/lib/  -L /usr/lib -L ../http/output/lib/ -std=c++11 -lpthread  -lmysqlcppcnn -ljson_libmt -lsimpleserver
INC = -I ../atb_sak/src/
LIBS = -L ../atb_sak/ -lpthread -latb_sak

all: $(PRG)

$(PRG):$(OBJS)
	ar -rcs $(PRG) *.o

%.o: %.cpp
	$(GCC) -c  $(CFLAGS) $(INC) $(LIBS) $< -o $@

tags:
	ctags -R *

clean :
	rm -f $(OBJS) $(TARGET) $(PRG)

