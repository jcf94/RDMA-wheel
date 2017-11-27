CXX = g++
CXXFLAGS = -std=c++11 -g3

SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

CLEAN-O = rm -f src/*.o

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