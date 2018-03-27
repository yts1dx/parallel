
#ifndef HAZARD_POINTER_H
#define HAZARD_POINTER_H

#include<iostream>
#include<atomic>
#include<thread>

unsigned const max_hazard_pointers = 100;

//记录thread对内存节点使用的数据结构
struct hazard_pointer
{
  std::atomic<std::thread::id> id;
  std::atomic<void *> pointer;
};
//全局变量需要在头文件中用extern声明，在source file中再次定义
extern hazard_pointer hazard_pointers[max_hazard_pointers];

//delete时需要使用相应的type，故这里使用了template
template<typename T>
void do_delete(void *p)
{
  delete static_cast<T*>(p);
}

//用来维护待删除的node
struct data_to_reclaim
{
  void *data;
  std::function<void (void*)> deleter;
  data_to_reclaim *next;

  template<typename T>
  data_to_reclaim(T*data):
      data(data),
      deleter(&do_delete<T>),
      next(nullptr)
  {
  }

  ~data_to_reclaim()
  {//在调取data_to_reclaim的析构时，能够是否data
    deleter(data);
  }
};
extern std::atomic<data_to_reclaim*> nodes_to_reclaim;

bool outstanding_hazard_pointers_for(void* p);
void delete_node_with_no_hazards();  
void add_to_reclaim_list(data_to_reclaim * node);

template<typename T>
void reclaim_later(T*data)
{
  add_to_reclaim_list(new data_to_reclaim(data));
}

std::atomic<void*> &get_hazard_pointer_for_current_thread();


//用来维护thread对应的hazard_pointer,需要做好以下三点
//1.hazard pointer的获取
//2.hazard pointer的归还
//3.hazard pointer的返回
class hp_owner
{
  hazard_pointer *hp;
public:
  hp_owner(hp_owner const&)=delete;
  hp_owner operator=(hp_owner const&)=delete;
  //构建构造函数时，在hazard_pointers中寻找到一个空闲的element
  hp_owner():
     hp(nullptr)
  {
    for (unsigned i = 0; i < max_hazard_pointers; ++i) {
      std::thread::id empty_id;
      if (hazard_pointers[i].id.compare_exchange_strong(empty_id, std::this_thread::get_id())) {
        hp = &hazard_pointers[i];
        break;
      } 
    }
    if (hp == nullptr) {
      throw std::runtime_error("No hazard pointers avaiavle");
    }
  }

  std::atomic<void*>&get_pointer()
  {
    return hp->pointer;
  }
  ~hp_owner()
  {
    hp->pointer.store(nullptr);
    hp->id.store(std::thread::id());
  }
};

#endif
