#ifndef __SYNC_QUEUE_H__
#define __SYNC_QUEUE_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition.hpp>

template<typename T>
class SyncQueue {
    private:
        typedef boost::mutex MutexType;
        typedef boost::unique_lock<MutexType> ScopedLock;
        typedef boost::condition ConditionType;

        std::list<T> queue_;
        mutable MutexType mutex_;
        ConditionType condition_;
    public:
        bool empty() const {
            ScopedLock lock(mutex_);
            return queue_.empty();
        }

        size_t size() const {
            ScopedLock lock(mutex_);
            return queue_.size();
        }

        bool front(T& result) const {
            ScopedLock lock(mutex_);
            if (queue_.empty()) {
                return false;
            } else {
                result = queue_.front();
            }
            return true;
        }

        bool back(T& result) const {
            ScopedLock lock(mutex_);
            if (queue_.empty()) {
                return false;
            } else {
                result = queue_.back();
            }
            return true;
        }

        void push(const T& t) {
            ScopedLock lock(mutex_);
            bool empty = queue_.empty();
            queue_.push_back(t);
            lock.unlock();
            if (empty) {
                condition_.notify_one();
            }
        }

        void pop() {
            ScopedLock lock(mutex_);
            queue_.pop_front();
        }

        void wait_and_pop(T& result) {
            ScopedLock lock(mutex_);
            while (queue_.empty()) {
                condition_.wait(lock);
            }
            result = queue_.front();
            queue_.pop_front();
        }
};

#endif /* __SYNC_QUEUE_H__ */
