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
	

};//д��1������ *pTLSThreadCache Ϊthread local storageĬ��Ϊnullptr


//TLS thread local storage ע�⣺������ˣ�only static��tcmalloc�� dynamicΪ����staticΪ����
#ifdef _WIN32 //windows __declspec˫�»��߸���׼
	static __declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//д��2��

#else//���д��2��
	//linux static tls GCC���Լ���(��API)...
	static __thread ThreadCache* pTLSThreadCache = nullptr;
	//pthread�⣺API(��̬)
	//int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
	//int pthread_key_delete(pthread_key_t key);
	//void* pthread_getspecific(pthread_key_t key);
	//int pthread_setspecific(pthread_key_t key, const void* value);
#endif