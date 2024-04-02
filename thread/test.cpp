#include "thread_pool.h"
#include <iostream>
#include <chrono>
#include <future>
#include <memory>
#include <unistd.h>
// 定义一个任务类
class Task : public task_base {
public:
    Task(int id, int delay) : id(id), delay(delay) {}

    virtual void run() override {
        std::cout << "Task " << id << " completed after " << delay << " seconds." << std::endl;
        sleep(delay);
    }

private:
    int id;
    int delay;
};

int main() {
    // 获取线程池实例
    thread_pool* pool = thread_pool::get_instance(5);


    sleep(5);
    // 测试添加任务
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.emplace_back(std::async(std::launch::async, [=]() {
            pool->add_task(new Task(i, i));
        }));
    }
    sleep(25);
    // 等待所有任务完成
    // 测试获取任务队列大小和线程数量
    std::cout << "Task queue size: " << pool->get_queue_size() << std::endl;
    std::cout << "Number of threads: " << pool->get_thread_num() << std::endl;

    // 测试停止线程池
    return 0;
}