#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"


static void* ConcurrentAlloc(size_t size)
{

	//TODO...
	if (size> SMALL_OBJ_MAX_BYTE)
	{
		PageCache::GetInstance()->_mtx.lock();
		Span* span = PageCache::GetInstance()->FetchNewSpan(size);
		PageCache::GetInstance()->_mtx.unlock();

		return (void*)(span->_pageId << PAGE_SHIFT);
	}
	// 通过TLS 每个pTLSThreadCache无锁的获取自己的专属的ThreadCache对象
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			ObjectPool<ThreadCache> TCPool;
			pTLSThreadCache = TCPool.OP_new();//脱离使用new  OP_delete ??

		}
		//cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
		return pTLSThreadCache->Allocate(size);
	}
}
static void ConcurrentFree(void* ptr)
{
	assert(ptr);
	assert(pTLSThreadCache);
	Span* span = PageCache::GetInstance()->MapToSpan(ptr);//MapToSpan内使用RAII自动管理资源（加解锁释放资源）,使用基数树后不用加锁
	size_t size = span->_objSize;
	if (size>SMALL_OBJ_MAX_BYTE)
	{
		PageCache::GetInstance()->_mtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_mtx.unlock();
	}
	else
	{
		pTLSThreadCache->Deallocate(ptr, size);
		//static ObjectPool<ThreadCache> TCPool;
		//pTLSThreadCache = TCPool.OP_delete();//脱离使用new  OP_delete ??
	}
}