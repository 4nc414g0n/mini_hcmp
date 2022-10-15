#include "macros.h"
#include "ConcurrentPoolAlloc.h"

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&, k]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(malloc(16));
					v.push_back(malloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	cout << nworks << " Threads concurrent execute for " << rounds << " rounds，each rounds malloc " << ntimes << " times which costs：" << malloc_costtime << "ms" << endl;

	cout << nworks << " Threads concurrent execute for " << rounds << " rounds，each rounds free " << ntimes << " times which costs：" << free_costtime << "ms" << endl;

	cout << nworks << " Threads concurrent execute malloc & free for " << nworks * rounds * ntimes << " times which costs：" << malloc_costtime + free_costtime << "ms in total" << endl;
}


// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(ConcurrentAlloc(16));
					v.push_back(ConcurrentAlloc((16 + i) % 8192 + 1));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}


	cout << nworks << " Threads concurrent execute for " << rounds << " rounds，each rounds concurrent alloc " << ntimes << " times which costs：" << malloc_costtime << "ms" << endl;

	cout << nworks << " Threads concurrent execute for " << rounds << " rounds，each rounds concurrent dealloc " << ntimes << " times which costs：" << free_costtime << "ms" << endl;

	cout << nworks << " Threads concurrent execute concurrent alloc&dealloc for " << nworks * rounds * ntimes << " times which costs：" << malloc_costtime + free_costtime << "ms in total" << endl;

}

int main()
{
	size_t n = 1000;
	cout << "------------------------------         mini_hcmp       ------------------------------" << endl;
	BenchmarkConcurrentMalloc(n, 5, 100);
	cout << endl << endl;


	//cout << "------------------------------           malloc        ------------------------------" << endl;

	//BenchmarkMalloc(n, 5, 100);
	

	return 0;
}