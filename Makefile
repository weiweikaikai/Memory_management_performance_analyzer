CC=g++
BIN=UnitMemory_Profile
CLI=CtrolTool
FLAG=-g -Wall
SRC=test.cpp  UnitMemory_Profile.cpp IPCMonitorServer.cpp 
_SRC=CtrolTool.cpp
INCLUDE=./
LIB=-lpthread
.PHONY:all
all:$(BIN) $(CLI)
$(BIN):$(SRC)
	$(CC) -o $@ $^ $(FLAG)  $(LIB)
$(CLI):$(_SRC)
	$(CC) -o $@ $^ $(FLAG)  $(LIB)
.PHONY:clean
clean:
	rm -rf $(BIN) $(CLI)
