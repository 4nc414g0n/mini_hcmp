//#pragma once
//#include <iostream>
////#include <algorithm>//与<windows.h>冲突
//#include <vector>
//#include <assert.h>
//#include <time.h>
//#include <vector>
//#include <thread>
//#include <mutex>
//
//using std::cout;
//using std::endl;
//
////static const 代替 #define( effective c++ 条款02 )
//static const size_t SMALL_OBJ_MAX_BYTE = 256 * 1024;//小对象 < 256kb
//static const size_t BIG_OBJ_MAX_BYTE = 1024 * 1024;//256kb < 中对象 < 1MB,
//													//大对象 > 1MB
//#ifdef _WIN64
//	typedef unsigned long long PAGE_ID;
//#elif _WIN32
//	typedef size_t PAGE_ID;
//#else// linux
//
//#endif
//static const size_t PAGE_SHIFT = 13;//8kb一页
//
//static const size_t MAXFREELIST = 208;//ThreadCache
//static const size_t MAXSPANLIST = 208;//CentralCache
//static const size_t MAXPAGELIST = 129;//PageCache 

//#ifdef _WIN32 //windows中 _WIN32(64/32均有此macro)
//	#include<windows.h> //VirtualAlloc
//#else //Linux
//	#include <unistd.h>//brk
//	#include <sys/mman.h>//mmap
//	#include <pthread.h>//pthread_spin_lock, _thread
//#endif 

//void* SystemAlloc(size_t size)//封装统一接口
//{
//	//按页申请
//	size_t kpages = (size >> PAGE_SHIFT);
//#ifdef _WIN32
//	void* newmemory = VirtualAlloc(0, kpages << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//	//void* newmemory = (void*)VirtualAlloc(0, kpages*(1>>12), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//#else
//	//brk,mmap...
//
//#endif
//	if (newmemory == nullptr)
//		throw std::bad_alloc();
//	return newmemory;
//}
#include "macros.h"
#pragma once
#include "ObjectPool.hpp"

class FreeList {
public:
	//头插（取前4字节–强转解引用(全局函数返回)）断言检查
	//取指针个字节统一封装
	void* Freelist()
	{
		return _freelist;
	}
	void Push(void* ptr)
	{
		assert(ptr);
		NextNode(ptr) = _freelist;
		_freelist = ptr;
		_size++;

		int i = 0;
		void* cur = _freelist;
		while (cur)
		{
			cur = NextNode(cur);
			++i;
		}

		if (_size != i)
		{
			int x = 0;
		}
	}
	//头删 断言检查
	void* Pop()
	{
		assert(_freelist);
		void* ret = _freelist;
		_freelist = NextNode(ret);
		_size--;

		int i = 0;
		void* cur = _freelist;
		while (cur)
		{
			cur = NextNode(cur);
			++i;
		}

		if (_size != i)
		{
			int x = 0;
		}
		return ret;
	}
	void PushRange(void* start, void* end, size_t n)//从CentralCache批量获取的FreeList自由链表（除去ThreadCache需要使用的那一个Node，剩下的插入ThreadCache的FreeList数组对应的桶）
	{
		assert(start);
		assert(end);
		NextNode(end) = _freelist;
		_freelist = start;
		_size += n;

		int i = 0;
		void* cur = _freelist;
		while (cur)
		{
			cur = NextNode(cur);
			++i;
		}

		if (_size != i)
		{
			int x = 0;
		}
	}
	void PopRange(void*& start, void*& end, size_t n)//归还给CentralCache（注意，start，end都是引用，可以修改，调用函数之后需要用到）
	{
		assert(n <= _size);
		start = _freelist;
		end = start;
		void* cur = NextNode(start);
		for (size_t i = 0; i < n - 1; i++)
		{
			cur = NextNode(cur);
			end = NextNode(end);
		}
		_freelist = NextNode(end);//end的下一个就是_freelist
		NextNode(end) = nullptr;//Pop掉的_freelist链与后面的链接关系切断
		_size -= n;

		int i = 0;
		cur = _freelist;
		while (cur)
		{
			cur = NextNode(cur);
			++i;
		}

		if (_size != i)
		{
			int x = 0;
		}
	}
	//判空
	bool Empty()
	{
		return _freelist == nullptr;
	}

