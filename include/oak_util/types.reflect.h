#pragma once

#include <oak_reflect/type_info.h>
#include <../subprojects/oak_util/include/oak_util/types.h>

namespace oak {
template<> struct Reflect<::oak::Result> {
	using T = ::oak::Result;
	static constexpr EnumConstantInfo enumConstants[] = {
		{ "SUCCESS", 0 },
		{ "INVALID_ARGS", 1 },
		{ "OUT_OF_MEMORY", 2 },
		{ "FAILED_IO", 3 },
		{ "FILE_NOT_FOUND", 4 },
	};
	static constexpr EnumTypeInfo typeInfo{ { 1833584812239198302ul, TypeInfoKind::ENUM }, "Result", "reflect;oak::catagory::none", &Reflect<int>::typeInfo, enumConstants };
};
template<> struct Reflect<::oak::String> {
	using T = ::oak::String;
	static constexpr StructTypeInfo typeInfo{ { 14081299499232919634ul, TypeInfoKind::STRUCT }, "String", "reflect;oak::catagory::primitive", sizeof(T), alignof(T), {} };
};
}
