server : server.cpp Tank.cpp
	g++ server.cpp -std=c++11  `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -g -o server.bin

client : client.cpp Tank.cpp
	g++ client.cpp -std=c++11  `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -g -o client.bin

