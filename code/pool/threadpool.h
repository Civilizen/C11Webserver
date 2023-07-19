/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
//线程池类
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>()){
        assert(threadCount > 0);
        for (size_t i = 0; i < threadCount; i++){ // 根据线程数创建一定数量的线程
            std::thread(
                [pool = pool_]{
                    // 首先获取线程队列的互斥锁
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) { // 循环等待任务队列是否有任务
                        // 池子里有新任务
                        if(!pool->tasks.empty()) {
                            auto task = std::move(pool->tasks.front());
                            pool->tasks.pop();
                            // 获取完任务后释放锁，让其他线程能获得下一个任务
                            locker.unlock();
                            // 执行任务
                            task();
                            // 尝试对队列上锁，接到下一个任务
                            locker.lock();
                        } 
                        // 线程池关闭了，跳出循环
                        //（由于条件判断语句的设置，即使关闭了也会把任务队列里面已经申请的任务做完）
                        else if(pool->isClosed) break;
                        // 任务队列为空，阻塞当前线程，等待条件变量通知
                        else pool->cond.wait(locker);
                    }
                }
            )
            .detach();
        }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx; // 互斥变量
        std::condition_variable cond; //条件变量
        bool isClosed; //线程池运行状态
        std::queue<std::function<void()>> tasks;// 任务队列，没有队列大小限制
    };
    std::shared_ptr<Pool> pool_;// 多个线程同时需要指向这个池的指针
};


#endif //THREADPOOL_H