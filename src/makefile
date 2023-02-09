CXX = g++
objects = main.o server.o log.o utils.o  http_conn.o 
bugobject = main.cpp ./server/server.cpp ./http/http_conn.cpp ./log/log.cpp ./utils/utils.cpp

bug:$(bugobject)
	$(CXX) $(bugobject)  -g -o main

server:$(objects)
	$(CXX) $(objects) -o main
	-rm $(objects)

main.o:main.cpp server.o
	$(CXX) -c main.cpp server.o

server.o:./server/server.cpp
	$(CXX) -c ./server/server.cpp 

http_conn.o:./http/http_conn.cpp utils.o log.o
	$(CXX) -c ./http/http_conn.cpp utils.o log.o

log.o:./log/log.cpp
	$(CXX) -c ./log/log.cpp

utils.o:./utils/utils.cpp
	$(CXX) -c ./utils/utils.cpp



.PHONY:clean
clean:
	-rm $(objects)