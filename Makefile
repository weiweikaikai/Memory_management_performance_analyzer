CC=g++
BIN=UnitMemory_Profile
FLAG=-g -Wall
SRC=test.cpp  UnitMemory_Profile.cpp
INCLUDE=./
LIB=-lpthread
$(BIN):$(SRC)
	$(CC) -o $@ $^ $(FLAG)  $(LIB)
.PHONY:clean
clean:
	rm -rf $(BIN)