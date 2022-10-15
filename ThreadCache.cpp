#pragma once

#include "ThreadCache.h"//ע���ظ�����
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
	//��ȡ�������㷨��һ��ֻ��Ҫһ������Ϊ�˼���ÿ�δ�CentralCacheȡ�ռ䣬�������������ģ�һ�ο��Ը����(����������)��
	// ����С��һ��������ȡһ��
	// �����һ��������ȡһ��
	size_t num = SMALL_OBJ_MAX_BYTE / size;
	//����BatchNum��С���ܵ���2������ܸ���512
	auto LimitContral = [&num]() {if (num < 2) num = 2; if (num > 512) num = 512; return num; };

	size_t BatchNum = min(_freelist_array[idx].MaxSize(), LimitContral());
	if (BatchNum == _freelist_array[idx].MaxSize())//δ����[2, 512]���䣬����������
	{
		_freelist_array[idx].MaxSize() += 1;//���������ٶȣ����Ե���
	}
	void* start = nullptr;
	void* end = nullptr;
	size_t ActualNum = CentralCache::GetInstance()->FetchRangeFreeListForThreadCache(start, end, BatchNum, size);
	assert(ActualNum >= 1);//���뵽������Ҫһ��Node

	

	if (ActualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else//����start��ͬʱ��ʣ�µ�ActualNum-1��Node����ThreadCache��FreeList�����Ӧ��Ͱ
	{
		//start->next->...->end  ��next->...->end����Ͱ
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