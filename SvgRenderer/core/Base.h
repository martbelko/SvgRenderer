#pragma once

#include <memory>

#ifdef _DEBUG
	#define DEBUGBREAK() __debugbreak()
	#define ENABLE_ASSERTS
#else
	#define DEBUGBREAK()
#endif

#define EXPAND_MACRO(x) x
#define STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

template<typename T>
using scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr scope<T> createScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr ref<T> createRef(Args&& ... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

#include "Assert.h"
#include "Log.h"
