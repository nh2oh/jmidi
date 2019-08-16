#include "midi_vlq.h"
#include "midi_vlq_deprecated.h"
#include <limits>

template<typename T1, typename T2>
struct has_different_signedness {
	static constexpr bool value = ((std::is_signed<T1>::value && std::is_unsigned<T2>::value) 
		|| (std::is_signed<T2>::value && std::is_unsigned<T1>::value));
};

template<typename TIn, typename TOut>
constexpr TOut clamp_cast(TIn in, TOut min, TOut max) {
	TOut result = min;
	if (in <= min) {
		return min;
	} else if (in >= max) {
		return max;
	}
	return static_cast<TOut>(result);
};

constexpr int clamp_cast_tests();
int my_function_yay();

int my_function_yay() {
	clamp_cast_tests();
	return 52;
}

constexpr int clamp_cast_tests() {
	{
		uint32_t val = 5;
		int32_t min = -17;
		int32_t max = -2;
		int32_t result = clamp_cast(val,min,max);
		bool b = (result==7);
	}

	{
		constexpr bool b = has_different_signedness<int32_t,uint32_t>::value;
		uint32_t val = 25;
		int32_t min = -17;
		int32_t max = -2;
		int32_t result = clamp_cast(val,min,max);
		bool bb = (result==7);
	}

	typename std::common_type<int16_t,int16_t>::type i16i16 = 0;  // int16_t
	typename std::common_type<int16_t,uint16_t>::type i16ui16 = 0;  // int32_t
	typename std::common_type<uint16_t,int32_t>::type ui16i32 = 0;  // int32_t*
	typename std::common_type<uint16_t,uint32_t>::type ui16ui32 = 0;  // uint32_t
	typename std::common_type<int32_t,uint32_t>::type i32ui32 = 0;  // uint32_t
	typename std::common_type<uint32_t,uint32_t>::type ui32ui32 = 0;  // uint32_t
	typename std::common_type<int64_t,uint32_t>::type i64ui32 = 0;  // int64_t
	typename std::common_type<int64_t,uint64_t>::type i64ui64 = 0;  // uint64_t

	bool b = (i16ui16==7);

	return i16ui16+b;
}





