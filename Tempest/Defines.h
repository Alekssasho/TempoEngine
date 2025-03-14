#pragma once

#ifdef _WIN64
#define TEMPEST_PLATFORM_WIN

// For now we have only a static lib build
#define TEMPEST_API
//#ifdef TEMPEST_EXPORT
//#define TEMPEST_API __declspec(dllexport)
//#else
//#define TEMPEST_API __declspec(dllimport)
//#endif
#endif

#include <inttypes.h>
#include <assert.h>


// TODO: Move to some utilities header

namespace Tempest
{
namespace Utils
{
struct NonCopyable
{
	NonCopyable() = default;
	~NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};
}
}
