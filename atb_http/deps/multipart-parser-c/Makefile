CC = gcc  
CFLAGS = -Wall -std=c89
  
# 目标文件  
TARGET = libmultipart_parser.so  
# # C文件  
SRCS = multipart_parser.c
# # 目标文件  
OBJS = $(SRCS:.c=.o)  
#   
#   # 链接为可执行文件  
$(TARGET):$(OBJS)  
	$(CC) -shared -o $@ $^  
	cp -r ${TARGET} /usr/local/lib/  
clean:  
	rm -rf $(TARGET) $(OBJS)  
  
# 编译规则 $@代表目标文件 $< 代表第一个依赖文件  
%.o:%.c  
	$(CC) $(CFLAGS) -o $@ -fPIC -c $<  
