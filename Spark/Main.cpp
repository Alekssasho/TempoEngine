#include <EngineCore.h>

int main()
{
	Tempest::EngineCoreOptions options;

	Tempest::EngineCore engine(options);
	engine.StartEngineLoop();
}