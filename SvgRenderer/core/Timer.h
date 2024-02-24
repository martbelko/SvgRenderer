#pragma once

#include <chrono>

namespace SvgRenderer {

	class Timer
	{
	public:
		Timer()
			: m_Start(std::chrono::high_resolution_clock::now()) {}

		void Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float ElapsedNanos()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
		}

		float ElapsedMillis()
		{
			return ElapsedNanos() / 1'000'000.0f;
		}
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

}