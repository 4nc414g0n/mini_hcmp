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
	//static RecycleCentralCache RCC;//程序结束自动调用RecycleCentralCache析构函数释放单例对象

private:
	//singleton_pattern
	CentralCache()//防构造
	{}
	CentralCache(const CentralCache& copy) = delete;//删除拷贝构造
	CentralCache& operator=(const CentralCache& lvalue) = delete;//删除operator=

	static CentralCache CentralCacheInstance;

	//static ObjectPool<CentralCache> CCPool;

public:
	// 获取一个非空的span
	Span* FetchOneSpan(size_t size);

	// 从CentralCache获取一定数量的对象给thread cache
	size_t FetchRangeFreeListForThreadCache(void*& start, void*& end, size_t batchNum, size_t size);
	void ReleaseListToSpans(void* start, size_t size);

	static std::mutex _mtx;//所有线程共享互斥锁
private:
	//spanlist
	SpanList _spanlist_array[MAXSPANLIST];
	//std::vector<SpanList> _spanlist_array;
};