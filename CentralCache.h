#pragma once
#include "Common.h"


class CentralCache {
public:
	static CentralCache* GetInstance();

	//class RecycleCentralCache {
	//public:
	//	~RecycleCentralCache()
	//	{
	//		if (CentralCacheInstance)
	//			CentralCache::CCPool.OP_delete(CentralCacheInstance);
	//	}
	//};
	//static RecycleCentralCache RCC;//��������Զ�����RecycleCentralCache���������ͷŵ�������

private:
	//singleton_pattern
	CentralCache()//������
	{}
	CentralCache(const CentralCache& copy) = delete;//ɾ����������
	CentralCache& operator=(const CentralCache& lvalue) = delete;//ɾ��operator=

	static CentralCache CentralCacheInstance;

	//static ObjectPool<CentralCache> CCPool;

public:
	// ��ȡһ���ǿյ�span
	Span* FetchOneSpan(size_t size);

	// ��CentralCache��ȡһ�������Ķ����thread cache
	size_t FetchRangeFreeListForThreadCache(void*& start, void*& end, size_t batchNum, size_t size);
	void ReleaseListToSpans(void* start, size_t size);

	static std::mutex _mtx;//�����̹߳�������
private:
	//spanlist
	SpanList _spanlist_array[MAXSPANLIST];
	//std::vector<SpanList> _spanlist_array;
};