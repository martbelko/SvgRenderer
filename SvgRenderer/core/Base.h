#pragma once

#include <memory>

#define SR_DEBUGBREAK() __debugbreak()

#ifdef _DEBUG
	#define ENABLE_ASSERTS
#endif

#define SR_EXPAND_MACRO(x) x
#define SR_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

namespace SvgRenderer {

	template<typename T>
	using scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Assert.h"
#include "Log.h"
