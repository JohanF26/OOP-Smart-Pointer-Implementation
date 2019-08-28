CPPFLAGS= -Wall -Wextra -pedantic -g

all:	SharedPtr_test
	# Insert command to compile the code and generate executable

SharedPtr_test:	SharedPtr_test.o
				g++ $(CPPFLAGS) -pthread SharedPtr_test.o -o SharedPtr_test

SharedPtr_test.o:	SharedPtr_test.cpp
					g++ -std=c++11 $(CPPFLAGS) -pthread -c SharedPtr_test.cpp

run:	SharedPtr_test
		./SharedPtr_test

checkmem:	SharedPtr_test
			valgrind --leak-check=full ./SharedPtr_test

clean:
			rm -f *.o SharedPtr_test
