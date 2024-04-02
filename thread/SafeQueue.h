#pragma once
#include <queue>
#include <mutex>
#include<stdio.h>
template <class T>
class SafeQueue
{
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;

public:
    bool empty()
    {
        std::unique_lock<std::mutex> lock(m_mutex); // 互斥信号变量加锁，防止m_queue被改变
        return m_queue.empty();
        // 离开作用域unique_lock自动释放锁
    }

    int size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
        // 离开作用域unique_lock自动释放锁
    }

    void push(T &data)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_queue.push(data);
        //printf("after push queue size is %d\n", m_queue.size());
    }

    bool pop(T &t)
    {
        std::unique_lock<std::mutex> lock(m_mutex); // 队列加锁
        if (m_queue.empty())
            return false;
        t = std::move(m_queue.front()); // 取出队首元素，返回队首元素值，并进行右值引用
        m_queue.pop();                  // 弹出入队的第一个元素
        //printf("after pop queue size is %d\n", m_queue.size());
        return true;
    }
};