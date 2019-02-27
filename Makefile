main.o : test.cpp
	g++ test.cpp -std=c++11 `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -g -o threaded_examp
