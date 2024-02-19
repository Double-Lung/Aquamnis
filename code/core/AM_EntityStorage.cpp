#include "AM_EntityStorage.h"

AM_EntityStorage::AM_EntityStorage()
{
	myEntities.reserve(10);
}

AM_EntityStorage::~AM_EntityStorage()
{
	for (auto& kv : myEntities)
	{
		if (kv.second)
			delete kv.second;
	}
}

AM_Entity* AM_EntityStorage::Add()
{
	AM_Entity* newEntity = AM_Entity::CreateEntity();
	myEntities.emplace(newEntity->myId, newEntity);
	return newEntity;
}

// not good for concurrency
AM_Entity* AM_EntityStorage::GetIfExist(uint64_t anId)
{
	auto ite = myEntities.find(anId);
	if (ite != myEntities.end())
		return ite->second;
	return nullptr;
}
