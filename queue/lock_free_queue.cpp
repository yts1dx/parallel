
template<class T>
void lock_free_queue<T>::push(T new_value)
{
  //这里使用unique_ptr是为了确保new_value的内存在*异常*情况下仍旧能够释放
  std::unique_ptr<T> new_data(new T(new_value));
  counted_node_ptr new_node_ptr;
  new_node_ptr.ptr_ = new node;
  new_node_ptr.external_count_ = 1;
  counted_node_ptr old_tail = tail_.load();
  while(true) {
    increase_external_count(tail_, old_tail);//由于访问了old_tail.ptr的内容，需要添加引用计数
    T *null_data = nullptr;
    if (old_tail.ptr_->data_.compare_exchange_strong(null_data, new_data.get())) {
      old_tail.ptr_->next_ = new_node_ptr;
      old_tail=tail_.exchange(new_node_ptr);
      free_external_count(old_tail);
      new_data.release();//由于new_data的内存已经指派给了queue，unique_ptr release掉对其所有权
      break;
    } else {
      old_tail.ptr_->release_ref();      
    }
  }
}

//添加old_counter的引用计数
template<class T>
void lock_free_queue<T>::increase_external_count(std::atomic<counted_node_ptr> &counter,
                                                 counted_node_ptr& old_counter)
{
  counted_node_ptr new_counter;
  do {
    new_counter = old_counter;
    new_counter.external_count_++;
  } while(!counter.compare_exchange_strong(old_counter, new_counter,
                                           std::memory_order_acquire, std::memory_order_relaxed));
  old_counter = new_counter;
}
//功能：弃用external_count，并判断当前节点是否可以删除
//实现：将external_count的内容添加到internal_count；将external_counters减一
template<class T>
void lock_free_queue<T>::free_external_count(counted_node_ptr& counter)
{
  node *const ptr = counter.ptr_;
  //external_count_准备弃用，将其加到internal_count中
  //减二，一是由于创建时会对external_count_进行加一，二是由于increase_external_counth时加一
  int const count_increase=counter.external_count_ - 2;
  node_counter new_counter;
  node_counter old_counter = ptr->count_.load();
  do {
    new_counter = old_counter;
    new_counter.external_counters_--;
    new_counter.internal_count_ +=count_increase;
  } while(!ptr->count_.compare_exchange_strong(old_counter, new_counter));

  if (new_counter.internal_count_ == 0 && new_counter.external_counters_ == 0) {
    //当external_counter为0后，不会有人再引用ptr，因此可以直接delete掉
    delete ptr;
  }
}
//返回的是一个临时的unique_ptr，因此在外部通过pop给一个unique_ptr赋值时，会调用转移语意的构造函数
template<class T>
std::unique_ptr<T> lock_free_queue<T>::pop()
{
  counted_node_ptr old_head = head_.load(std::memory_order_relaxed);
  while(true) {
    increase_external_count(head_, old_head);
    node *ptr = old_head.ptr_;
    if (ptr == tail_.load().ptr_) {
      ptr->release_ref();
      return std::unique_ptr<T>();
    }
    if (head_.compare_exchange_strong(old_head, ptr->next_)) {
      T* const res = ptr->data_.exchange(nullptr);
      free_external_count(old_head);
      //这里是一个临时对象，这个临时对象作为参数传递给返回值，返回值在作为参数传给外部的调用者
      //因此在外部调用pop给unique_ptr赋值时，一共使用了两次转移语意
      return std::unique_ptr<T>(res);
    } else {
      old_head.ptr_->release_ref();
    }
  }
}
