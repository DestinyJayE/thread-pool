#pragma once
#include <thread>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <atomic>
#include <assert.h>
#include "SafeQueue.h"
class task_base
{

public:
    virtual void run() = 0;
};
class thread_pool
{
private:
    std::vector<std::thread> threads;    // 线程集合
    SafeQueue<task_base *> task_queue;    // 任务队列
    int num_threads;                     // 线程数量
    void clear_queue();                  // 清理队列
    void clear_threads();                // 清理线程
    thread_pool(int num = 10);           // 私有化构造函数
    static thread_pool *instance;        // 单例
    void create_threads(int numThreads); // 创建线程
    static std::mutex instance_mutex;    // 单例锁
    void start(int numThreads);
    void worker_thread();
    bool is_running;
public:
    bool is_run() const;
    ~thread_pool();
    void add_task(task_base *task);
    static thread_pool *get_instance(int num= 10);
    int get_queue_size();
    int get_thread_num();
    void stop();
};
