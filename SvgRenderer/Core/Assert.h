#pragma once

#include "Base.h"
#include "Log.h"

#include <filesystem>

#define SR_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { ##type##CRITICAL(msg, __VA_ARGS__); SR_DEBUGBREAK(); } }
#define SR_INTERNAL_ASSERT_WITH_MSG(type, check, ...) SR_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define SR_INTERNAL_ASSERT_NO_MSG(type, check) SR_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", SR_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define SR_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define SR_INTERNAL_ASSERT_GET_MACRO(...) SR_EXPAND_MACRO( SR_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, SR_INTERNAL_ASSERT_WITH_MSG, SR_INTERNAL_ASSERT_NO_MSG) )

#ifdef ENABLE_ASSERTS
	#define SR_ASSERT(...) SR_EXPAND_MACRO( SR_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(SR_, __VA_ARGS__) )
#else
	#define SR_ASSERT(...)
#endif

#define SR_VERIFY(...) SR_EXPAND_MACRO( SR_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(SR_, __VA_ARGS__) )
