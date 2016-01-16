#include <iostream>
#include <stdint.h>
using namespace std;
#define SUCCESS  0
#define INVALID_ID UINT64_MAX
class FixQueue
{
  static const int MAX_CAPACITY = 1024;
public:
  FixQueue():capacity_(MAX_CAPACITY),data_(NULL),push_idx_(INVALID_ID),pop_idx_(INVALID_ID)
             {}
  int init();
  int push(void *val);
  int pop(void *&val);
  //  void top();
private:
  uint64_t capacity_;
  void **data_;
  uint64_t push_idx_;
  uint64_t pop_idx_;
};

int FixQueue::init()
{
  int ret = SUCCESS;
  data_ = static_cast<void **>(malloc(MAX_CAPACITY * sizeof (void *)));
  if (NULL == data_) {
    cout<<"fail to malloc"<<endl;
  } else {
    push_idx_ = 0;
    pop_idx_ = 0;
  }
  return ret;
}

int FixQueue::push(void *val)
{
  int ret = SUCCESS;
  if ((push_idx_ - pop_idx_ + 1 + capacity_) % capacity_ == capacity_ ) {
    ret = -1;
    cout<<"queue is full\n";
  } else {
    push_idx_++;
    data_[push_idx_ % capacity_] = val;
  }
  return ret;
}

int FixQueue::pop(void *&val)
{
  int ret = SUCCESS;
  if (push_idx_ == pop_idx_) {
    ret = -1;
    cout<<"queue is empty\n";
  } else {
    pop_idx_++;    
    val = data_[pop_idx_ % capacity_];
  }
  return ret;
}
int main(int argc, char *argv[])
{
  
  return 0;
}
