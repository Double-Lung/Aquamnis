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

	AM_AllocationObject()
		: myOffset(0)
		, mySize(0)
		, myType(NOTSET)
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(const ObjectType aType)
		: myOffset(0)
		, mySize(0)
		, myType(aType)
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(const ObjectType aType, const uint64_t anOffset, const uint64_t aSize)
		: myOffset(anOffset)
		, mySize(aSize)
		, myType(aType)
		, myIsEmpty(true)
	{
	}

	AM_AllocationObject(AM_AllocationObject&& anObject) noexcept
		: myOffset(0)
		, mySize(0)
		, myType(NOTSET)
		, myIsEmpty(true)
	{
		*this = std::move(anObject);
	}

	virtual ~AM_AllocationObject() = default;

	const uint64_t GetOffset() const { return myOffset; }
	const uint64_t GetSize() const { return mySize; }
	bool IsEmpty() { return myIsEmpty; }
	void SetOffset(const uint64_t anOffset) { myOffset = anOffset; }
	void SetSize(const uint64_t aSize) { mySize = aSize; }
	void SetIsEmpty(bool aState) { myIsEmpty = aState; }

protected:
	void Reset()
	{
		myOffset = 0;
		mySize = 0;
		myIsEmpty = true;
	}

	uint64_t myOffset;
	uint64_t mySize;
	ObjectType myType;
	bool myIsEmpty;

private:
	AM_AllocationObject(const AM_AllocationObject& anObject) = delete;
	AM_AllocationObject& operator=(const AM_AllocationObject& anObject) = delete;
	AM_AllocationObject& operator=(AM_AllocationObject&& anObject) noexcept
	{
		if (this == &anObject)
			return *this;
		myOffset = std::exchange(anObject.myOffset, 0);
		mySize = std::exchange(anObject.mySize, 0);
		myType = std::exchange(anObject.myType, NOTSET);
		myIsEmpty = std::exchange(anObject.myIsEmpty, true);
		return *this;
	}
};
