######################################
#
#######################################
#source file
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

#target you can change test to what you want
TARGET  := atb

#compile and lib parameter
CC      := gcc
CXX     := g++

LIBS    := -lpthread -lmysqlclient -latb_sak -lhttp_parser -lmultipart_parser -latb_http -latb_adapter -lwebsockets -latb_wss
LDFLAGS := -L /usr/lib \
	   -L /usr/local/lib \
	   -L /usr/lib64/mysql \
	   -L atb_sak/ \
           -L atb_http/deps/http-parser/ \
	   -L atb_http/deps/multipart-parser-c/ \
	   -L atb_http/output/lib/ \
	   -L atb_adapter/ \
           -L atb_wss/

INCLUDE := -I /usr/local/include \
	   -I atb_sak/src/ \
	   -I atb_http/deps/http-parser/ \
	   -I atb_http/deps/multipart-parser-c/ \
	   -I atb_http/output/include/ \
	   -I atb_adapter/ \
	   -I atb_wss/

CFLAGS  := -g -rdynamic -Wall
CXXFLAGS:= $(CFLAGS)
CXXFLAGS+= -std=c++11
CXXFLAGS+= $(INCLUDE)

#i think you should do anything here
.PHONY : everything objs clean veryclean rebuild

everything : $(TARGET)

all : $(TARGET)

objs : $(OBJS)

rebuild: veryclean everything


clean :
	rm -rf *.o
	rm $(TARGET)

veryclean : clean
	rm -rf $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
