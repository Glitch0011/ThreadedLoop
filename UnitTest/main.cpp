#include <atomic>
#include <vector>

#include <ThreadedLoop.h>
#include <UnitTest.h>

void SumExample()
{
	std::vector<int> toSum;

	for (int i = 0; i < 1000; i++)
		toSum.push_back(i + 1);

	std::atomic<int> sum;
	{
		ThreadedLoop<int> loop([&sum](int* start, int* end)
		{
			do
			{
				sum += *start;

			} while (++start != end);
		}, &toSum);

		loop.RunAndWait();
	}

	UnitTest::TestEqual<int>([=]()
	{
		int singleThreadSum = 0;

		for (int i = 0; i < 1000; i++)
			singleThreadSum += i + 1;

		return singleThreadSum;
	}, sum.load());
}

struct Boid
{
	double x, y, vx, vy;

	Boid(double x, double y, double vx, double vy) : x(x), y(y), vx(vx), vy(vy) { }
};

void BoidExample()
{
	std::vector<Boid> boids;

	//Setup boids
	for (int i = 0; i < 100000; i++)
		boids.push_back(Boid(0, 0, 1, 1));

	//Get time for single-thread
	auto singleThreadTime = UnitTest::TestTime([&]()
	{
		for (auto& i : boids)
		{
			i.x += i.vx;
			i.y += i.vy;
			i.vx /= sqrt((i.vx * i.vx) + (i.vy * i.vy));
			i.vy /= sqrt((i.vx * i.vx) + (i.vy * i.vy));
		}
	});

	//Setup threads outside of test framework
	ThreadedLoop<Boid> loop([](Boid* i, Boid* end)
	{
		do
		{
			i->x += i->vx;
			i->y += i->vy;
			i->vx /= sqrt((i->vx * i->vx) + (i->vy * i->vy));
			i->vy /= sqrt((i->vx * i->vx) + (i->vy * i->vy));

		} while (++i != end);

	}, &boids);

	//Run it once just to make sure all threads are ready
	loop.RunAndWait();

	//Perform test
	auto multiThreadTime = UnitTest::TestTime([&]()
	{
		loop.RunAndWait();
	});

	//Check to see that the multi-threaded took less time
	UnitTest::TestLessThan<std::chrono::microseconds>([&](){ return multiThreadTime; }, singleThreadTime);
}

int main()
{
	SumExample();

	BoidExample();

	return 0;
}