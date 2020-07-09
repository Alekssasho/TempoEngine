#pragma once
#include <Defines.h>

namespace Tempest
{
struct EngineCoreOptions
{

};

class TEMPEST_API EngineCore
{
public:
	EngineCore(const EngineCoreOptions& options);
	~EngineCore();

	void StartEngineLoop();
private:
	EngineCoreOptions m_Options;
};

static EngineCore* gEngine = nullptr;
}