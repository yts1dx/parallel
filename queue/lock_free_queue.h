#ifndef HEADER
#define HEADER
#include<iostream>
#include<atomic>
#include<thread>
template<class T>
class lock_free_queue
{
  struct node;
  struct counted_node_ptr
  {
    int external_count_;
    node* ptr_;
  };
  std::atomic<counted_node_ptr> head_;
  std::atomic<counted_node_ptr> tail_;
  struct node_counter
  {
    unsigned internal_count_:30;
    unsigned external_counters_:2;
  };
  
  struct node
  {
    std::atomic<T*> data_;
    std::atomic<node_counter> count_;
    counted_node_ptr next_;//???为什么不是atomic
    ~node()
    {
      //      std::cout<<"destory node"<<std::endl;
    }
    node()
    {
      node_counter new_count;
      new_count.internal_count_ = 0;
      //在一个node创建时，一定有两个external_counter:
      //当该node是queue初始创建时，head和tail指向它
      //当该node是在push时创建，tail和其前一个node指向它
      new_count.external_counters_ = 2;
      count_.store(new_count);
      next_.ptr_ = nullptr;
      next_.external_count_ = 0;
    }
    //对node减引用计数，通过internal_count_减一来实现
    void release_ref()
    {
      node_counter old_count = count_.load();
      node_counter new_count;
      do {
        new_count = old_count;
        --new_count.internal_count_;
      } while (count_.compare_exchange_strong(old_count, new_count));
      if (new_count.internal_count_ == 0 && new_count.external_counters_ == 0) {
        delete this;
      }
    }
  };
private:
  void increase_external_count(std::atomic<counted_node_ptr> &counter, counted_node_ptr& old_counter);
  void free_external_count(counted_node_ptr& counter);
public:
  lock_free_queue()
  {
    node *new_node = new node;
    counted_node_ptr node_ptr;
    node_ptr.ptr_ = new_node;
    node_ptr.external_count_ = 1;
    head_.store(node_ptr);
    tail_.store(node_ptr);
  }

  void push(T new_value);
  std::unique_ptr<T> pop();
  
};
#include "lock_free_queue.cpp"
#endif
