#pragma once

namespace Tempest
{

class GameplayFeature
{
public:
	virtual ~GameplayFeature() {}
	virtual void PrepareSystems(class World& world) = 0;
};
}