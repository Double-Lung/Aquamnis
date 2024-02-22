#pragma once
#include <unordered_map>

class AM_Entity;
struct AM_VkContext;
struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;
class AM_EntityStorage
{
public:
	AM_EntityStorage();
	~AM_EntityStorage();
	AM_Entity* Add();
	AM_Entity* GetIfExist(uint64_t anId);
	void GetEntitiesOfType(std::vector<AM_Entity*>& outEntities, uint8_t aType);
	void DestroyEntities(AM_VkContext& aVkContext, VmaAllocator anAllocator);

	const std::unordered_map<uint64_t, AM_Entity*>& GetStorage() const { return myEntities; }

private:
	std::unordered_map<uint64_t, AM_Entity*> myEntities;
};