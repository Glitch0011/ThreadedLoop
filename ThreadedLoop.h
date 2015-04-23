#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>

#define THREAD_COUNT 8

typedef std::function<void(int, int, int)> ThreadedLoopFunction;

class ThreadedLoop
{
	ThreadedLoopFunction func;

	std::condition_variable calcWaits;

	volatile std::atomic<int> finished = 0;
	volatile std::atomic<int> waiting = 0;

	volatile bool die = false;

	std::mutex mutexie[THREAD_COUNT];
	volatile bool running[THREAD_COUNT];

	std::mutex mainThreadMutex;
	std::vector<std::thread> threads;
	std::condition_variable mainConditionalVariable;

public:

	//Required copy constructor
	ThreadedLoop(ThreadedLoop& a) = delete;

	ThreadedLoop(ThreadedLoopFunction func, int totalBoids)
	{
		memset((void*)this->running, 0,  sizeof(bool) * THREAD_COUNT);

		for (int i = 0; i < THREAD_COUNT; i++)
		{
			threads.push_back(std::thread([=]()
			{
#ifdef WIN32
				SetThreadAffinityMask(GetCurrentThread(), 1 << i);
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
				while (true)
				{
					std::unique_lock<std::mutex> lck(mutexie[i]);

					if (!running[i])
						calcWaits.wait(lck);

					if (die)
					{
						finished++;
						this->running[i] = false;
						mainConditionalVariable.notify_one();
						return;
					}
					
					
					auto perThread = (totalBoids / THREAD_COUNT);
					auto offset = perThread * i;

					//If we're the final thread, add the remainder to the loop to finish all the boids
					func(i, perThread + (i != (THREAD_COUNT - 1) ? 0 : (totalBoids % THREAD_COUNT)), offset);

					finished++;
					this->running[i] = false;
					mainConditionalVariable.notify_one();
				}
			}));
		}
	}

	~ThreadedLoop()
	{
		this->die = true;
		this->Run();

		for (auto& t : threads)
			t.join();
	}

	void Run()
	{
		memset((void*)this->running, (int)true, sizeof(this->running));

		calcWaits.notify_all();
	}

	void Wait()
	{
		while (finished != THREAD_COUNT)
			std::this_thread::yield();

		/*while (!mainConditionalVariable.wait_for(std::unique_lock<std::mutex>(mainThreadMutex), std::chrono::microseconds(10), [&]() { return finished == THREAD_COUNT; }))
		{
			std::this_thread::yield();
		}*/

		finished = 0;
	}

	void RunAndWait()
	{
		this->Run();
		this->Wait();
	}
};