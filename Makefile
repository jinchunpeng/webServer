CXX = g++
CXXFLAGS = -g -Wall

CLIENT = ./bin/client
SERV00 = ./bin/serv00
SERV01 = ./bin/serv01
SERV02 = ./bin/serv02
SERV03 = ./bin/serv03
SERV04 = ./bin/serv04

CLIENT_FILE = client.cpp
SERV00_FILE = serv00.cpp
SERV01_FILE = serv01.cpp
SERV02_FILE = serv02.cpp
SERV03_FILE = serv03.cpp
SERV04_FILE = serv04.cpp

all: dir client serv00 serv01 serv02 serv03 serv04
	
dir:
	mkdir -p bin log 


client:	$(CLIENT_FILE)
	$(CXX) -o $(CLIENT) $(CLIENT_FILE) $(CXXFLAGS)
serv00: $(SERV00_FILE)
	$(CXX) -o $(SERV00) $(SERV00_FILE) $(CXXFLAGS)
serv01: $(SERV01_FILE)
	$(CXX) -o $(SERV01) $(SERV01_FILE) $(CXXFLAGS)
serv02: $(SERV02_FILE)
	$(CXX) -o $(SERV02) $(SERV02_FILE) $(CXXFLAGS)
serv03: $(SERV03_FILE)
	$(CXX) -o $(SERV03) $(SERV03_FILE) $(CXXFLAGS) -lpthread
serv04: $(SERV04_FILE)
	$(CXX) -o $(SERV04) $(SERV04_FILE) $(CXXFLAGS) -lpthread

clean:
	rm -rf $(CLIENT) $(SERV00) $(SERV01) $(SERV02) $(SERV03) $(SERV04) bin log