	size_t& MaxSize()//用于慢启动控制，返回引用目的可修改
	{
		return _maxsize;
	}
	size_t Size()
	{
		return _size;
	}
	std::mutex _mtx;
private:
	void* _freelist=nullptr;
	size_t _maxsize = 1; //用于慢启动控制
	size_t _size = 0;//用于和_maxsize比较（size>_maxsize就进行回收逻辑）
	
};

struct Span {

	//存储Span对象的特征
	PAGE_ID _pageId = 0; // 大块内存起始页的页号
	size_t  _n = 0;      // 页数

	Span* _next=nullptr;
	Span* _prev=nullptr;

	size_t _useCount = 0;//记录分配给ThreadCache的span分配+Num，归还的时候-Num，为0就继续归还给PageCache

	void* _freelist = nullptr;//切好的FreeList链表

	bool _inUse=false;//向PageCahce通过FetchNewSpan获得的span状态置为使用中，防止PageCache合并span时将其合并

	size_t _objSize = 0;  // 切好的小对象的大小

};
class SpanList {//带头双向循环链表
public:
	SpanList()
	{
		_head = SpanPool.OP_new();
		_head->_next = _head;
		_head->_prev = _head;
	}
	Span* Begin()
	{
		return _head->_next;
	}
	Span* End()
	{
		return _head;
	}
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}
	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}
	void Insert(Span* pos, Span* newspan)//pos前插入newspan
	{
		assert(pos);
		assert(newspan);
		Span* prev = pos->_prev;
		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = pos;
		pos->_prev = newspan;

	}
	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos!=_head);//头指针不能被Erase

		// 1、条件断点
		// 2、查看栈帧
		if (pos == _head)
		{
			int x = 0;
		}

		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}
public:
	bool Empty()
	{
		return _head->_next == _head;
	}
	std::mutex& Get_mtx()
	{
		return _spanlist_mtx;
	}
private:
	Span* _head;
	//ObjectPool<Span> SpanPool;
	ObjectPool<Span> SpanPool;
	std::mutex _spanlist_mtx;//互斥锁（改进自旋锁）
};

class SizeClass {
	//简化的管理映射规则（控制整体在10%左右的内碎片浪费）
	// [1,128]					8byte对齐	    freelist[0,16)
	// [128+1,1024]				16byte对齐	    freelist[16,72)
	// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)
	
	//tcmalloc中一共85个sizeclass
public:
	static size_t _RoundUp(size_t size, size_t alignSize)//alignSize对齐数(byte)
	{
		//_RoundUp取整(位操作，比 + - * / 快)
		return ((size + alignSize - 1) & ~(alignSize - 1));
	}
	static size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8*1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64*1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8*1024);
		}
		else
		{
			//BIG_OBJ...TODO...
			return -1;
		}
	}
	static size_t _GetIndex(size_t size, size_t alignShift)//2 raised to the power of 'align_shift' 2的 align_shift次方
	{
#ifdef _WIN64
		return ((size + (1i64 << alignShift) - 1) >> alignShift) - 1;
#elif _WIN32
		return ((size + (1 << alignShift) - 1) >> alignShift) - 1;
#endif // _WIN64
	}
	static size_t GetIndex(size_t size)
	{
		//中大对象直接去PageCache或系统申请
		assert(size < SMALL_OBJ_MAX_BYTE);
		size_t IndexOffset[5] = { 16, 56, 56,56 };
		if (size <= 128) {
			return _GetIndex(size, 3);
		}
		else if (size <= 1024) {
			return _GetIndex(size - 128, 4) + IndexOffset[0];
		}
		else if (size <= 8 * 1024) {
			return _GetIndex(size - 1024, 7) + IndexOffset[1] + IndexOffset[0];
		}
		else if (size <= 64 * 1024) {
			return _GetIndex(size - 8 * 1024, 10) + IndexOffset[2] + IndexOffset[1] + IndexOffset[0];
		}
		else if (size <= 256 * 1024) {
			return _GetIndex(size - 64 * 1024, 13) + IndexOffset[3] + IndexOffset[2] + IndexOffset[1] + IndexOffset[0];
		}
		else 
		{
			assert(false);
		}
		return -1;
	}
};