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
	std::mutex _mtx;//������GetInstanceҲ����PageCache���ʣ�static��������
private:
	SpanList _pagelist_arry[MAXPAGELIST];//129��Ԫ�أ�0��Ԫ�ز��洢��1~128�±��Ӧ1~128��ҳSpan
	ObjectPool<Span> _SpanPool;
//#ifdef _WIN64
//	PageMap3<48 - PAGE_SHIFT> _pageMap;//64λ��ʹ��PageMap3
//#else
//	PageMap2<32 - PAGE_SHIFT> _pageMap;//32λ��ʹ��PageMap2
//#endif
	std::unordered_map<PAGE_ID, Span*> _pageMap;
};

