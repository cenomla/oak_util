#pragma once

#include <cassert>
#include <type_traits>

#include "types.h"
#include "ptr.h"

// TODO: Clean up old type info stuff

namespace oak {

	/*
	template<typename T>
	TypeInfo const* type_info_internal() noexcept;

	template<typename T>
	TypeInfo const* type_info() noexcept {
		if constexpr(std::is_pointer_v<T>) {
			static PtrInfo const info{
				{ TypeKind::PTR, "pointer", sizeof(T), alignof(T) },
				type_info<std::remove_pointer_t<T>>()
			};
			return &info;
		} else if constexpr(std::is_array_v<T>) {
			static ArrayInfo const info{
				{ TypeKind::ARRAY, "array", sizeof(T), alignof(T) },
				type_info<std::remove_extent_t<T>>(), std::extent_v<T>
			};
			return &info;
		} else {
			return type_info_internal<T>();
		}
	}

	template<typename T>
	StructInfo const* struct_info() noexcept {
		auto typeInfo = type_info<T>();
		assert(typeInfo->kind == TypeKind::STRUCT);
		return static_cast<StructInfo const*>(typeInfo);
	}

	template<typename T> u64 type_id() noexcept {
		return struct_info<T>()->tid;
	}

	template<typename T>
	Slice<TypeInfo const*> types_in_catagory() noexcept;

	template<typename T>
	u64 catagory_id() noexcept;

	template<typename C>
	bool is_type_in_catagory(TypeInfo const *typeInfo) noexcept {
		auto catagoryId = typeInfo->kind == TypeKind::STRUCT ? static_cast<StructInfo const*>(typeInfo)->catagoryId : 0;
		return catagoryId == catagory_id<C>();
	}

	TypeInfo const* get_type_info_by_name(String name) noexcept;

	template<typename T>
	String enum_name(T val) noexcept {
		if constexpr (std::is_enum_v<T>) {
			String result;
			auto ei = static_cast<EnumInfo const*>(type_info<T>());
			for (auto ev : Slice{ ei->members, ei->memberCount }) {
				if (ev.value == static_cast<i64>(val)) {
					result = ev.name;
					break;
				}
			}
			return result;
		} else {
			return "";
		}
	}

	inline const TypeInfo noTypeInfo{ oak::TypeKind::NONE, "none", 0, 0 };

	*/
}

