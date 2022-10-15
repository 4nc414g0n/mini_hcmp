
#pragma once
#include <iostream>
//#include <algorithm>//与<windows.h>冲突
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <set>
#include <assert.h>
#include <time.h>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>


using std::cout;
using std::endl;

//static const 代替 #define( effective c++ 条款02 )
static const size_t SMALL_OBJ_MAX_BYTE = 256 * 1024;//小对象 < 256kb
static const size_t BIG_OBJ_MAX_BYTE = 1024 * 1024;//256kb < 中对象 < 1MB,
													//大对象 > 1MB
#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#else// linux

#endif
static const size_t PAGE_SHIFT = 13;//8kb一页

static const size_t MAXFREELIST = 208;//ThreadCache
static const size_t MAXSPANLIST = 208;//CentralCache
static const size_t MAXPAGELIST = 129;//PageCache 

#ifdef _WIN32 //windows中 _WIN32(64/32均有此macro)
#include<windows.h> //VirtualAlloc
#else //Linux
	#include <unistd.h>//brk
	#include <sys/mman.h>//mmap
	#include <pthread.h>//pthread_spin_lock, _thread
#endif 

inline static void* SystemAlloc(size_t kpages)//封装统一接口
{
	//按页申请
#ifdef _WIN32
	void* newmemory = VirtualAlloc(0, kpages << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	//void* newmemory = (void*)VirtualAlloc(0, kpages*(1>>12), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//mmap...
	void* ptr = mmap(0, kpages << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	//brk...

#endif
	if (newmemory == nullptr)
		throw std::bad_alloc();
	return newmemory;
}
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	//munmap:
#include "PageCache.h"
	Span* span = PageCache::GetInstance()->MapToSpan(ptr);
	size_t size = span->_objSize;
	munmap(ptr, size);
	// sbrk ...

#endif
}
static void*& NextNode(void* ptr)
{
	//assert(ptr);
	if (ptr == nullptr)
	{
		cout << ptr << endl;
	}
	return *((void**)ptr);
}