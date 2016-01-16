#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <memory>
using namespace std;
class AlignAllocator
{
public:
  void *alloc(size_t request, size_t align_size);
  void free(void *ptr);
  size_t upper_align(size_t input, size_t align_size);
private:
  
};

void *AlignAllocator::alloc(size_t request, size_t align_size)
{
     static const size_t ptr_alloc = sizeof(void *);
     //static const size_t align_size = 64;
     static const size_t request_size = request+align_size;
     static const size_t needed = ptr_alloc+request_size;

     void *alloc = NULL;
     void *ptr = NULL;
     if (NULL == (alloc = malloc(needed))) {
       perror("fail to alloc");
     } else {
       ptr = reinterpret_cast<void *>(upper_align(reinterpret_cast<size_t>(alloc), align_size));
       ((void **)ptr)[-1] = alloc; // save for delete calls to use
       cout<<"alloc: "<<hex<<alloc<<endl<<"ptr: "<<hex<<ptr<<endl;
     }

     return ptr;  
}

void AlignAllocator::free(void *ptr)
{
  void *ptr_f =((void **)ptr)[-1];
  cout<<"ptr_f: "<<hex<<ptr_f<<endl<<"ptr: "<<hex<<ptr<<endl;
  ::free(ptr_f);
}

size_t AlignAllocator::upper_align(size_t input, size_t alignment)
{
  input = (input + alignment -1) & (~(alignment - 1));
  return input;
}
int main()
{
  AlignAllocator allocator;
  const size_t request = 3;
  void *p = allocator.alloc(request, 1<<20);
  cout<<"address: "<<hex<<p<<endl;
  allocator.free(p);
}


