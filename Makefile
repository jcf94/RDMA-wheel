# ***********************************************
# MYID : Chen Fan
# LANG : Makefile
# PROG : 
# ***********************************************

CXX = g++
CXXFLAGS = -std=c++14

SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

CLEAN-O = rm -f src/*.o

release: CXXFLAGS += -O3
release: all

debug: CXXFLAGS += -g3 -DDEV_MODE
debug: all

all: main.cpp rdmalib.a
	$(CXX) $(CXXFLAGS) -o $@ $^ -libverbs -lmlx4 -pthread
	$(CLEAN-O)
	@echo "┌──────────────────────────────┐"
	@echo "|     Target Make Success      |"
	@echo "└──────────────────────────────┘"

rdmalib.a: $(OBJ)
	ar rc $@ $^

clean:
	rm -f all rdmalib.a
	$(CLEAN-O)
	@echo "┌──────────────────────────────┐"
	@echo "|     Target Clean Success     |"
	@echo "└──────────────────────────────┘"

aaa:
	@echo $(SRC)
	@echo $(OBJ)