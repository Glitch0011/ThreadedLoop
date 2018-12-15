#pragma once

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <vector>

#define THREAD_COUNT 8

#ifdef WIN32
	#include <Windows.h>
#endif

template <class T> class ThreadedLoop
{
	typedef std::function<void(T* start, T* end)> ThreadedLoopFunction;

	ThreadedLoopFunction func;

	std::condition_variable calcWaits;

	volatile std::atomic<int> finished = 0;
	volatile bool die = false;

	std::mutex mutexie[THREAD_COUNT];
	volatile bool running[THREAD_COUNT];

	std::vector<std::thread> threads;
	std::condition_variable mainConditionalVariable;

	std::vector<T>* data = nullptr;

public:

	//Disable copying to control the thread-creation
	ThreadedLoop(ThreadedLoop& a) = delete;

	ThreadedLoop(ThreadedLoopFunction func, std::vector<T>* data = nullptr)
	{
		this->data = data;

		if (!this->data)
		{
		
		}

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
					
					auto totalBoids = data->size();

					auto perThread = (totalBoids / THREAD_COUNT);

					//If we're the final thread, add the remainder to the loop to finish all the boids
					auto perThreadWithRemainder = perThread + (i != (THREAD_COUNT - 1) ? 0 : (totalBoids % THREAD_COUNT));
					auto offset = perThread * i;

					auto start = (T*)data->data() + offset;
					auto end = start + perThreadWithRemainder;

					func(start, end);

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

private:
	void Run()
	{
		memset((void*)this->running, (int)true, sizeof(this->running));

		calcWaits.notify_all();
	}

	void Wait()
	{
		while (finished != THREAD_COUNT)
			std::this_thread::yield();

		finished = 0;
	}

public:
	void RunAndWait()
	{
		this->Run();
		this->Wait();
	}
};