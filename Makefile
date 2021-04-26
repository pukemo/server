all: clean run

run:
	g++ -Wall -g server.cpp -o run

clean:
	rm -f run
