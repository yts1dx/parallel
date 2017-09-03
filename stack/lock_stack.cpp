#include <iostream>
#include <thread>
#include "lock_stack.h"


LockStack<int> global_stack;

class AccessStack
{
public:
  void operator()()
  {
    try {
      while(1) {
        int value = -1;
        global_stack.pop(value);
      }
    } catch (EmptyStack &e) {
      cout <<e.what()<<endl;
    }
    cout<<"AAAAAA";
  }
};

void init_global_stack()
{
  for (int i = 0; i < 10; ++i) {
    global_stack.push(i);
  }
}

int main(int argc, char *argv[])
{
  init_global_stack();
  AccessStack access;
  std::thread th1(access);
  std::thread th2(access);
  th1.join();
  th2.join();
  return 0;
}
