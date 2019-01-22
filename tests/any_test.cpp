#include <type_traits>
#include <new>
#include <oak_util/type_info.h>

struct Test {
	int64_t a;
	float b;
};

namespace oak {

	template<typename T>
	void generic_construct(void *obj) {
		if constexpr(std::is_default_constructible_v<T>) {
			new (obj) T{};
		}
	}

	template<> const TypeInfo* type_info<void>() {
		static const TypeInfo info{ TypeKind::VOID, "void", 0, 0 };
		return &info;
	}

	template<> const TypeInfo* type_info<bool>() {
		static const TypeInfo info{ TypeKind::BOOL, "bool", 1, 1 };
		return &info;
	}

	template<> const TypeInfo* type_info<uint8_t>() {
		static const TypeInfo info{ TypeKind::UINT8, "uint8", 1, 1 };
		return &info;
	}

	template<> const TypeInfo* type_info<uint16_t>() {
		static const TypeInfo info{ TypeKind::UINT16, "uint16", 2, 2 };
		return &info;
	}

	template<> const TypeInfo* type_info<uint32_t>() {
		static const TypeInfo info{ TypeKind::UINT32, "uint32", 4, 4 };
		return &info;
	}

	template<> const TypeInfo* type_info<uint64_t>() {
		static const TypeInfo info{ TypeKind::UINT64, "uint64", 8, 8 };
		return &info;
	}

	template<> const TypeInfo* type_info<int8_t>() {
		static const TypeInfo info{ TypeKind::INT8, "int8", 1, 1 };
		return &info;
	}

	template<> const TypeInfo* type_info<int16_t>() {
		static const TypeInfo info{ TypeKind::INT16, "int16", 2, 2 };
		return &info;
	}

	template<> const TypeInfo* type_info<int32_t>() {
		static const TypeInfo info{ TypeKind::INT32, "int32", 4, 4 };
		return &info;
	}

	template<> const TypeInfo* type_info<int64_t>() {
		static const TypeInfo info{ TypeKind::INT64, "int64", 8, 8 };
		return &info;
	}

	template<> const TypeInfo* type_info<float>() {
		static const TypeInfo info{ TypeKind::FLOAT32, "float32", 4, 4 };
		return &info;
	}

	template<> const TypeInfo* type_info<double>() {
		static const TypeInfo info{ TypeKind::FLOAT64, "float64", 8, 8 };
		return &info;
	}

	template<> const TypeInfo* type_info<char>() {
		return type_info<int8_t>();
	}

	template<> const TypeInfo* type_info<Test>() {
		using T = Test;
		static const MemberInfo members[] = {
			{ "a", type_info<decltype(T::a)>(), offsetof(T, a), 0 },
			{ "b", type_info<decltype(T::b)>(), offsetof(T, b), 0 },
		};
		static const StructInfo info{
			{ TypeKind::STRUCT, "Test", sizeof(T), alignof(T) },
			generic_construct<T>, members, 2, 1, 0 };
		return &info;
	}

	template<> const TypeInfo* type_info<Slice<Test>>() {
		using T = oak::Slice<Test>;
		static const MemberInfo members[] = {
			{ "data", type_info<decltype(T::data)>(), offsetof(T, data), 0 },
			{ "count", type_info<decltype(T::count)>(), offsetof(T, count), 0 },
		};
		static const StructInfo info{ { TypeKind::OAK_SLICE, "Slice", sizeof(T), alignof(T) },
			generic_construct<T>, members, 2, 0, 0 };
		return &info;
	}

}

int main(int argc, char **argv) {

	Test test;
	oak::Any thing{ test };
	auto thing_ = thing;

	auto m_a = thing.get_member("a");
	auto m_b = thing.get_member("b");

	m_a.to_value<int64_t>() = 64;
	m_b.to_value<float>() = 8.f;

	assert(m_a.to_value<int64_t>() == 64);
	assert(m_b.to_value<float>() == 8.f);

	thing_.get_member("a").to_value<int64_t>() = 32;

	assert(thing.get_member("a").to_value<int64_t>() == 32);

	++m_a.to_value<int64_t>();

	assert(test.a == 33 && test.b == 8.f);

	Test tests[] = {
		{ 49, 0.f },
		{ -21, 13.f },
		{ 0, 19.f },
	};

	oak::Slice testsSlice{ tests };

	oak::Any things{ testsSlice };
	assert(things.get_element(1).get_member("a").to_value<int64_t>() == -21);
	assert(things.get_element(2).get_member("b").to_value<float>() == 19.f);


	return 0;
}

