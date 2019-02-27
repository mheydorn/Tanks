server : server.cpp
	g++ server.cpp -std=c++11 `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -g -o server.bin

client : client.cpp
	g++ client.cpp -std=c++11 `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -g -o client.bin

