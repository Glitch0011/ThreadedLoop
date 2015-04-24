#include <atomic>
#include <vector>

#include <ThreadedLoop.h>

void SumExample()
{
	std::atomic<int> sum;
	std::vector<int> toSum;

	for (int i = 0; i < 1000; i++)
		toSum.push_back(i + 1);

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
}

struct Boid
{
	double x, y, vx, vy;

	Boid(double x, double y, double vx, double vy) : x(x), y(y), vx(vx), vy(vy) { }
};

void BoidExample()
{
	std::vector<Boid> boids;

	for (int i = 0; i < 1000; i++)
		boids.push_back(Boid(0, 0, 1, 1));

	{
		ThreadedLoop<Boid> loop([](Boid* start, Boid* end)
		{
			do
			{
				start->x += start->vx;
				start->y += start->vy;

			} while (++start != end);

		}, &boids);

		loop.RunAndWait();
	}
}

int main()
{
	SumExample();

	BoidExample();

	return 0;
}