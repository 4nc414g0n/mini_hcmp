//#pragma once
//#include "ObjectPool.hpp"
//
//class test
//{
//public:
//	test()
//		:_a(0)
//	{}
//private:
//	short _a;
//};
//int main()
//{
//	const size_t N = 100000;//N¥Œ…Í«Î
//
//	std::vector<test*> v1;
//	size_t start1 = clock();
//	for (int i = 0; i < N; ++i)
//	{
//		v1.push_back(new test);
//	}
//	for (int i = 0; i < N; ++i)
//	{
//		delete v1[i];
//	}
//	size_t end1 = clock();
//
//	std::vector<test*> v2;
//	ObjectPool<test> ObjPool;
//
//	size_t start2 = clock();
//	for (int i = 0; i < N; i++)
//	{
//		v2.push_back(ObjPool.OP_new());
//	}
//	for (int i = 0; i < N; i++)
//	{
//		ObjPool.OP_delete(v2[i]);
//	}
//	size_t end2 = clock();
//
//	cout << "new/delete(malloc): " << end1 - start1 << endl;
//	cout << "ObjectPool:OP_new/OP_delete(VirtualAlloc): "<<end2 - start2 << endl;
//}