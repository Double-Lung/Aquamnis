#include "AM_EntityStorage.h"

AM_Entity& AM_EntityStorage::Add()
{
	static uint64_t id = 1; // reserve 0
	AM_Entity& newEntity = myEntities[id];
	newEntity.SetId(id);
	++id;
	return newEntity;
}

// not good for concurrency
AM_Entity* AM_EntityStorage::GetIfExist(uint64_t anId)
{
	auto ite = myEntities.find(anId);
	if (ite != myEntities.end())
		return &ite->second;
	return nullptr;
}
