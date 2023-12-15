INCLUDE_DIR := -I./
CXX_FLAGS := -std=c++11 -W -Wall -Werror -Wextra -Wno-deprecated -Wno-unused-result -O2 -m64
CC := g++
SO_LIBS := -lpthread -llog

SRC_LIST := $(wildcard *.cpp)
OBJ_LIST := $(patsubst %.cpp, %.o, $(SRC_LIST))

test_timer : $(OBJ_LIST)
	$(CC) $^ -o $@ $(SO_LIBS)

%.o : %.cpp
	$(CC) $^ -c -o $@ $(CXX_FLAGS) $(INCLUDE_DIR)

.PHONY: clean

clean:
	-rm -rf $(OBJ_LIST)