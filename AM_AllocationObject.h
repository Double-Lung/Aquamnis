#pragma once
#include <vulkan/vulkan.h>

class AM_AllocationObject
{
public:
	enum ObjectType
	{
		NOTSET,
		BUFFER,
		IMAGE,
	};

	AM_AllocationObject(const ObjectType aType, const uint64_t anOffset, const uint64_t aSize)
		: myOffset(anOffset)
		, mySize(aSize)
		, myMappedMemory(nullptr)
		, myType(aType)
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(AM_AllocationObject&& anObject) noexcept
		: myOffset(0)
		, mySize(0)
		, myMappedMemory(nullptr)
		, myType(NOTSET)
		, myIsEmpty(true)
	{
		*this = std::move(anObject);
	}

	virtual ~AM_AllocationObject() = default;

	void SetOffset(const uint64_t anOffset) { myOffset = anOffset; }
	const uint64_t GetOffset() const { return myOffset; }

	void SetSize(const uint64_t aSize) { mySize = aSize; }
	const uint64_t GetSize() const { return mySize; }

	void SetIsEmpty(bool aState) { myIsEmpty = aState; }
	bool IsEmpty() const { return myIsEmpty; }

	void SetMappedMemory(void* aMemoryPointer) { myMappedMemory = aMemoryPointer; }
	void* GetMappedMemory() const {  return myMappedMemory; }

protected:
	void Reset()
	{
		myOffset = 0;
		mySize = 0;
		myIsEmpty = true;
	}

	uint64_t myOffset;
	uint64_t mySize;
	void* myMappedMemory;
	ObjectType myType;
	bool myIsEmpty;

private:
	AM_AllocationObject() = delete;
	AM_AllocationObject(const AM_AllocationObject& anObject) = delete;
	AM_AllocationObject& operator=(const AM_AllocationObject& anObject) = delete;
	AM_AllocationObject& operator=(AM_AllocationObject&& anObject) noexcept
	{
		if (this == &anObject)
			return *this;
		myOffset = std::exchange(anObject.myOffset, 0);
		mySize = std::exchange(anObject.mySize, 0);
		myMappedMemory = std::exchange(anObject.myMappedMemory, nullptr);
		myType = std::exchange(anObject.myType, NOTSET);
		myIsEmpty = std::exchange(anObject.myIsEmpty, true);
		return *this;
	}
};
