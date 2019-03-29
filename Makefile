# ***********************************************
# MYID : Chen Fan
# LANG : Makefile
# PROG : 
# ***********************************************

CC = g++
CFLAGS += -std=c++14 -Iutils/ThreadPool/src -Iutils/MemoryPool/src

LD = g++
LDFLAGS += -std=c++14

#NAME = $(wildcard *.cpp)
NAME = benchmark.cpp
#NAME = main.cpp
NAME-OBJS = $(patsubst %.cpp, %.o, $(NAME))
TARGET = $(patsubst %.cpp, %, $(NAME))

SRCS = $(wildcard src/*.cpp utils/ThreadPool/src/*.cpp utils/MemoryPool/src/*.cpp)
SRCS-OBJS = $(patsubst %.cpp, %.o, $(SRCS))

CLEAN-O = rm -f $(SRCS-OBJS) $(NAME-OBJS)

release: CFLAGS += -O3
release: LDFLAGS += -O3 -libverbs -lmlx4 -pthread
release: all

debug: CFLAGS += -g3 -DDEV_MODE
debug: LDFLAGS += -g3 -libverbs -lmlx4 -pthread
debug: all

all: $(TARGET)
	$(CLEAN-O)
	@echo "=------------------------------="
	@echo "|     Target Make Success      |"
	@echo "=------------------------------="

$(TARGET): $(SRCS-OBJS) $(NAME)
	$(LD) -o $@ $^ $(LDFLAGS)

rdmalib.a: $(SRCS-OBJS)
	ar rc $@ $^

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)
	$(CLEAN-O)
	@echo "=-----------------------------="
	@echo "|     Target Clean Success    |"
	@echo "=-----------------------------="

show:
	@echo NAME: $(NAME)
	@echo NAME-OBJS: $(NAME-OBJS)
	@echo TARGET: $(TARGET)
	@echo SRCS: $(SRCS)
	@echo SRCS-OBJS: $(SRCS-OBJS)
