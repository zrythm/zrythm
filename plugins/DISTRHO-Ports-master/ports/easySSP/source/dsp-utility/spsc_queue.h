#ifndef TOMATL_SPSC_QUEUE
#define TOMATL_SPSC_QUEUE
//#include <Windows.h>

// TODO: memory fences
namespace tomatl { namespace dsp {

	// load with 'consume' (data-dependent) memory ordering
template<typename T>
T load_consume(T const* addr)
{
  // hardware fence is implicit on x86
  T v = *const_cast<T const volatile*>(addr);
  //__memory_barrier(); // compiler fence
  return v;
}

// store with 'release' memory ordering
template<typename T>
void store_release(T* addr, T v)
{
  // hardware fence is implicit on x86
 // __memory_barrier(); // compiler fence
  *const_cast<T volatile*>(addr) = v;
}

// cache line size on modern x86 processors (in bytes)
size_t const cache_line_size = 64;

// single-producer/single-consumer queue
template<typename T>
class spsc_queue
{
public:
  spsc_queue()
  {
      node* n = new node;
      n->next_ = 0;
      tail_ = head_ = first_= tail_copy_ = n;
  }

  ~spsc_queue()
  {
      node* n = first_;
      do
      {
          node* next = n->next_;
          delete n;
          n = next;
      }
      while (n);
  }

  void enqueue(T v)
  {
      node* n = alloc_node();
      n->next_ = 0;
      n->value_ = v;
      store_release(&head_->next_, n);
      head_ = n;
  }

  // returns 'false' if queue is empty
  bool dequeue(T& v)
  {
      if (load_consume(&tail_->next_))
      {
          v = tail_->next_->value_;
          store_release(&tail_, tail_->next_);
          return true;
      }
      else
      {
          return false;
      }
  }

private:
  // internal node structure
  struct node
  {
      node* next_;
      T value_;
  };

  // consumer part
  // accessed mainly by consumer, infrequently be producer
  node* tail_; // tail of the queue

  // delimiter between consumer part and producer part,
  // so that they situated on different cache lines
  char cache_line_pad_ [cache_line_size];

  // producer part
  // accessed only by producer
  node* head_; // head of the queue
  node* first_; // last unused node (tail of node cache)
  node* tail_copy_; // helper (points somewhere between first_ and tail_)

  node* alloc_node()
  {
      // first tries to allocate node from internal node cache,
      // if attempt fails, allocates node via ::operator new()

      if (first_ != tail_copy_)
      {
          node* n = first_;
          first_ = first_->next_;
          return n;
      }
      tail_copy_ = load_consume(&tail_);
      if (first_ != tail_copy_)
      {
          node* n = first_;
          first_ = first_->next_;
          return n;
      }
      node* n = new node;
      return n;
  }

  spsc_queue(spsc_queue const&);
  spsc_queue& operator = (spsc_queue const&);
};

// usage example
/*int main()
{
  spsc_queue<int> q;
  q.enqueue(1);
  q.enqueue(2);
  int v;
  bool b = q.dequeue(v);
  b = q.dequeue(v);
  q.enqueue(3);
  q.enqueue(4);
  b = q.dequeue(v);
  b = q.dequeue(v);
  b = q.dequeue(v);
}*/

}}

#endif