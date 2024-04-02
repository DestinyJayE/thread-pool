#include "thread_pool.h"

thread_pool *thread_pool::instance = nullptr;
std::mutex thread_pool::instance_mutex;
/**
 * 获取线程池实例  饿汉模式 线程安全
*/
thread_pool *thread_pool::get_instance(int num)
{
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (instance == nullptr)
    {
        instance = new thread_pool(num);
    }
    return instance;
}
/**
 * 线程池构造函数
*/
thread_pool::thread_pool(int num)
{
    printf("creating thread pool...\n");
    is_running = true;
    num_threads = num;
    create_threads(num);
    printf("done\n");
}
/**
 * 线程池析构函数
*/
thread_pool::~thread_pool()
{
    clear_queue();
    clear_threads();
    delete instance;
    instance = nullptr;
    is_running = false;
}
/**
 * 获取线程数量
*/
int thread_pool::get_thread_num()
{
    return threads.size();
}
/**
 * 获取任务队列大小
*/
int thread_pool::get_queue_size()
{
    return task_queue.size();
}

/**
 * 创建新线程
*/
void thread_pool::create_threads(int numThreads)
{
    for (int i = 0; i < numThreads; i++)
    {
        threads.emplace_back(&thread_pool::worker_thread, this);
    }
}

/**
 * 线程池的执行函数
*/
void thread_pool::worker_thread()
{
    while (is_running)
    {
        task_base *task = nullptr;
        if (!is_running)
            break;
        if(!task_queue.pop(task))
            {
                continue;
            }
        //printf("thread %d is processing task\n", std::this_thread::get_id());
        task->run();
        delete task; // Release memory after task execution
    }
}

/**
 * 启动线程池
*/

void thread_pool::start(int numThreads)
{
    assert(numThreads>0);
    num_threads = numThreads;
    create_threads(num_threads);
}

/**
 * 停止线程池（这样会导致所有线程资源释放）
*/
void thread_pool::stop()
{
    is_running = false;
}

/**
 * 清除任务队列
*/
void thread_pool::clear_queue()
{
    while (!task_queue.empty())
    {
        task_base *task;
        task_queue.pop(task) == true;
        delete task;
    }
}

/**
 * 清除线程集合
*/
void thread_pool::clear_threads()
{
    for (auto &thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
    threads.clear();
}

/**
 * @param task 需要添加的任务指针
 * 向线程池中添加任务
*/
void thread_pool::add_task(task_base *task)
{
    task_queue.push(task);
}
