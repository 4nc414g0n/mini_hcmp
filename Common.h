//#pragma once
//#include <iostream>
////#include <algorithm>//��<windows.h>��ͻ
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
////static const ���� #define( effective c++ ����02 )
//static const size_t SMALL_OBJ_MAX_BYTE = 256 * 1024;//С���� < 256kb
//static const size_t BIG_OBJ_MAX_BYTE = 1024 * 1024;//256kb < �ж��� < 1MB,
//													//����� > 1MB
//#ifdef _WIN64
//	typedef unsigned long long PAGE_ID;
//#elif _WIN32
//	typedef size_t PAGE_ID;
//#else// linux
//
//#endif
//static const size_t PAGE_SHIFT = 13;//8kbһҳ
//
//static const size_t MAXFREELIST = 208;//ThreadCache
//static const size_t MAXSPANLIST = 208;//CentralCache
//static const size_t MAXPAGELIST = 129;//PageCache 

//#ifdef _WIN32 //windows�� _WIN32(64/32���д�macro)
//	#include<windows.h> //VirtualAlloc
//#else //Linux
//	#include <unistd.h>//brk
//	#include <sys/mman.h>//mmap
//	#include <pthread.h>//pthread_spin_lock, _thread
//#endif 

//void* SystemAlloc(size_t size)//��װͳһ�ӿ�
//{
//	//��ҳ����
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
	//ͷ�壨ȡǰ4�ֽڨCǿת������(ȫ�ֺ�������)�����Լ��
	//ȡָ����ֽ�ͳһ��װ
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
	//ͷɾ ���Լ��
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
	void PushRange(void* start, void* end, size_t n)//��CentralCache������ȡ��FreeList����������ȥThreadCache��Ҫʹ�õ���һ��Node��ʣ�µĲ���ThreadCache��FreeList�����Ӧ��Ͱ��
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
	void PopRange(void*& start, void*& end, size_t n)//�黹��CentralCache��ע�⣬start��end�������ã������޸ģ����ú���֮����Ҫ�õ���
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
		_freelist = NextNode(end);//end����һ������_freelist
		NextNode(end) = nullptr;//Pop����_freelist�����������ӹ�ϵ�ж�
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
	//�п�
	bool Empty()
	{
		return _freelist == nullptr;
	}

	size_t& MaxSize()//�������������ƣ���������Ŀ�Ŀ��޸�
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
	size_t _maxsize = 1; //��������������
	size_t _size = 0;//���ں�_maxsize�Ƚϣ�size>_maxsize�ͽ��л����߼���
	
};

struct Span {

	//�洢Span���������
	PAGE_ID _pageId = 0; // ����ڴ���ʼҳ��ҳ��
	size_t  _n = 0;      // ҳ��

	Span* _next=nullptr;
	Span* _prev=nullptr;

	size_t _useCount = 0;//��¼�����ThreadCache��span����+Num���黹��ʱ��-Num��Ϊ0�ͼ����黹��PageCache

	void* _freelist = nullptr;//�кõ�FreeList����

	bool _inUse=false;//��PageCahceͨ��FetchNewSpan��õ�span״̬��Ϊʹ���У���ֹPageCache�ϲ�spanʱ����ϲ�

	size_t _objSize = 0;  // �кõ�С����Ĵ�С

};
class SpanList {//��ͷ˫��ѭ������
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
	void Insert(Span* pos, Span* newspan)//posǰ����newspan
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
		assert(pos!=_head);//ͷָ�벻�ܱ�Erase

		// 1�������ϵ�
		// 2���鿴ջ֡
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
	std::mutex _spanlist_mtx;//���������Ľ���������
};

class SizeClass {
	//�򻯵Ĺ���ӳ����򣨿���������10%���ҵ�����Ƭ�˷ѣ�
	// [1,128]					8byte����	    freelist[0,16)
	// [128+1,1024]				16byte����	    freelist[16,72)
	// [1024+1,8*1024]			128byte����	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)
	
	//tcmalloc��һ��85��sizeclass
public:
	static size_t _RoundUp(size_t size, size_t alignSize)//alignSize������(byte)
	{
		//_RoundUpȡ��(λ�������� + - * / ��)
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
	static size_t _GetIndex(size_t size, size_t alignShift)//2 raised to the power of 'align_shift' 2�� align_shift�η�
	{
#ifdef _WIN64
		return ((size + (1i64 << alignShift) - 1) >> alignShift) - 1;
#elif _WIN32
		return ((size + (1 << alignShift) - 1) >> alignShift) - 1;
#endif // _WIN64
	}
	static size_t GetIndex(size_t size)
	{
		//�д����ֱ��ȥPageCache��ϵͳ����
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