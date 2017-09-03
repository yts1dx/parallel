/*
  1. 支持并发访问的stack
  2. 通过mutex来控制并发访问
*/

#include <exception>
#include <memory>
#include <mutex>
#include <stack>
using namespace std;

struct EmptyStack:public std::exception
{
  const char* what() const throw()
  {
    return "empty stack";
  }
  EmptyStack() {
  }
  ~EmptyStack() {
  }
  
};

template <class T>
class LockStack
{
public:
  LockStack() {}
  LockStack(LockStack &other);

  void push(const T & value);
  void pop(T & value);
  shared_ptr<T> pop();
  bool empty()const
  {
    lock_guard<mutex> lock(mutex_);
    return data_.empty();
  }
private:
  stack<T> data_;
  //mutable修饰符为了在const函数中仍旧可以使用mutex
  mutable mutex mutex_;
};

template<class T>
LockStack<T>::LockStack(LockStack &other)
{
  lock_guard<mutex> lock(mutex_);
  data_ = other;
}

template<class T>
void LockStack<T>::push(const T&value)
{
  std::lock_guard<std::mutex> lock(mutex_);
  data_.push(value);
}

template<class T>
void LockStack<T>::pop(T &value)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (data_.empty()) {
    throw EmptyStack();
  } else {
    value = data_.top();
    data_.pop();
  }
}

template<class T>
std::shared_ptr<T> LockStack<T>::pop()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (data_.empty()) {
    throw EmptyStack();
  } 
  std::shared_ptr<T> res(std::make_shared<T>(data_.top()));
  data_.pop();
  return res;
}
