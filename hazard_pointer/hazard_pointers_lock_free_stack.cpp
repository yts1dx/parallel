#include "hazard_pointer.h"
hazard_pointer hazard_pointers[max_hazard_pointers];
std::atomic<data_to_reclaim*> nodes_to_reclaim;

template<typename T>
class hazard_pointer_lock_free_stack
{
private:
  struct node
  {
    node(const T&other_data)
        :data(std::make_shared<T>(other_data)),
         next(nullptr)
    {
    }
    ~node() {
      std::thread::id this_id = std::this_thread::get_id();
      std::cout<<"destroy node"<<" thread id"<<this_id<<std::endl;
    }
    std::shared_ptr<T> data;
    node * next;
  };
  std::atomic<node*> head;
  std::atomic<unsigned> threads_in_pop;
  std::atomic<node*> to_be_delete;

public:
  void push(const T&data);
  std::shared_ptr<T> pop();
};

template<typename T>
void hazard_pointer_lock_free_stack<T>::push(const T&data)
{
  node *new_node = new node(data);
  new_node->next = head.load();
  while(!head.compare_exchange_weak(new_node->next, new_node));
}

template<typename T>
std::shared_ptr<T> hazard_pointer_lock_free_stack<T>::pop()
{
  //获得指向当前线程的hazard pointer
  //std::atomic is neither copyable nor movable.因此这里返回引用
  std::atomic<void*> &hp = get_hazard_pointer_for_current_thread();
  node *old_head= head.load();
  node *tmp = NULL;
  //将待待删除的节点加入到hp中，并尝试解引用该节点
  do {
    do {//将待删除的节点添加到current thread的hazard pointer中
      tmp = old_head;
      hp.store(tmp);
      old_head = head.load();
    } while(tmp != old_head);//两个while保证了，这些operation过程中head没有变更过
  } while(old_head != nullptr && !head.compare_exchange_strong(old_head, old_head->next));

  hp.store(nullptr);//已经解引用，当前thread的hp不需要再指向节点
  std::shared_ptr<T> res;
  if (old_head) {
    res.swap(old_head->data);
    if (outstanding_hazard_pointers_for(old_head)) {//检查其他thread是否占用old_head
      reclaim_later(old_head);
    } else {//走到这里说明没有其他thead占用old_head ,并且由于已经解引用，以后也不可能会有线程占用
      delete old_head;
    }
    delete_node_with_no_hazards();
  }
  return res;
}


int main(int argc, char *argv[])
{
  hazard_pointer_lock_free_stack<int> s;
  s.push(1);
  s.push(2);
  s.push(3);
  std::shared_ptr<int> p;

  for (int i = 0 ; i < 3 ; ++i) {
    p = s.pop();
    if (p != nullptr) {
      std::cout<<*p<<std::endl;
    } else {
      std::cout<<"empty"<<std::endl;
    }
  }
 
  return 0;
}
