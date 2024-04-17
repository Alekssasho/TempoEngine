#pragma once

#include <Defines.h>

//#define STOMP_ALLOCATOR
#ifdef STOMP_ALLOCATOR
#define PZ_PLATFORM_64BITS
#define PZ_PLATFORM_WINDOWS
#include <pz_stomp_allocator.h>

void* operator new(std::size_t size)
{
	return pz_stomp_allocator_alloc(size, 0);
}
void operator delete(void* ptr)
{
	pz_stomp_allocator_free(ptr);
}
void* operator new[](std::size_t size)
{
	return pz_stomp_allocator_alloc(size, 0);
}
void operator delete[](void* ptr)
{
	pz_stomp_allocator_free(ptr);
}
#endif

// TODO: This cannot be included in two different cpps as there will be multiple definition of this symbols.
// Maybe we need to hide them inside some macro

void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}


void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}