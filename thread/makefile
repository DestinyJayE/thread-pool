all:test thread_pool.o
thread_pool.o:thread_pool.cpp
	g++ -c -std=c++11 thread_pool.cpp -o thread_pool.o -g
test:thread_pool.o test.cpp
	g++ -std=c++11 -o test thread_pool.o test.cpp -g
clean:
	rm -f *.o test