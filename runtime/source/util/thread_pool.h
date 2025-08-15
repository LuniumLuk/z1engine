#pragma once

#include <cstddef>
#include <vector>
#include <queue>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace z1 {

	// A simple thread-safe priority aueue
	template<typename T, typename Container = std::vector<T>, typename Compare = std::less<T>>
	struct ThreadSafePriorityQueue : protected std::priority_queue<T, Container, Compare> {
		using Lock = std::unique_lock<std::mutex>;
		using Base = std::priority_queue<T, Container, Compare>;

		ThreadSafePriorityQueue() = default;
		~ThreadSafePriorityQueue() { clear(); }
		ThreadSafePriorityQueue(ThreadSafePriorityQueue const&) = delete;
		ThreadSafePriorityQueue(ThreadSafePriorityQueue&&) = delete;
		ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue const&) = delete;
		ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue&&) = delete;

		bool empty() const {
			Lock lock(m_mtx);
			return Base::empty();
		}

		size_t size() const {
			Lock lock(m_mtx);
			return Base::size();
		}

		void clear() {
			Lock lock(m_mtx);
			while (!Base::empty()) {
				Base::pop();
			}
		}

		// Enqueue, as push() in STL queue
		void enqueue(T const& obj) {
			Lock lock(m_mtx);
			Base::push(obj);
			m_cv.notify_one();
		}

		template <typename ...Args>
		void emplace(Args&& ...args) {
			Lock lock(m_mtx);
			Base::emplace(std::forward<Args>(args)...);
			m_cv.notify_one();
		}

		// Dequeue, combination of top() and pop() in STL queue
		bool dequeue(T& holder) {
			Lock lock(m_mtx);
			if (Base::empty()) {
				return false;
			}
			else {
				holder = std::move(Base::top());
				Base::pop();
				return true;
			}
		}

		// Dequeue with timeout
		template <typename Rep, typename Period>
		bool dequeue(T& holder, const std::chrono::duration<Rep, Period>& timeout) {
			Lock lock(m_mtx);
			if (m_cv.wait_for(lock, timeout, [&] { return !Base::empty(); })) {
				holder = std::move(Base::top());
				Base::pop();
				return true;
			}
			else {
				return false;
			}
		}

	private:
		std::condition_variable m_cv;
		mutable std::mutex m_mtx;
	};

	// A simple thread-safe queue
	template<typename T, typename Container = std::deque<T>>
	struct ThreadSafeQueue : protected std::queue<T, Container> {
		using Lock = std::unique_lock<std::mutex>;
		using Base = std::queue<T, Container>;

		ThreadSafeQueue() = default;
		~ThreadSafeQueue() { clear(); }
		ThreadSafeQueue(ThreadSafeQueue const&) = delete;
		ThreadSafeQueue(ThreadSafeQueue&&) = delete;
		ThreadSafeQueue& operator=(ThreadSafeQueue const&) = delete;
		ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

		bool empty() const {
			Lock lock(m_mtx);
			return Base::empty();
		}

		size_t size() const {
			Lock lock(m_mtx);
			return Base::size();
		}

		void clear() {
			Lock lock(m_mtx);
			while (!Base::empty()) {
				Base::pop();
			}
		}

		// Enqueue, as push() in STL queue
		void enqueue(T const& obj) {
			Lock lock(m_mtx);
			Base::push(obj);
			m_cv.notify_one();
		}

		template <typename ...Args>
		void emplace(Args&& ...args) {
			Lock lock(m_mtx);
			Base::emplace(std::forward<Args>(args)...);
			m_cv.notify_one();
		}

		// Dequeue, combination of top() and pop() in STL queue
		bool dequeue(T& holder) {
			Lock lock(m_mtx);
			if (Base::empty()) {
				return false;
			}
			else {
				holder = std::move(Base::front());
				Base::pop();
				return true;
			}
		}

		// Dequeue with timeout
		template <typename Rep, typename Period>
		bool dequeue(T& holder, const std::chrono::duration<Rep, Period>& timeout) {
			Lock lock(m_mtx);
			if (m_cv.wait_for(lock, timeout, [&] { return !Base::empty(); })) {
				holder = std::move(Base::top());
				Base::pop();
				return true;
			}
			else {
				return false;
			}
		}

	private:
		std::condition_variable m_cv;
		mutable std::mutex m_mtx;
	};

	// A simple thread pool
	struct ThreadPool {
		ThreadPool(size_t);
		~ThreadPool();

		template<typename F, typename... Args>
		std::future<typename std::invoke_result<F, Args...>::type> enqueue(F&& f, Args&&... args);

		template<typename F, typename... Args>
		std::future<typename std::invoke_result<F, Args...>::type> enqueue(size_t thread_range, F&& f, Args&&... args);

	private:
		std::vector< std::thread > m_workers;
		std::queue< std::pair<std::function<void()>, size_t> > m_tasks;

		std::mutex m_mtx;
		std::condition_variable m_cv;
		bool m_stop;
	};

	inline ThreadPool::ThreadPool(size_t size) : m_stop(false) {
		for (size_t i = 0; i < size; ++i) {
			m_workers.emplace_back(
				[this, i] {
					for (;;) {
						std::function<void()> task;
						// https://en.wikipedia.org/wiki/Spurious_wakeup
						// https://stackoverflow.com/questions/42714632/notify-one-in-c-thread-waking-up-more-than-one-thread
						// https://en.cppreference.com/w/cpp/thread/condition_variable/wait
						// about 'std::condition_variable::wait':
						//   wait causes the current thread to block until the condition variable is notified or a !!spurious wakeup!! occurs.
						//   pred can be optionally provided to detect !!spurious wakeup!!.
						{
							std::unique_lock<std::mutex> lock(this->m_mtx);
							this->m_cv.wait(lock,
								[this, i] {
									// stopped and no more tasks, safe to return
									if (this->m_stop && this->m_tasks.empty())
										return true;

									if (!this->m_tasks.empty()) {
										if (i < this->m_tasks.front().second) {
											return true;
										}
										else {
											// skip the task, notify another thread to handle this
											this->m_cv.notify_one();
											return false;
										}
									}
									return false;
								});

							// stopped and no more tasks, safe to return
							if (this->m_stop && this->m_tasks.empty())
								return;

							// do the task within range
							if (!this->m_tasks.empty() && (i < this->m_tasks.front().second)) {
								task = std::move(this->m_tasks.front().first);
								this->m_tasks.pop();
							}
						}
						task();
					}
				});
		}
	}

	inline ThreadPool::~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(m_mtx);
			m_stop = true;
		}
		m_cv.notify_all();
		for (std::thread& worker : m_workers)
			worker.join();
	}

	template<typename F, typename... Args>
	std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(size_t thread_range, F&& f, Args&&... args) {
		if (m_stop)
			throw std::runtime_error("Enqueue on stopped ThreadPool");

		using return_type = typename std::invoke_result<F, Args...>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(m_mtx);
			m_tasks.emplace([task]() { (*task)(); }, thread_range);
		}
		m_cv.notify_one();
		return res;
	}

	template<typename F, typename... Args>
	std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args) {
		return enqueue(std::forward<size_t>(m_workers.size()), std::forward<F>(f), std::forward<Args>(args)...);
	}

}
