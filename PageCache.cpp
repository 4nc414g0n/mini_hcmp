#pragma once
#include "PageCache.h"



//std::mutex PageCache::_mtx;//public
//ObjectPool<PageCache> PageCache::PCPool;
//PageCache* PageCache::PageCacheInstance = nullptr;
PageCache PageCache::PageCacheInstance;
PageCache* PageCache::GetInstance()//注意：懒汉不行，只能饿汉
{
	/*if (PageCacheInstance == nullptr)
	{
		PageCache::_mtx.lock();
		if (PageCacheInstance == nullptr)
			PageCacheInstance =PCPool.OP_new();
		PageCache::_mtx.unlock();
	}*/
	return &PageCacheInstance;
}

Span* PageCache::FetchNewSpan(size_t size)
{
	assert(size > 0);
	size_t alignSize = SizeClass::RoundUp(size);//注意要向上取整
	size_t kpages = alignSize >> PAGE_SHIFT;
	//大空间（128pages以上的空间）
	if (kpages>MAXPAGELIST-1)
	{	
		void* ptr = SystemAlloc(kpages);
		PAGE_ID pid = (PAGE_ID)ptr >> PAGE_SHIFT;
		Span* span = _SpanPool.OP_new();
		span->_objSize = size;
		span->_pageId = pid;

		_pageMap[pid] = span;
		//_pageMap.set(pid,span);

		return span;
	}
	//128pages以下
	else
	{
		size_t num = SMALL_OBJ_MAX_BYTE / size;
		auto LimitContral = [&num]() {if (num < 2) num = 2; if (num > 512) num = 512; return num; };//和FetchFromCentralCache中num一样的逻辑
		//计算页数，一次向系统获取几个页
		size_t PageNum = (LimitContral() * size) >> PAGE_SHIFT;//在PageNum页的SpanList取 (一次多取一些)
		if (PageNum == 0) PageNum = 1;//最低不小于1页

		assert(PageNum > 0 && PageNum < MAXPAGELIST);

		// 先检查第PageNum个桶里面有没有span
		if (!_pagelist_arry[PageNum].Empty())
		{
			Span* span = _pagelist_arry[PageNum].PopFront();


			//注意建立id和span的映射，方便central cache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < span->_n; i++)
			{
				_pageMap[span->_pageId + i] = span;
				//_pageMap.set(span->_pageId + i, span);
			}
			span->_inUse = true;//处于非空闲状态
			span->_objSize = size;//加上属性，为ConcurrentFree减少参数

			return span;
		}
		// 检查一下后面的桶里面有没有span，如果有将其切分
		else
		{
			for (size_t i = PageNum + 1; i < MAXPAGELIST; i++)
			{
				if (!_pagelist_arry[i].Empty())
				{
					Span* left_span = _pagelist_arry[i].PopFront();
					Span* PageNum_span = _SpanPool.OP_new();
					PageNum_span->_pageId = left_span->_pageId;
					PageNum_span->_n = PageNum;
					left_span->_pageId += PageNum;
					left_span->_n -= PageNum;
					_pagelist_arry[left_span->_n].PushFront(left_span);//切分后，插入新的桶


					_pageMap[left_span->_pageId] = left_span;//注意left_span只需要首尾页和left_pan的映射（PageCache合并的时候只是找前一个和后一个）
					_pageMap[left_span->_pageId + left_span->_n - 1] = left_span;//注意这里的-1：从1000到1002是三页
					/*_pageMap.set(left_span->_pageId, left_span);
					_pageMap.set(left_span->_pageId + left_span->_n - 1, left_span);*/


					for (size_t i = 0; i < PageNum_span->_n; i++)//PageNum_span中的每一个页号都要与PageNum_span建立映射，
					{//因为从ThreadCache还到CentralCache的freelist range其中的节点对象内存地址可能属于任意一个page，而需要页号才能找到span
						_pageMap[PageNum_span->_pageId + i] = PageNum_span;
						//_pageMap.set(PageNum_span->_pageId + i, PageNum_span);
					}

					PageNum_span->_inUse = true;//处于非空闲状态
					PageNum_span->_objSize = size;//加上属性，为ConcurrentFree减少参数
					return PageNum_span;
				}
			}
		}

		// 这时就去找堆要一个128页的span
		Span* NewSpan = _SpanPool.OP_new();

		void* ptr = SystemAlloc(MAXPAGELIST-1);

		NewSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;//>> PAGE_SHIFT仅仅是内存地址和页号的对应关系罢了
		NewSpan->_n = MAXPAGELIST - 1;
		_pagelist_arry[NewSpan->_n].PushFront(NewSpan);


		return FetchNewSpan(size);//注意设计（递归一次）如果不这样写需要加锁：写法1：子函数_FetchNewSpan()内加锁 写法2：加递归锁recursive_mutex
	}
}
Span* PageCache::MapToSpan(void* obj)
{
	PAGE_ID pid = ((PAGE_ID)obj) >> PAGE_SHIFT;//内存地址求页号
	//使用RAII管理资源
	std::unique_lock<std::mutex> lck(_mtx);
	std::unordered_map<PAGE_ID, Span*>::const_iterator it = _pageMap.find(pid);
	if (it == _pageMap.end())
	{
		assert(false);
		cout << "wrong" << endl;
		return nullptr;
	}
	else
		return it->second;

	//注意，使用基数树后可以不用加锁了
	/*auto ret = reinterpret_cast<Span*>(_pageMap.get(pid));
	assert(ret != nullptr);
	return ret;*/
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{

	size_t size = span->_objSize;
	//大空间（128pages以上的空间）
	if (span->_n> MAXPAGELIST-1)
	{
		void* ptr = (void*)((span->_pageId) << PAGE_SHIFT);
		SystemFree(ptr);
		_SpanPool.OP_delete(span);
		return;
	}
	else
	{
		//外部已加锁
		
		while (1)
		{
			PAGE_ID previd = span->_pageId - 1;//注意放里面！！！
			std::unordered_map<PAGE_ID, Span*>::const_iterator it = _pageMap.find(previd);
			if (it == _pageMap.end())//无此span
			{
				break;
			}
			Span* prevspan = it->second;
			/*auto ret = reinterpret_cast<Span*>(_pageMap.get(previd));
			if (ret == nullptr)
				break;
			Span* prevspan = ret;*/
			if (prevspan->_inUse == true)//此span非空闲
			{
				break;
			}
			else
			{
				if (prevspan->_n + span->_n > MAXPAGELIST - 1)//两span页和大于128
				{
					break;
				}
				span->_n += prevspan->_n;
				span->_pageId = prevspan->_pageId;
				_pagelist_arry[prevspan->_n].Erase(prevspan);
				_SpanPool.OP_delete(prevspan);//注意delete之前new的span
			}

		}
		while (1)
		{
			PAGE_ID nextid = span->_pageId + span->_n;
			std::unordered_map<PAGE_ID, Span*>::const_iterator it = _pageMap.find(nextid);
			if (it == _pageMap.end())
			{
				break;
			}
			Span* nextspan = it->second;
			/*auto ret = reinterpret_cast<Span*>(_pageMap.get(nextid));
			if (ret == nullptr)
				break;
			Span* nextspan = ret;*/
			if (nextspan->_inUse == true)
			{
				break;
			}
			else
			{
				if (nextspan->_n + span->_n > MAXPAGELIST - 1)
				{
					break;
				}
				span->_n += nextspan->_n;
				//span->_pageId = nextspan->_pageId;
				_pagelist_arry[nextspan->_n].Erase(nextspan);
				_SpanPool.OP_delete(nextspan);
			}
		}
		_pagelist_arry[span->_n].PushFront(span);//头插
		_pageMap[span->_pageId] = span;//重新建立映射关系
		_pageMap[span->_pageId + span->_n - 1] = span;//重新建立映射关系

		/*_pageMap.set(span->_pageId, span);
		_pageMap.set(span->_pageId + span->_n - 1, span);*/
		span->_inUse = false;//归还后重设_inUse为空闲状态
	}

	
}

