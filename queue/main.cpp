#include"lock_free_queue.h"
#include<iostream>
struct Data
{
  Data(int v):v_(v) {
  }
  ~Data()
  {
    std::cout<<"destory data:"<<v_<<std::endl;
  }
  int v_;
};
int main(int argc, char *argv[])
{
  lock_free_queue<Data> q ;
  
  q.push(Data(1));
  q.push(Data(2));
  q.push(Data(3));
  std::unique_ptr<Data> data = q.pop();
  std::cout<<"pop:"<<data->v_<<std::endl;
  data = q.pop();
  std::cout<<"pop:"<<data->v_<<std::endl;
  data = q.pop();
  std::cout<<"pop:"<<data->v_<<std::endl;

}
