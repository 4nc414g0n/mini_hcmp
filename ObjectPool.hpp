#pragma once
#include "macros.h"

template<class T>
class ObjectPool {
public:
	T* OP_new()
	{
		T* ret = nullptr;

		if (_freelist)//自由链表不为空直接从里面取（注意这里不用判断_freelist空间是否够用,delete还回来的一定是分配时的大小，
						//即next也一定取得的是下一个sizeof(T)大小的空间）
		{
			//void*万能指针 32位下4字节 64位下8字节(截取_freelist的头指针大小个字节)
			void* next = NextNode(_freelist);
			ret = (T*)_freelist;
			_freelist = next;
		}
		else //自由链表空
		{
			//_remainspacesize若小于sizeof(T),直接向系统申请(调用windowsAPI VirtualAlloc (malloc底层也是调用的这个函数))
			if (_remainspacesize < sizeof(T))
			{
				_remainspacesize = 128 * 1024;//一般申请128kb
				size_t kpages = _remainspacesize>>PAGE_SHIFT;//一般申请128kb
				_memory = (char*)SystemAlloc(kpages);
				/*if (_memory == nullptr)
					throw std::bad_alloc;*/
			}
			//_remainspacesize大于小于sizeof(T)的统一逻辑
			ret = (T*)_memory;

			//为了正确形成_freelist链表(防止T对象的大小小于一个指针的大小)
			_memory += sizeof(T)<sizeof(void*)?sizeof(void*):sizeof(T);
			_remainspacesize -= sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//_memory += sizeof(T);
			//_remainspacesize -= sizeof(T);

		}
		//定位new一般是配合内存池使用的，因为内存池分配出来的空间没有初始化，
		//因此如果需要在这块内存池分配出来的空间上构造自定义类型的对象，
		//里面可能有其他自定义对象，需要调用对应的构造函数，需要使用定位new显式调用构造函数构造目标对象
		if (ret == nullptr)
			cout << 1 << endl;
		new(ret)T;//注意这里对于PageCache和CentralCache不能定位new，构造函数私有化
		return ret;//遗留问题：定长内存池返回处nullptr？？？？
	}
	void OP_delete(T* ptr)
	{
		assert(ptr);
		//显式调用析构函数(T可能为自定义对象，里面可能有其他自定义对象，需要调用对应的析构函数才能释放)
		ptr->~T();

		//头插
		NextNode(ptr) = _freelist;//在ptr取一个指针大小，将值置为_freelist, 即指向_freelist  *(void**)是为了满足不同平台需求,
		//*(int**)ptr = (int*)_freelist;//也可*(int**)...最后不管是void* int* char*都是一个指针大小个字节	
		_freelist = ptr;//_freelist永远是头
	}
private:
	char* _memory = nullptr;//向堆申请的memory
	size_t _remainspacesize = 0;//memory剩余的空间大小(byte)
	void* _freelist = nullptr;//自由链表（管理归还的内存对象）
};