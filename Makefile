OBJECTS=./build/terminal.o

all: ${OBJECTS}
	g++ main.cpp ${OBJECTS} -g -o ./bin/main
./build/terminal.o: ./Terminal.cpp
	g++ Terminal.cpp -o ./build/terminal.o -g -c
clean:
	rm ./bin/main
	rm -rf ${OBJECTS}
