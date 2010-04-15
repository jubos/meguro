#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H
#include <queue>
#include <pthread.h>
#include <exception>

using namespace std;

class QueueEmptyException : public std::exception
{
  virtual const char* what() const throw()
  {
    return "Queue Empty";
  }
};

template <typename T>
class ThreadSafeQueue {

  public:
    ThreadSafeQueue(unsigned int capacity) {
      this->done_ = false;
      this->capacity = capacity;
      pthread_mutex_init(&mutex,NULL);
      pthread_cond_init(&cond_full,NULL);
      pthread_cond_init(&cond_empty,NULL);
    };

    ThreadSafeQueue() {
      ThreadSafeQueue(0);
    }

    bool done() { return done; }
    void done(bool done);
    void enqueue(T const &);
    unsigned int size();
    T dequeue();

  protected:
    queue<T> data;
    unsigned int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t cond_empty;
    bool done_;
};

#include "thread_safe_queue.cpp"
#endif 
