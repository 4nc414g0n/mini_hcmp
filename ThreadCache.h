#pragma once
#include "Common.h"

class ThreadCache {
public:
	void* Allocate(size_t size);
	void ListTooLong(FreeList& list, size_t size);
	void Deallocate(void* ptr, size_t size);
	void* FetchFromCentralCache(size_t idx, size_t size);
	void* StealFromPeer();
	//ObjectPool<ThreadCache> TCPool;
private:
	FreeList _freelist_array[MAXFREELIST];
	

};//写法1：声明 *pTLSThreadCache 为thread local storage默认为nullptr


//TLS thread local storage 注意：这里简化了，only static（tcmalloc中 dynamic为主，static为辅）
#ifdef _WIN32 //windows __declspec双下划线更标准
	static __declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//写法2：

#else//配合写法2：
	//linux static tls GCC语言级别(非API)...
	static __thread ThreadCache* pTLSThreadCache = nullptr;
	//pthread库：API(动态)
	//int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
	//int pthread_key_delete(pthread_key_t key);
	//void* pthread_getspecific(pthread_key_t key);
	//int pthread_setspecific(pthread_key_t key, const void* value);
#endif