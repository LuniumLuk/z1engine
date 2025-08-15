#pragma once
#include <random>
#include <limits>

namespace z1 {

	class Random
	{
	public:
		// returns a random float in [min, max)
		static float rfloat(float min = 0.0f, float max = 1.0f)
		{
			static thread_local std::mt19937 generator{ std::random_device{}() };
			std::uniform_real_distribution<float> distribution(min, max);
			return distribution(generator);
		}

		// returns a random double in [min, max)
		static double rdouble(double min = 0.0, double max = 1.0)
		{
			static thread_local std::mt19937 generator{ std::random_device{}() };
			std::uniform_real_distribution<double> distribution(min, max);
			return distribution(generator);
		}
	};

}
