#pragma once
#include "AM_Entity.h"

class AM_EntityStorage
{
public:
	AM_Entity& Add();
	AM_Entity* GetIfExist(uint64_t anId);

private:
	std::unordered_map<uint64_t, AM_Entity> myEntities;
};