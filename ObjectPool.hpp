#pragma once
#include "macros.h"

template<class T>
class ObjectPool {
public:
	T* OP_new()
	{
		T* ret = nullptr;

		if (_freelist)//��������Ϊ��ֱ�Ӵ�����ȡ��ע�����ﲻ���ж�_freelist�ռ��Ƿ���,delete��������һ���Ƿ���ʱ�Ĵ�С��
						//��nextҲһ��ȡ�õ�����һ��sizeof(T)��С�Ŀռ䣩
		{
			//void*����ָ�� 32λ��4�ֽ� 64λ��8�ֽ�(��ȡ_freelist��ͷָ���С���ֽ�)
			void* next = NextNode(_freelist);
			ret = (T*)_freelist;
			_freelist = next;
		}
		else //���������
		{
			//_remainspacesize��С��sizeof(T),ֱ����ϵͳ����(����windowsAPI VirtualAlloc (malloc�ײ�Ҳ�ǵ��õ��������))
			if (_remainspacesize < sizeof(T))
			{
				_remainspacesize = 128 * 1024;//һ������128kb
				size_t kpages = _remainspacesize>>PAGE_SHIFT;//һ������128kb
				_memory = (char*)SystemAlloc(kpages);
				/*if (_memory == nullptr)
					throw std::bad_alloc;*/
			}
			//_remainspacesize����С��sizeof(T)��ͳһ�߼�
			ret = (T*)_memory;

			//Ϊ����ȷ�γ�_freelist����(��ֹT����Ĵ�СС��һ��ָ��Ĵ�С)
			_memory += sizeof(T)<sizeof(void*)?sizeof(void*):sizeof(T);
			_remainspacesize -= sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			//_memory += sizeof(T);
			//_remainspacesize -= sizeof(T);

		}
		//��λnewһ��������ڴ��ʹ�õģ���Ϊ�ڴ�ط�������Ŀռ�û�г�ʼ����
		//��������Ҫ������ڴ�ط�������Ŀռ��Ϲ����Զ������͵Ķ���
		//��������������Զ��������Ҫ���ö�Ӧ�Ĺ��캯������Ҫʹ�ö�λnew��ʽ���ù��캯������Ŀ�����
		if (ret == nullptr)
			cout << 1 << endl;
		new(ret)T;//ע���������PageCache��CentralCache���ܶ�λnew�����캯��˽�л�
		return ret;//�������⣺�����ڴ�ط��ش�nullptr��������
	}
	void OP_delete(T* ptr)
	{
		assert(ptr);
		//��ʽ������������(T����Ϊ�Զ��������������������Զ��������Ҫ���ö�Ӧ���������������ͷ�)
		ptr->~T();

		//ͷ��
		NextNode(ptr) = _freelist;//��ptrȡһ��ָ���С����ֵ��Ϊ_freelist, ��ָ��_freelist  *(void**)��Ϊ�����㲻ͬƽ̨����,
		//*(int**)ptr = (int*)_freelist;//Ҳ��*(int**)...��󲻹���void* int* char*����һ��ָ���С���ֽ�	
		_freelist = ptr;//_freelist��Զ��ͷ
	}
private:
	char* _memory = nullptr;//��������memory
	size_t _remainspacesize = 0;//memoryʣ��Ŀռ��С(byte)
	void* _freelist = nullptr;//������������黹���ڴ����
};