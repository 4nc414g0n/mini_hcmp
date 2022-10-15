//#include "ThreadCache.h"
//#include "ConcurrentPoolAlloc.h"
//
//void Alloc1()
//{
//	for (size_t i = 0; i < 5; ++i)
//	{
//		void* ptr = ConcurrentAlloc(6);
//	}
//}
//
//void Alloc2()
//{
//	for (size_t i = 0; i < 5; ++i)
//	{
//		void* ptr = ConcurrentAlloc(7);
//	}
//}
//
//
//void TLSTest()
//{
//	std::thread t1(Alloc1);
//	t1.join();
//
//	std::thread t2(Alloc2);
//	t2.join();
//}
//
//void TestConcurrentAlloc1()
//{
//	void* p1 = ConcurrentAlloc(6);
//	void* p2 = ConcurrentAlloc(8);
//	void* p3 = ConcurrentAlloc(1);
//	void* p4 = ConcurrentAlloc(7);
//	void* p5 = ConcurrentAlloc(1024 * 2);
//	void* p6 = ConcurrentAlloc(8);
//	void* p7 = ConcurrentAlloc(124);
//
//
//	cout << p1 << endl;
//	cout << p2 << endl;
//	cout << p3 << endl;
//	cout << p4 << endl;
//	cout << p5 << endl;
//	cout << p6 << endl;
//	cout << p7 << endl;
//
//	ConcurrentFree(p1);
//	ConcurrentFree(p2);
//	ConcurrentFree(p3);
//	ConcurrentFree(p4);
//	ConcurrentFree(p5);
//	ConcurrentFree(p6);
//	ConcurrentFree(p7);
//}
//
//void TestConcurrentAlloc2()
//{
//	/*for (size_t i = 0; i < 1050; ++i)
//	{
//		void* p1 = ConcurrentAlloc(1);
//		cout << p1 << endl;
//	}
//
//	void* p2 = ConcurrentAlloc(20);
//	cout << p2 << endl;*/
//	
//}
//
//
//void MultiThreadAlloc1()
//{
//	std::vector<void*> v1;
//	for (size_t i = 0; i < 1040; ++i)
//	{
//		void* ptr = ConcurrentAlloc((16 + i) % 8192 + 1);
//		v1.push_back(ptr);
//	}
//
//	for (size_t i = 0; i < 1040; i++)
//	{
//		ConcurrentFree(v1[i]);
//	}
//}
//
//void MultiThreadAlloc2()
//{
//	std::vector<void*> v2;
//	for (size_t i = 0; i < 103; ++i)
//	{
//		void* ptr = ConcurrentAlloc((160 + i) % 8192 + 1);
//		v2.push_back(ptr);
//	}
//
//	for (size_t i = 0; i < 103; i++)
//	{
//		ConcurrentFree(v2[i]);
//	}
//}
//void MultiThreadAlloc3()
//{
//	std::vector<void*> v3;
//	for (size_t i = 0; i < 20; ++i)
//	{
//		void* ptr = ConcurrentAlloc(1029);
//		v3.push_back(ptr);
//	}
//
//	for (auto e : v3)
//	{
//		ConcurrentFree(e);
//	}
//}
//void MultiThreadAlloc4()
//{
//	std::vector<void*> v4;
//	for (size_t i = 0; i < 200; ++i)
//	{
//		void* ptr = ConcurrentAlloc(1029*4);
//		v4.push_back(ptr);
//	}
//
//	for (auto e : v4)
//	{
//		ConcurrentFree(e);
//	}
//}
//
//void TestMultiThread()
//{
//	std::thread t1(MultiThreadAlloc1);
//	std::thread t2(MultiThreadAlloc2);
//	std::thread t3(MultiThreadAlloc3);
//	std::thread t4(MultiThreadAlloc4);
//
//	t1.join();
//	t2.join();
//	t3.join();
//	t4.join();
//}
//int main()
//{
//	//TestObjectPool();
//	//TLSTest();
//
//	//TestConcurrentAlloc1();
//	//TestConcurrentAlloc2();
//	TestMultiThread();
//	return 0;
//}