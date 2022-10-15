#pragma once

#include "ThreadCache.h"//注意重复包含
#include "CentralCache.h"


void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= SMALL_OBJ_MAX_BYTE);

	size_t alignsize = SizeClass::RoundUp(size);
	size_t idx = SizeClass::GetIndex(size);

	if (!_freelist_array[idx].Empty())
	{
		int i = 0;
		void* cur = _freelist_array[idx].Freelist();
		while (cur)
		{
			cur = NextNode(cur);
			++i;
		}

		if (_freelist_array[idx].Size() != i)
		{
			int x = 0;
		}
		return _freelist_array[idx].Pop();
	}
	else
	{
		return FetchFromCentralCache(idx, alignsize);
	}

	//TODO...
	//return nullptr;
}
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= SMALL_OBJ_MAX_BYTE);

	size_t idx = SizeClass::GetIndex(size);
	_freelist_array[idx].Push(ptr);
	if (_freelist_array[idx].Size() >= _freelist_array[idx].MaxSize())
	{
		//if (_freelist_array[idx].Size() > _freelist_array[idx].MaxSize())
		//{
		//	cout << 1 << endl;
		//}
		ListTooLong(_freelist_array[idx], size);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t idx, size_t size)
{
	//采取慢启动算法（一次只需要一个，但为了减少每次从CentralCache取空间，加锁解锁的消耗，一次可以给多个(慢启动控制)）
	// 对象小，一次批量多取一点
	// 对象大，一次批量少取一点
	size_t num = SMALL_OBJ_MAX_BYTE / size;
	//控制BatchNum最小不能低于2，最大不能高于512
	auto LimitContral = [&num]() {if (num < 2) num = 2; if (num > 512) num = 512; return num; };

	size_t BatchNum = min(_freelist_array[idx].MaxSize(), LimitContral());
	if (BatchNum == _freelist_array[idx].MaxSize())//未超过[2, 512]区间，继续慢启动
	{
		_freelist_array[idx].MaxSize() += 1;//慢启动的速度，可以调节
	}
	void* start = nullptr;
	void* end = nullptr;
	size_t ActualNum = CentralCache::GetInstance()->FetchRangeFreeListForThreadCache(start, end, BatchNum, size);
	assert(ActualNum >= 1);//申请到的至少要一个Node

	

	if (ActualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else//返回start，同时将剩下的ActualNum-1个Node链入ThreadCache的FreeList数组对应的桶
	{
		//start->next->...->end  将next->...->end链入桶
		void* next = NextNode(start);
		_freelist_array[idx].PushRange(next, end, ActualNum-1);

		return start;
	}
}


//void* ThreadCache::StealFromPeer()
//{
//	//...
//	return nullptr;
//}