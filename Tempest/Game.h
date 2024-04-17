#pragma once
#include <CommonIncludes.h>

#include <Engine.h>

namespace Tempest
{
struct GameOptions
{
	const char* LevelToLoad;
};

class TEMPEST_API Game
{
public:
	Game(const GameOptions& options, const EngineOptions& engineOptions);
	~Game();

	void Start();

private:
	// Data members
	GameOptions m_GameOptions;

	Engine m_Engine;

	static EngineOptions PrepareEngineOptions(const GameOptions& gameOptions, const EngineOptions& inOptions);
	static void LoadLevel(uint32_t, void* levelToLoad);
};

extern Game* gGame;
}
