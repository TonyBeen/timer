INCLUDE_DIR := -I./
CXX_FLAGS := -std=c++11 -W -Wall -Werror -Wextra -Wno-deprecated -Wno-unused-result -O2 -m64
CC := g++
SO_LIBS := -lpthread
LOG_SO_LIB := -llog

SRC_LIST := $(wildcard *.cpp)
OBJ_LIST := $(patsubst %.cpp, %.o, $(SRC_LIST))

main : main.o $(OBJ_LIST)
	$(CC) $^ -o $@ $(SO_LIBS)

test_timer : test_timer.o $(OBJ_LIST)
	$(CC) $^ -o $@ $(SO_LIBS) $(LOG_SO_LIB)

%.o : %.cpp
	$(CC) $^ -c -o $@ $(CXX_FLAGS) $(INCLUDE_DIR)

%.o : %.cc
	$(CC) $^ -c -o $@ $(CXX_FLAGS) $(INCLUDE_DIR)

.PHONY: main test_timer clean

clean:
	-rm -rf $(OBJ_LIST)