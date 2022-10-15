#pragma once
#include "CentralCache.h"
#include "PageCache.h"

//singleton_pattern
std::mutex CentralCache::_mtx;
//ObjectPool<CentralCache> CentralCache::CCPool;
//CentralCache* CentralCache::CentralCacheInstance = nullptr;
CentralCache CentralCache::CentralCacheInstance;
CentralCache* CentralCache::GetInstance()//注意：懒汉不行只能饿汉！！！！！
{
	/*if (CentralCacheInstance == nullptr)
	{
		CentralCache::_mtx.lock();
		if (CentralCacheInstance == nullptr)
		{
			CentralCacheInstance =new CentralCache;
		}
		CentralCache::_mtx.unlock();
	}*/
	return &CentralCacheInstance;
}

//获取一个非空的Span
Span* CentralCache::FetchOneSpan(size_t size)
{
	size_t idx = SizeClass::GetIndex(size);
	//_spanlist_array[idx]
	// 查看当前的spanlist中是否有还有未分配对象的span（只要span里面还剩哪怕一个freelist节点都可以）

	Span* it = _spanlist_array[idx].Begin();//模拟迭代器遍历_spanlist_array[idx]
	while (it != _spanlist_array[idx].End())
	{
		if (it->_freelist != nullptr)
			return it;
		else
			it = it->_next;
	}
	// 走到这里说明没有空闲span了，只能找pagecache要(加解锁)

	_spanlist_array[idx].Get_mtx().unlock();//线程释放内存对象回来，不会阻塞

	PageCache::GetInstance()->_mtx.lock();
	Span* span = PageCache::GetInstance()->FetchNewSpan(size);
	PageCache::GetInstance()->_mtx.unlock();


	// 计算span的大块内存的起始地址和大块内存的大小(字节数)
	char* start = (char*)((span->_pageId) << PAGE_SHIFT);//页号算内存地址 除8k
	size_t byte = (span->_n) << PAGE_SHIFT;//页数乘8k
	char* end = start + byte;//定义为char*方便向后按一字节移动

	// 把大块内存切成自由链表链接起来
	// 1、先切一块下来去做头，方便尾插
	span->_freelist = (void*)start;
	void* cur = (void*)start;
	start += size;//一个FreeList节点大小为size
	int i = 0;
	while (start < end)
	{
		i++;
		NextNode(cur) = start;
		cur = NextNode(cur);
		start += size;
	}

	NextNode(cur) = nullptr;

	//// 1、条件断点
	//// 2、疑似死循环，可以中断程序，程序会在正在运行的地方停下来
	//int j = 0;
	//cur = span->_freelist;
	//while (cur)
	//{
	//	cur = NextNode(cur);
	//	++j;
	//}

	//if (j != (byte / size))
	//{
	//	int x = 0;
	//}


	_spanlist_array[idx].Get_mtx().lock();//操作桶，重新加上桶锁

	// 切好span以后，需要把span挂到桶里面去的时候
	_spanlist_array[idx].PushFront(span);
	return span;
}

//这里start和end传引用，方便FetchFromCentralCache使用
size_t CentralCache::FetchRangeFreeListForThreadCache(void*& start, void*& end, size_t BatchNum, size_t size)
{

	size_t idx = SizeClass::GetIndex(size);

	_spanlist_array[idx].Get_mtx().lock();
	Span* span = FetchOneSpan(size);

	assert(span);
	assert(span->_freelist);
	void* cur = span->_freelist;
	size_t i = 0;
	size_t ActualNum = 1;

	start = cur;
	while (i < BatchNum - 1 && NextNode(cur))//start向后走BatchNum-1个Node到达end，但可能此span拆分成的FreeList节点不足BatchNum-1个
	{
		cur = NextNode(cur);
		ActualNum++;
		i++;
	}
	end = cur;
	//更新span的_freelist自由链表
	span->_freelist = NextNode(cur);
	//截断end与之后freelist节点的链接
	NextNode(end) = nullptr;


	span->_useCount += ActualNum;//记录分给ThreadCache多少（为0表示全部归还）

	int j = 0;
	cur = start;
	while (cur)
	{
		cur = NextNode(cur);
		++j;
	}

	if (j != ActualNum)
	{
		int x = 0;
	}

	_spanlist_array[idx].Get_mtx().unlock();

	return ActualNum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	//还回Span的Freelist小块内存顺序都已经乱了，直接置空不用处理
	size_t idx = SizeClass::GetIndex(size);

	_spanlist_array[idx].Get_mtx().lock();

	while (start)
	{
		void* next = NextNode(start);

		Span* span = PageCache::GetInstance()->MapToSpan(start);
		NextNode(start) = span->_freelist;//头插入对应的span
		span->_freelist = start;
		span->_useCount--;

		if (span->_useCount == 0)//已全部归还
		{
			_spanlist_array[idx].Erase(span);//从_spanlist_array中移除
			span->_freelist = nullptr;
			span->_prev = nullptr;
			span->_next = nullptr;

			_spanlist_array[idx].Get_mtx().unlock();//其他线程可以对_spanist_array[idx]进行操作

			PageCache::GetInstance()->_mtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_mtx.unlock();

			_spanlist_array[idx].Get_mtx().lock();
		}
		start = next;
		
	}
	_spanlist_array[idx].Get_mtx().unlock();

}