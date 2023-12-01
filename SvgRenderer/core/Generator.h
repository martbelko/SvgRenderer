#pragma once

#include <coroutine>
#include <exception>
#include <string>
#include <vector>
#include <filesystem>

namespace SvgRenderer {

	template<typename T>
	struct Generator
	{
		struct promise_type
		{
			T current_data;

			Generator get_return_object()
			{
				return Generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			std::suspend_always initial_suspend() { return {}; }
			std::suspend_always final_suspend() noexcept { return {}; }

			void unhandled_exception()
			{
				std::terminate();
			}

			void return_void() {}

			std::suspend_always yield_value(const Data& data)
			{
				current_data = data;
				return {};
			}
		};

		std::coroutine_handle<promise_type> coro;

		Generator(std::coroutine_handle<promise_type> coro) : coro(coro) {}

		~Generator()
		{
			if (coro)
				coro.destroy();
		}

		const T& current()
		{
			return coro.promise().current_data;
		}

		bool MoveNext()
		{
			if (!coro.done())
			{
				coro.resume();
				return !coro.done();
			}

			return false;
		}
	};

}
