#include<iostream>
#include "lock_free_stack.h"
int main(int argc, char *argv[])
{

  lock_free_stack<int> stack;
  stack.push(1);
  stack.push(2);
  stack.push(3);

  std::shared_ptr<int> p;

  for (int i = 0 ; i < 3 ; ++i) {
    p = stack.pop();
    if (p != nullptr) {
      std::cout<<*p<<std::endl;
    } else {
      std::cout<<"empty"<<std::endl;
    }
  }

  return 0;
}

