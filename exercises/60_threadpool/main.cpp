#include "../exercise.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

// READ: 线程池的基本概念和实现
// READ: std::thread <https://zh.cppreference.com/w/cpp/thread/thread>
// READ: std::mutex <https://zh.cppreference.com/w/cpp/thread/mutex>
// READ: std::condition_variable <https://zh.cppreference.com/w/cpp/thread/condition_variable>
// READ: std::future <https://zh.cppreference.com/w/cpp/thread/future>
// READ: std::packaged_task <https://zh.cppreference.com/w/cpp/thread/packaged_task>


// TODO: 实现一个简化的线程池
// HINT：使用 std::thread, std::mutex, std::condition_variable, std::future, std::packaged_task
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_flag(false) {
		for (size_t i = 0; i < num_threads; ++i) {
			workers.emplace_back([this] {
				while (true) {
					std::function<void()> task;
					
					{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						this->condition.wait(lock, [this] {
							return this->stop_flag || !this->tasks.empty();
						});
						
						if (this->stop_flag && this->tasks.empty()) {
							return;
						}
						
						task = std::move(this->tasks.front());
						this->tasks.pop();
					}
					
					task();
				}
			});
		}
	}

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // 析构函数，停止线程池
    ~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop_flag = true;
		}
		
		condition.notify_all();
		
		for (std::thread &worker : workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}


    // 向线程池提交一个任务
    template<class F, class... Args>
	auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
		using return_type = typename std::result_of<F(Args...)>::type;
		
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		
		std::future<return_type> res = task->get_future();
		
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			
			if (stop_flag) {
				throw std::runtime_error("enqueue on stopped ThreadPool");
			}
			
			tasks.emplace([task]() { (*task)(); });
		}
		
		condition.notify_one();
		return res;
	}


    // 停止线程池
    void stop() {
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop_flag = true;
		}
		condition.notify_all();
	}

    // 等待所有任务完成
    void join() {
        // TODO: 等待所有任务完成
		for (std::thread &worker : workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
    }
private:
	std::vector<std::thread> workers;           // 工作线程集合
    std::queue<std::function<void()>> tasks;    // 任务队列
    std::mutex queue_mutex;                     // 保护任务队列的互斥锁
    std::condition_variable condition;          // 通知线程的条件变量
    bool stop_flag;                                  // 停止标志
};




int main(int argc, char **argv) {
    // 1. 创建一个包含 4 个线程的线程池
    ThreadPool pool(4);
	
    std::atomic<int> counter = 0;
    auto increment_counter = [&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
    };

    // 2. 向线程池提交一个简单的任务 (增加计数器)
    std::future<void> future1 = pool.enqueue(increment_counter);
    future1.get();// 等待任务完成
    ASSERT(counter.load() == 1, "Counter should be 1 after one task");

    // 3. 提交一个返回值的任务
    auto multiply = [](int a, int b) { return a * b; };
    std::future<int> future2 = pool.enqueue(multiply, 5, 6);
    int result = future2.get();// 等待并获取结果
    ASSERT(result == 30, "Result of 5 * 6 should be 30");

    // 4. 提交多个任务
    std::vector<std::future<void>> futures;
    int num_tasks = 10;
    for (int i = 0; i < num_tasks; ++i) {
        futures.emplace_back(pool.enqueue(increment_counter));
    }

    // 等待所有任务完成
    for (auto &fut : futures) {
        fut.get();
    }

    // 验证计数器的最终值 (初始为1，加上10次递增)
    ASSERT(counter.load() == 11, "Counter should be 11 after 10 more tasks");

    // 5. 线程池析构函数会自动停止并Join所有线程
    // (无需显式调用 stop 或 join)
    return 0;
}
