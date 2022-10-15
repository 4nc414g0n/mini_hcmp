#pragma once
#include "CentralCache.h"
#include "PageCache.h"

//singleton_pattern
std::mutex CentralCache::_mtx;
//ObjectPool<CentralCache> CentralCache::CCPool;
//CentralCache* CentralCache::CentralCacheInstance = nullptr;
CentralCache CentralCache::CentralCacheInstance;
CentralCache* CentralCache::GetInstance()//ע�⣺��������ֻ�ܶ�������������
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

//��ȡһ���ǿյ�Span
Span* CentralCache::FetchOneSpan(size_t size)
{
	size_t idx = SizeClass::GetIndex(size);
	//_spanlist_array[idx]
	// �鿴��ǰ��spanlist���Ƿ��л���δ��������span��ֻҪspan���滹ʣ����һ��freelist�ڵ㶼���ԣ�

	Span* it = _spanlist_array[idx].Begin();//ģ�����������_spanlist_array[idx]
	while (it != _spanlist_array[idx].End())
	{
		if (it->_freelist != nullptr)
			return it;
		else
			it = it->_next;
	}
	// �ߵ�����˵��û�п���span�ˣ�ֻ����pagecacheҪ(�ӽ���)

	_spanlist_array[idx].Get_mtx().unlock();//�߳��ͷ��ڴ�����������������

	PageCache::GetInstance()->_mtx.lock();
	Span* span = PageCache::GetInstance()->FetchNewSpan(size);
	PageCache::GetInstance()->_mtx.unlock();


	// ����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С(�ֽ���)
	char* start = (char*)((span->_pageId) << PAGE_SHIFT);//ҳ�����ڴ��ַ ��8k
	size_t byte = (span->_n) << PAGE_SHIFT;//ҳ����8k
	char* end = start + byte;//����Ϊchar*�������һ�ֽ��ƶ�

	// �Ѵ���ڴ��г�����������������
	// 1������һ������ȥ��ͷ������β��
	span->_freelist = (void*)start;
	void* cur = (void*)start;
	start += size;//һ��FreeList�ڵ��СΪsize
	int i = 0;
	while (start < end)
	{
		i++;
		NextNode(cur) = start;
		cur = NextNode(cur);
		start += size;
	}

	NextNode(cur) = nullptr;

	//// 1�������ϵ�
	//// 2��������ѭ���������жϳ��򣬳�������������еĵط�ͣ����
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


	_spanlist_array[idx].Get_mtx().lock();//����Ͱ�����¼���Ͱ��

	// �к�span�Ժ���Ҫ��span�ҵ�Ͱ����ȥ��ʱ��
	_spanlist_array[idx].PushFront(span);
	return span;
}

//����start��end�����ã�����FetchFromCentralCacheʹ��
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
	while (i < BatchNum - 1 && NextNode(cur))//start�����BatchNum-1��Node����end�������ܴ�span��ֳɵ�FreeList�ڵ㲻��BatchNum-1��
	{
		cur = NextNode(cur);
		ActualNum++;
		i++;
	}
	end = cur;
	//����span��_freelist��������
	span->_freelist = NextNode(cur);
	//�ض�end��֮��freelist�ڵ������
	NextNode(end) = nullptr;


	span->_useCount += ActualNum;//��¼�ָ�ThreadCache���٣�Ϊ0��ʾȫ���黹��

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
	//����Span��FreelistС���ڴ�˳���Ѿ����ˣ�ֱ���ÿղ��ô���
	size_t idx = SizeClass::GetIndex(size);

	_spanlist_array[idx].Get_mtx().lock();

	while (start)
	{
		void* next = NextNode(start);

		Span* span = PageCache::GetInstance()->MapToSpan(start);
		NextNode(start) = span->_freelist;//ͷ�����Ӧ��span
		span->_freelist = start;
		span->_useCount--;

		if (span->_useCount == 0)//��ȫ���黹
		{
			_spanlist_array[idx].Erase(span);//��_spanlist_array���Ƴ�
			span->_freelist = nullptr;
			span->_prev = nullptr;
			span->_next = nullptr;

			_spanlist_array[idx].Get_mtx().unlock();//�����߳̿��Զ�_spanist_array[idx]���в���

			PageCache::GetInstance()->_mtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_mtx.unlock();

			_spanlist_array[idx].Get_mtx().lock();
		}
		start = next;
		
	}
	_spanlist_array[idx].Get_mtx().unlock();

}