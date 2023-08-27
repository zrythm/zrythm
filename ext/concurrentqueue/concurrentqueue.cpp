#include "concurrentqueue.h"
#include "concurrentqueue_c.h"

typedef moodycamel::ConcurrentQueue<void*> MoodycamelCQType, *MoodycamelCQPtr;

extern "C" {

int moodycamel_cq_create(MoodycamelCQHandle* handle, size_t capacity)
{
	MoodycamelCQPtr retval = new MoodycamelCQType(capacity);
	if (retval == nullptr) {
		return 0;
	}
	*handle = retval;
	return 1;
}

int moodycamel_cq_destroy(MoodycamelCQHandle handle)
{
	delete reinterpret_cast<MoodycamelCQPtr>(handle);
	return 1;
}

int moodycamel_cq_enqueue(MoodycamelCQHandle handle, MoodycamelValue value)
{
	return reinterpret_cast<MoodycamelCQPtr>(handle)->enqueue(value) ? 1 : 0;
}

int moodycamel_cq_try_enqueue(MoodycamelCQHandle handle, MoodycamelValue value)
{
	return reinterpret_cast<MoodycamelCQPtr>(handle)->try_enqueue(value) ? 1 : 0;
}

int moodycamel_cq_try_dequeue(MoodycamelCQHandle handle, MoodycamelValue* value)
{
	return reinterpret_cast<MoodycamelCQPtr>(handle)->try_dequeue(*value) ? 1 : 0;
}

int moodycamel_cq_try_enqueue_producer(MoodycamelCQHandle handle, void * token, MoodycamelValue value)
{
  moodycamel::ProducerToken * tok = reinterpret_cast<moodycamel::ProducerToken *>(token);
  assert (typeid (*tok) == typeid(moodycamel::ProducerToken));
	return reinterpret_cast<MoodycamelCQPtr>(handle)->try_enqueue(*tok, value) ? 1 : 0;
}

int moodycamel_cq_try_dequeue_consumer(MoodycamelCQHandle handle, void * token, MoodycamelValue* value)
{
  moodycamel::ConsumerToken * tok = reinterpret_cast<moodycamel::ConsumerToken *>(token);
	return reinterpret_cast<MoodycamelCQPtr>(handle)->try_dequeue(*tok, *value) ? 1 : 0;
}

size_t moodycamel_cq_size_approx(MoodycamelCQHandle handle)
{
    return reinterpret_cast<MoodycamelCQPtr>(handle)->size_approx();
}

void * moodycamel_create_producer_token(MoodycamelCQHandle handle)
{
	MoodycamelCQPtr q = reinterpret_cast<MoodycamelCQPtr>(handle);
  assert (typeid (*q) == typeid(moodycamel::ConcurrentQueue<void *>));
  moodycamel::ProducerToken * ptok = new moodycamel::ProducerToken(*q);
  return (void *) ptok;
}

void * moodycamel_create_consumer_token(MoodycamelCQHandle handle)
{
	MoodycamelCQPtr q = reinterpret_cast<MoodycamelCQPtr>(handle);
  assert (typeid (*q) == typeid(moodycamel::ConcurrentQueue<void *>));
  moodycamel::ConsumerToken * ctok = new moodycamel::ConsumerToken(*q);
  return ctok;
}

}
