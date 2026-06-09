#include "U_Pointers.h"

bool PointerUtils::compareArrays(const std::unique_ptr<std::uint8_t[]>& arr1, size_t size1, const std::unique_ptr<std::uint8_t[]>& arr2, size_t size2)
{
	if (size1 != size2)
	{
		return false;
	}
	return std::equal(arr1.get(), arr1.get() + size1, arr2.get());
}

bool PointerUtils::compareRawArrays(const std::uint8_t* arr1, size_t size1, const std::uint8_t* arr2, size_t size2)
{
	if (size1 != size2)
	{
		return false;
	}
	return std::equal(arr1, arr1 + size1, arr2);
	//or return std::memcmp(arr1, arr2, size1) == 0;
}
