#pragma once
#include "Common.h"
//#include "PageMap.h"

class PageCache {
public:
	static PageCache* GetInstance();

	/*class RecyclePageCache {
	public:
		~RecyclePageCache()
		{
			if (PageCacheInstance)
				PCPool.OP_delete(PageCacheInstance);
		}
	};
	static RecyclePageCache RPC;*/
private:
	PageCache() {}
	PageCache(const PageCache& copy) = delete;
	PageCache& operator=(const PageCache& lvalue) = delete;

	static PageCache PageCacheInstance;

	//static ObjectPool<PageCache> PCPool;
public:
	Span* FetchNewSpan(size_t size);
	Span* MapToSpan(void* obj);
	void ReleaseSpanToPageCache(Span* span);
	std::mutex _mtx;//既用于GetInstance也用于PageCache访问（static？？？）
private:
	SpanList _pagelist_arry[MAXPAGELIST];//129个元素，0号元素不存储，1~128下标对应1~128大页Span
	ObjectPool<Span> _SpanPool;
//#ifdef _WIN64
//	PageMap3<48 - PAGE_SHIFT> _pageMap;//64位下使用PageMap3
//#else
//	PageMap2<32 - PAGE_SHIFT> _pageMap;//32位下使用PageMap2
//#endif
	std::unordered_map<PAGE_ID, Span*> _pageMap;
};

