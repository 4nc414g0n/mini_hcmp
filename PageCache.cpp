#pragma once
#include "PageCache.h"



//std::mutex PageCache::_mtx;//public
//ObjectPool<PageCache> PageCache::PCPool;
//PageCache* PageCache::PageCacheInstance = nullptr;
PageCache PageCache::PageCacheInstance;
PageCache* PageCache::GetInstance()//ע�⣺�������У�ֻ�ܶ���
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
	size_t alignSize = SizeClass::RoundUp(size);//ע��Ҫ����ȡ��
	size_t kpages = alignSize >> PAGE_SHIFT;
	//��ռ䣨128pages���ϵĿռ䣩
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
	//128pages����
	else
	{
		size_t num = SMALL_OBJ_MAX_BYTE / size;
		auto LimitContral = [&num]() {if (num < 2) num = 2; if (num > 512) num = 512; return num; };//��FetchFromCentralCache��numһ�����߼�
		//����ҳ����һ����ϵͳ��ȡ����ҳ
		size_t PageNum = (LimitContral() * size) >> PAGE_SHIFT;//��PageNumҳ��SpanListȡ (һ�ζ�ȡһЩ)
		if (PageNum == 0) PageNum = 1;//��Ͳ�С��1ҳ

		assert(PageNum > 0 && PageNum < MAXPAGELIST);

		// �ȼ���PageNum��Ͱ������û��span
		if (!_pagelist_arry[PageNum].Empty())
		{
			Span* span = _pagelist_arry[PageNum].PopFront();


			//ע�⽨��id��span��ӳ�䣬����central cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			for (PAGE_ID i = 0; i < span->_n; i++)
			{
				_pageMap[span->_pageId + i] = span;
				//_pageMap.set(span->_pageId + i, span);
			}
			span->_inUse = true;//���ڷǿ���״̬
			span->_objSize = size;//�������ԣ�ΪConcurrentFree���ٲ���

			return span;
		}
		// ���һ�º����Ͱ������û��span������н����з�
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
					_pagelist_arry[left_span->_n].PushFront(left_span);//�зֺ󣬲����µ�Ͱ


					_pageMap[left_span->_pageId] = left_span;//ע��left_spanֻ��Ҫ��βҳ��left_pan��ӳ�䣨PageCache�ϲ���ʱ��ֻ����ǰһ���ͺ�һ����
					_pageMap[left_span->_pageId + left_span->_n - 1] = left_span;//ע�������-1����1000��1002����ҳ
					/*_pageMap.set(left_span->_pageId, left_span);
					_pageMap.set(left_span->_pageId + left_span->_n - 1, left_span);*/


					for (size_t i = 0; i < PageNum_span->_n; i++)//PageNum_span�е�ÿһ��ҳ�Ŷ�Ҫ��PageNum_span����ӳ�䣬
					{//��Ϊ��ThreadCache����CentralCache��freelist range���еĽڵ�����ڴ��ַ������������һ��page������Ҫҳ�Ų����ҵ�span
						_pageMap[PageNum_span->_pageId + i] = PageNum_span;
						//_pageMap.set(PageNum_span->_pageId + i, PageNum_span);
					}

					PageNum_span->_inUse = true;//���ڷǿ���״̬
					PageNum_span->_objSize = size;//�������ԣ�ΪConcurrentFree���ٲ���
					return PageNum_span;
				}
			}
		}

		// ��ʱ��ȥ�Ҷ�Ҫһ��128ҳ��span
		Span* NewSpan = _SpanPool.OP_new();

		void* ptr = SystemAlloc(MAXPAGELIST-1);

		NewSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;//>> PAGE_SHIFT�������ڴ��ַ��ҳ�ŵĶ�Ӧ��ϵ����
		NewSpan->_n = MAXPAGELIST - 1;
		_pagelist_arry[NewSpan->_n].PushFront(NewSpan);


		return FetchNewSpan(size);//ע����ƣ��ݹ�һ�Σ����������д��Ҫ������д��1���Ӻ���_FetchNewSpan()�ڼ��� д��2���ӵݹ���recursive_mutex
	}
}
Span* PageCache::MapToSpan(void* obj)
{
	PAGE_ID pid = ((PAGE_ID)obj) >> PAGE_SHIFT;//�ڴ��ַ��ҳ��
	//ʹ��RAII������Դ
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

	//ע�⣬ʹ�û���������Բ��ü�����
	/*auto ret = reinterpret_cast<Span*>(_pageMap.get(pid));
	assert(ret != nullptr);
	return ret;*/
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{

	size_t size = span->_objSize;
	//��ռ䣨128pages���ϵĿռ䣩
	if (span->_n> MAXPAGELIST-1)
	{
		void* ptr = (void*)((span->_pageId) << PAGE_SHIFT);
		SystemFree(ptr);
		_SpanPool.OP_delete(span);
		return;
	}
	else
	{
		//�ⲿ�Ѽ���
		
		while (1)
		{
			PAGE_ID previd = span->_pageId - 1;//ע������棡����
			std::unordered_map<PAGE_ID, Span*>::const_iterator it = _pageMap.find(previd);
			if (it == _pageMap.end())//�޴�span
			{
				break;
			}
			Span* prevspan = it->second;
			/*auto ret = reinterpret_cast<Span*>(_pageMap.get(previd));
			if (ret == nullptr)
				break;
			Span* prevspan = ret;*/
			if (prevspan->_inUse == true)//��span�ǿ���
			{
				break;
			}
			else
			{
				if (prevspan->_n + span->_n > MAXPAGELIST - 1)//��spanҳ�ʹ���128
				{
					break;
				}
				span->_n += prevspan->_n;
				span->_pageId = prevspan->_pageId;
				_pagelist_arry[prevspan->_n].Erase(prevspan);
				_SpanPool.OP_delete(prevspan);//ע��delete֮ǰnew��span
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
		_pagelist_arry[span->_n].PushFront(span);//ͷ��
		_pageMap[span->_pageId] = span;//���½���ӳ���ϵ
		_pageMap[span->_pageId + span->_n - 1] = span;//���½���ӳ���ϵ

		/*_pageMap.set(span->_pageId, span);
		_pageMap.set(span->_pageId + span->_n - 1, span);*/
		span->_inUse = false;//�黹������_inUseΪ����״̬
	}

	
}

