#pragma once

#ifdef _WIN64
#ifdef TEMPEST_EXPORT
#define TEMPEST_API __declspec(dllexport)
#else
#define TEMPEST_API __declspec(dllimport)
#endif
#endif