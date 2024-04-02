all:server_epoll client
server_epoll:server_epoll.cpp ../thread/thread_pool.cpp
	g++ -o server_epoll server_epoll.cpp ../thread/thread_pool.cpp 
client:client.cpp
	g++ -o client client.cpp
clean:
	rm -f client server_epoll
