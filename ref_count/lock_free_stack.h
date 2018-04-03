#ifndef HEADER
#define HEADER
#include<iostream>
#include<atomic>
#include<thread>
template<typename T>
class lock_free_stack
{
private:
  struct node;
  struct counted_node_ptr
  {
    int external_count;//外部引用计数
    node *ptr;
  };
  struct node
  {
    std::shared_ptr<T> data;
    std::atomic<int> internal_count;//内部引用计数
    counted_node_ptr next;//使用了包含外部引用计数的wrapper
    node(T const&data_):
        data(std::make_shared<T>(data_)),
        internal_count(0)
        {
        }
  };
  //注：这里需要保证counted_node_ptr的原子操作
  //但是counted_node_ptr占用了两个字节，如果CPU没有提供DWCAS的话，会导致lock的产生
  std::atomic<counted_node_ptr> head;
private:
  void increase_head_count(counted_node_ptr &old_counter);
public:
  void push(T const &data);
  std::shared_ptr<T> pop();
};

template<class T>
void lock_free_stack<T>::push(T const &data)
{
  counted_node_ptr new_node;
  new_node.ptr = new node(data);
  new_node.external_count = 1;//创建新节点时，因为head要指向他，所以external_count要加1（个人认为这里可以比加1)
  new_node.ptr->next = head.load();
  while(!head.compare_exchange_weak(new_node.ptr->next, new_node));
}

//增加head中external_count的引用计数
template<typename T>
void lock_free_stack<T>::increase_head_count(counted_node_ptr &old_counter)
{ //此处使用new_counter十分关键！！！
  //如果直接使用old_counter进行自增，则可能会影响到head值的非预期增加，
  //因此，这里的实现非常有技巧性
  counted_node_ptr new_counter;
  do
  {
    new_counter = old_counter;
    ++new_counter.external_count;
  } while(!head.compare_exchange_strong(old_counter, new_counter));
  old_counter = new_counter;
}

template<typename T>
std::shared_ptr<T> lock_free_stack<T>::pop()
{
  counted_node_ptr old_head = head.load();
  for(;;)
  {
    increase_head_count(old_head);
    node *const ptr = old_head.ptr;
    if (ptr == nullptr) {//stack is empty
      return std::shared_ptr<T>();
    } else {
      if (head.compare_exchange_strong(old_head, ptr->next)) {
        //此时节点已经解引用，需要将external_count准备弃用,其他线程不会再对external_count加一，
        //因此将其值加到internal_count，当前node以后仅使用internal_count
        std::shared_ptr<T> res;
        res.swap(ptr->data);
        
        ptr->internal_count.fetch_sub(1);//减去push时的添加的引用计数
        ptr->internal_count.fetch_sub(1);//减去pop时，最开始的引用计数
        ptr->internal_count.fetch_add(old_head.external_count);//抛弃external_count，在internal_count中消除其影响
        //如果在对node解引用前，有其他thread获取该node的指针，则interal_count会大于0
        //否则，会等于0，此时可释放该node
        if (ptr->internal_count == 0) {
          delete ptr;
        }
        return res;
      } else if (ptr->internal_count.fetch_sub(1) == 1) {
        //当减去1后引用计数为0，则可以删除该节点
        //走到这里表明解引用时没有释放node，当前thread是对node的最后一个引用，可以在这里释放node
        delete ptr;
      } else {
        //走到这里表明街引用时并没有释放node，并且除了本thread有其他thread仍旧在持有ptr，当前thread无法释放node
      }
    } 
  }
}

#endif
