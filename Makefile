main.o : test.cpp
	gcc test.cpp `pkg-config --cflags --libs gtk+-2.0 --libs gthread-2.0` -Wall -o threaded_examp
