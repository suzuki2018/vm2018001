######################################
#
##
######################################
  
#target you can change test to what you want
#共享库文件名，lib*.a
TARGET  := libatb_sak.a
  
#compile and lib parameter
#编译参数
CC      := gcc
AR      = ar
RANLIB  = ranlib
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -I./src
CFLAGS  := -g -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H

  
#i think you should do anything here
#下面的基本上不需要做任何改动了

DIRS := $(shell find . -maxdepth 3 -type d)

#source file
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
#SOURCE  := $(wildcard *.c) $(wildcard *.cpp) $(wildcard ./src/cfg/*.cpp) $(wildcard ./src/log/*.cpp) $(wildcard ./src/thread/*.cpp) $(wildcard ./src/mysql/*.cpp ./src/json/*.cpp)
SOURCE = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
OBJFILES = $(foreach dir,$(DIRS),$(wildcard $(dir)/*.o))  
.PHONY : everything objs clean veryclean rebuild
  
everything : $(TARGET)
  
all : $(TARGET)
  
objs : $(OBJS)
  
rebuild: veryclean everything
                
clean :
	rm -fr $(OBJFILES) *.a
    
veryclean : clean
	rm -fr $(TARGET)
  
$(TARGET) : $(OBJS)
	$(AR) cru $(TARGET) $(OBJS)
	$(RANLIB) $(TARGET)
