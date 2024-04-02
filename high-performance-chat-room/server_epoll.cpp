#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../thread/thread_pool.h"

class message : public task_base
{
public:
    message(std::string content, int send_socket, std::map<int, std::string> sock_to_ip, int listensock)
    {
        m_content = content;
        m_send_socket = send_socket;
        m_sock_to_ip = sock_to_ip;
        m_listensock = listensock;
    }
    void run()
    {
        for (auto it = m_sock_to_ip.begin(); it != m_sock_to_ip.end(); it++)
        {
            if (it->first == m_listensock || it->first == m_send_socket)
                continue;
            ssize_t sent = send(it->first, m_content.c_str(), m_content.size(), 0);
            if (sent < 0)
            {
                perror("send failed");
            }
            else if (sent == 0)
            {
                std::cout << "Connection closed by peer" << std::endl;
                close(it->first);
                m_sock_to_ip.erase(it);
            }
        }
    }

private:
    std::string m_content;
    int m_send_socket;
    std::map<int, std::string> m_sock_to_ip;
    int m_listensock;
};

struct Task
{
    std::string content;
    int send_socket;
    std::map<int, std::string> *sock_to_ip;
    int listensock;
};

std::queue<Task> task_queue;
std::mutex mtx;
std::condition_variable cv;
const int MAX_THREADS = 4; // 最大线程数
/*
void worker_thread()
{
    while (true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                    { return !task_queue.empty(); });
            task = task_queue.front();
            task_queue.pop();
        }

        sentToAll(std::move(task.content), task.send_socket, *task.sock_to_ip, task.listensock);
    }
}
*/
// 把socket设置成非阻塞
int setnonblocking(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    {
        perror("fcntl(F_GETFL) failed");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl(F_SETFL) failed");
        return -1;
    }
    return 0;
}
/*
void sentToAll(std::string content, int send_socket, std::map<int, std::string> &sock_to_ip, int listensock)
{
    for (auto it = sock_to_ip.begin(); it != sock_to_ip.end(); it++)
    {
        if (it->first == listensock || it->first == send_socket)
            continue;
        ssize_t sent = send(it->first, content.c_str(), content.size(), 0);
        if (sent < 0)
        {
            perror("send failed");
        }
        else if (sent == 0)
        {
            std::cout << "Connection closed by peer" << std::endl;
            close(it->first);
            sock_to_ip.erase(it);
        }
    }
}
*/
// 初始化服务端的监听端口
int initserver(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket() failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt() failed");
        close(sock);
        return -1;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind() failed");
        close(sock);
        return -1;
    }

    if (listen(sock, 5) != 0)
    {
        perror("listen() failed");
        close(sock);
        return -1;
    }

    return sock;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "usage: ./server_epoll [port]" << std::endl;
        return -1;
    }
    // 建立socket 与 ip 的映射关系
    std::map<int, std::string> sock_to_ip;

    // 初始化服务端
    int listensock = initserver(atoi(argv[1]));
    std::cout << "listensock=" << listensock << std::endl;

    // 设置监听端口为非阻塞
    if (setnonblocking(listensock) < 0)
    {
        std::cerr << "Failed to set listensock non-blocking" << std::endl;
        return -1;
    }

    std::cout << "Listening" << std::endl;

    if (listensock < 0)
    {
        std::cerr << "initserver() failed" << std::endl;
        return -1;
    }

    // 创建epoll句柄
    int epollfd = epoll_create1(0);
    if (epollfd < 0)
    {
        perror("epoll_create1 failed");
        return -1;
    }

    // 为服务端的listensock准备读事件
    epoll_event ev;
    ev.data.fd = listensock;
    ev.events = EPOLLIN;

    // 将listening socket加入epoll监听
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev) < 0)
    {
        perror("epoll_ctl(ADD) failed");
        close(epollfd);
        return -1;
    }

    // 创建线程池
    thread_pool *pool = thread_pool::get_instance(10);

    epoll_event evs[1024];

    while (true) // 事件循环
    {
        // 调用epoll_wait
        int infds = epoll_wait(epollfd, evs, 10, -1);
        if (infds < 0)
        {
            perror("epoll_wait failed");
            break;
        }

        if (infds == 0)
        {
            std::cout << "epoll_wait timeout" << std::endl;
            continue;
        }

        // 循环处理事件
        for (int ii = 0; ii < infds; ii++)
        {
            if (evs[ii].data.fd == listensock)
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock, (struct sockaddr *)&client, &len);
                if (clientsock < 0)
                {
                    perror("accept failed");
                    continue;
                }

                std::cout << "accept client(socket=" << clientsock << ") ok" << std::endl;

                if (setnonblocking(clientsock) < 0)
                {
                    std::cerr << "Failed to set clientsock non-blocking" << std::endl;
                    close(clientsock);
                    continue;
                }

                ev.data.fd = clientsock;
                ev.events = EPOLLIN;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev) < 0)
                {
                    perror("epoll_ctl(ADD) failed");
                    close(clientsock);
                    continue;
                }

                sock_to_ip[clientsock] = inet_ntoa(client.sin_addr);
                std::string content = sock_to_ip[clientsock] + " logging in";
                message *new_mes = new message(content,clientsock,sock_to_ip,listensock);
                pool->add_task(new_mes);
            }
            else
            {
                int eventfd = evs[ii].data.fd;
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                ssize_t received = recv(evs[ii].data.fd, buffer, sizeof(buffer), 0);
                if (received < 0)
                {
                    perror("recv failed");
                    close(evs[ii].data.fd);
                    sock_to_ip.erase(evs[ii].data.fd);
                }
                else if (received == 0)
                {
                    std::cout << "Client(eventfd=" << evs[ii].data.fd << ") disconnected" << std::endl;
                    close(evs[ii].data.fd);
                    auto it = sock_to_ip.find(evs[ii].data.fd);
                    if (it != sock_to_ip.end())
                    {
                        std::string content = it->second + " logging out";
                        message *new_mes = new message(content,eventfd,sock_to_ip,listensock);
                        pool->add_task(new_mes);
                        sock_to_ip.erase(it);
                    }
                }
                else
                {
                    std::cout << "recv(eventfd=" << evs[ii].data.fd << "):" << buffer << std::endl;
                    auto it = sock_to_ip.find(evs[ii].data.fd);
                    std::string content = it->second + " send:\n" + buffer;
                    message *new_mes = new message(content,eventfd,sock_to_ip,listensock);
                    pool->add_task(new_mes);
                }
            }
        }
    }
    close(listensock);
    close(epollfd);
    return 0;
}