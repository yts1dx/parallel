
#include"hazard_pointer.h"

//如果没有被thread指引，则可以删除
bool outstanding_hazard_pointers_for(void* p)
{
  for (unsigned i = 0; i < max_hazard_pointers; ++i) {
    if (hazard_pointers[i].pointer.load() == p) {
      return true;
    }
  }
  return false;
}

//尝试reclaim所有节点
//在删除节点时这里使用了一个技巧：
//1.首先将nodes_to_reclaim置为NULL，并将其值获取到本地节点上，并判断当前链表的每个节点是否可以删除，此时已经没有竞争
//2.如果可以删除则直接删除
//3.否则再次加入到nodes_to_reclaim中。
//注：nodes_to_reclaim中的node已经解引用了，当其没有被thread占用后，就再也不会被占用
void delete_node_with_no_hazards()
{
  data_to_reclaim *current = nodes_to_reclaim.exchange(nullptr);
  while(current) {
    data_to_reclaim *next = current->next;
    if (!outstanding_hazard_pointers_for(current->data)) {
      delete current;
    } else {
      add_to_reclaim_list(current);
    }
    current = next;
  }
}

void add_to_reclaim_list(data_to_reclaim * node)
{
  node->next = nodes_to_reclaim.load();
  while(!nodes_to_reclaim.compare_exchange_weak(node->next, node));
}

//通过thread_local static来保存thread的hp_owner
std::atomic<void*> &get_hazard_pointer_for_current_thread()
{
  thread_local static hp_owner hazard;
  return hazard.get_pointer();
}
