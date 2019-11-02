#pragma once

#include <cassert>
#include <type_traits>

#include <osig_defs.h>

#include "types.h"
#include "ptr.h"

namespace oak {

	constexpr bool USING_TYPE_INFO = true;

	template<typename T>
	struct Slice;

	enum class TypeKind {
		NONE,
		VOID,
		BOOL,
		INT8,
		INT16,
		INT32,
		INT64,
		UINT8,
		UINT16,
		UINT32,
		UINT64,
		FLOAT32,
		FLOAT64,
		PTR,
		ARRAY,
		STRUCT,
		ENUM,
		OAK_SLICE,
	};

	struct TypeInfo {
		TypeKind kind;
		String name;
		u64 size = 0;
		u64 align = 0;
	};

	enum VarFlags : u32 {
		VAR_VOLATILE = 0x01,
	};

	struct MemberInfo {
		String name;
		TypeInfo const *type = nullptr;
		u64 offset = 0;
		u32 flags = 0;
	};

	struct EnumConstant {
		String name;
		i64 value = 0;
	};

	struct PtrInfo : TypeInfo {
		TypeInfo const *of = nullptr;
	};

	struct ArrayInfo : TypeInfo {
		TypeInfo const *of = nullptr;
		i64 count = 0;
	};

	struct StructInfo : TypeInfo {
		void (*construct)(void *) = nullptr;
		MemberInfo const *members = nullptr;
		i64 memberCount = 0;
		u64 tid = 0;
		u64 catagoryId = 0;

		constexpr MemberInfo const* begin() const noexcept {
			return members;
		}

		constexpr MemberInfo const* end() const noexcept {
			return members + memberCount;
		}

	};

	struct EnumInfo : TypeInfo {
		TypeInfo const *underlyingType = nullptr;
		EnumConstant const *members = nullptr;
		i64 memberCount = 0;

		constexpr EnumConstant const* begin() const noexcept {
			return members;
		}

		constexpr EnumConstant const* end() const noexcept {
			return members + memberCount;
		}
	};

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

	struct _reflect(oak::catagory::none) Any {
		void *ptr = nullptr;
		TypeInfo const *type _opaque = nullptr;

		Any() = default;
		Any(void *ptr_, TypeInfo const *type_) : ptr{ ptr_ }, type{ type_ } {}

		template<typename T, typename DT = std::decay_t<T>, typename = std::enable_if_t<!std::is_same_v<DT, Any>>>
		explicit Any(T&& thing) : ptr{ &thing }, type{ type_info<DT>() } {}

		Any get_member(String name) noexcept;
		Any get_member(String name) const noexcept;
		Any get_element(i64 index) noexcept;

		void construct() noexcept;

		template<typename T>
		T const& to_value() const noexcept {
			return *static_cast<T const*>(ptr);
		}

		template<typename T>
		T& to_value() noexcept {
			return *static_cast<T*>(ptr);
		}

		void set_enum_value(i64 ev) noexcept;
		i64 get_enum_value() const noexcept;

	};

	bool operator==(Any const& lhs, Any const& rhs) noexcept;

	inline bool operator!=(Any const& lhs, Any const& rhs) noexcept {
		return !(lhs == rhs);
	}

	void copy_fields(Any dst, Any src);
}

