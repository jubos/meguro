#ifndef THREAD_SAFE_QUEUE_CPP
#define THREAD_SAFE_QUEUE_CPP
#include "thread_safe_queue.h"
#include <pthread.h>

#include <iostream>

using namespace std;

template <typename T> 
void ThreadSafeQueue<T>::enqueue(T const &value)
{
	pthread_mutex_lock(&mutex);
  if (capacity > 0) {
    while (data.size() == capacity) {
      pthread_cond_wait(&cond_full, &mutex);
    }
  }
  data.push(value);
	pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&cond_empty);
}

template <typename T>  
T
ThreadSafeQueue<T>::dequeue()
{
	pthread_mutex_lock(&mutex);
	if (data.size() == 0) {
    while(data.size() == 0 && !done_) {
      pthread_cond_wait(&cond_empty,&mutex);
    }
  }
  
  if (done_ && data.size() == 0) {
    pthread_mutex_unlock(&mutex);
    throw QueueEmptyException();
  }

  T value = data.front();
  data.pop();
	pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&cond_full);
	return value;
}

template <typename T>  
unsigned int
ThreadSafeQueue<T>::size()
{
  int size;
	pthread_mutex_lock(&mutex);
  size = data.size();
	pthread_mutex_unlock(&mutex);
  return size;
}

template <typename T>
void 
ThreadSafeQueue<T>::done(bool done)
{
  pthread_mutex_lock(&mutex);
  this->done_ = done;
  pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&cond_empty);
}
#endif
