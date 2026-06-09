#include <type_traits>
#include <memory>
#include <algorithm>

namespace PointerUtils
{
	bool compareArrays(const std::unique_ptr<std::uint8_t[]>& arr1, size_t size1, const std::unique_ptr<std::uint8_t[]>& arr2, size_t size2);
	bool compareRawArrays(const std::uint8_t* arr1, size_t size1, const std::uint8_t* arr2, size_t size2);

	template <typename T>
	struct is_smart_pointer : std::false_type {};

	template <typename T>
	struct is_smart_pointer<std::unique_ptr<T>> : std::true_type {};

	template <typename T>
	struct is_smart_pointer<std::shared_ptr<T>> : std::true_type {};

	template <typename T>
	struct is_smart_pointer<std::weak_ptr<T>> : std::true_type {};

	template <typename Type>
	struct value_nonptr_type_impl { using type = Type; };

	template <typename Type>
	struct value_nonptr_type_impl<Type*> { using type = Type; };

	template <typename Type>
	struct value_nonptr_type_impl<std::unique_ptr<Type>> { using type = Type; };

	template <typename Type>
	struct value_nonptr_type_impl<std::shared_ptr<Type>> { using type = Type; };

	template <typename Type>
	struct value_nonptr_type_impl<std::weak_ptr<Type>> { using type = Type; };

	template <typename Type>
	using value_nonptr_type = typename value_nonptr_type_impl<Type>::type;

	template <typename Type>
	struct to_raw_pointer { using type = Type; };

	template <typename Type>
	struct to_raw_pointer<std::unique_ptr<Type>> { using type = Type*; };

	template <typename Type>
	struct to_raw_pointer<std::shared_ptr<Type>> { using type = Type*; };

	template <typename Type>
	struct to_raw_pointer<std::weak_ptr<Type>> { using type = Type*; };

	template <typename Type>
	using to_raw_pointer_t = typename to_raw_pointer<Type>::type;
}