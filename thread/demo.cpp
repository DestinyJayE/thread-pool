#include<iostream>
#include<mutex>
#include<thread>
#include<stdio.h>
#include<condition_variable>
#include<unistd.h>
#include<exception>
std::mutex m_lock;
std::condition_variable condition;
std::unique_lock<std::mutex> lock(m_lock);
void print()
{
    printf("%d wait lock\n",std::this_thread::get_id());
    condition.wait(lock);
    printf("%d is notify\n",std::this_thread::get_id());
    sleep(5);
    //lock.lock();
    try{
    //lock.unlock();
    }
    catch(...)
    {

    }
    printf("this is thread %d\n",std::this_thread::get_id());
    sleep(10);
}
void get_up()
{
    sleep(5);
    condition.notify_all();
    lock.lock();
}

int main()
{
    std::thread t1(print);
    std::thread t2(print);
    std::thread t3(get_up);
    t1.join();
    t2.join(); 
    t3.join();
}