#pragma once

#include <cinttypes>
#include <cstddef>
#include <cassert>
#include <type_traits>

#include "string.h"
#include "ptr.h"
#include "osig_defs.h"

#define ssizeof(x) static_cast<int64_t>(sizeof(x))
#define array_count(x) (sizeof(x)/sizeof(*x))
#define sarray_count(x) static_cast<int64_t>(array_count(x))

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
		OAK_ARRAY,
		OAK_HASH_MAP,
	};

	struct TypeInfo {
		TypeKind kind;
		String name;
		size_t size = 0;
		size_t align = 0;
	};

	enum VarFlags : uint32_t {
		VAR_VOLATILE = 0x01,
	};

	struct VarInfo {
		String name;
		const TypeInfo *type = nullptr;
		size_t offset = 0;
		uint32_t flags = 0;
	};

	struct EnumConstant {
		String name;
		int64_t value = 0;
	};

	struct PtrInfo : TypeInfo {
		const TypeInfo *of = nullptr;
	};

	struct ArrayInfo : TypeInfo {
		const TypeInfo *of = nullptr;
		int64_t count = 0;
	};

	struct StructInfo : TypeInfo {
		void (*construct)(void *) = nullptr;
		VarInfo const *members = nullptr;
		int64_t memberCount = 0;
		size_t tid = 0;
		size_t catagoryId = 0;

		inline VarInfo const* begin() const { return members; }
		inline VarInfo const* end() const { return members + memberCount; }
	};

	struct EnumInfo : TypeInfo {
		TypeInfo const *underlyingType = nullptr;
		EnumConstant const *members = nullptr;
		int64_t memberCount = 0;

		inline EnumConstant const* begin() const {
			return members;
		}

		inline EnumConstant const* end() const {
			return members + memberCount;
		}
	};

	template<typename T>
	constexpr decltype(auto) enum_int(T val) {
		if constexpr (std::is_enum_v<T>) {
			return static_cast<std::underlying_type_t<T>>(val);
		} else {
			static_assert("\"enum_int\" must be used with an enum type");
		}
	}

	template<typename T>
	TypeInfo const* type_info_internal();

	template<typename T>
	TypeInfo const* type_info() {
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
	StructInfo const* struct_info() {
		auto typeInfo = type_info<T>();
		assert(typeInfo->kind == TypeKind::STRUCT);
		return static_cast<StructInfo const*>(typeInfo);
	}

	template<typename T> size_t type_id() {
		return struct_info<T>()->tid;
	}

	template<typename T>
	Slice<TypeInfo const*> types_in_catagory();

	template<typename T>
	size_t catagory_id();

	template<typename C>
	bool is_type_in_catagory(TypeInfo const *typeInfo) {
		auto catagoryId = typeInfo->kind == TypeKind::STRUCT ? static_cast<StructInfo const*>(typeInfo)->catagoryId : 0;
		return catagoryId == catagory_id<C>();
	}

	constexpr TypeInfo noTypeInfo{ oak::TypeKind::NONE, "none", 0, 0 };

	struct Any {
		void *ptr = nullptr;
		const TypeInfo *type = nullptr;

		Any() = default;
		Any(void *ptr_, TypeInfo const *type_) : ptr{ ptr_ }, type{ type_ } {}

		Any(Any const& other) = default;
		template<typename T, typename DT = std::decay_t<T>, typename = std::enable_if_t<!std::is_same_v<DT, Any>>>
		Any(T&& thing) : ptr{ &thing }, type{ type_info<DT>() } {}

		inline Any get_member(String name) {
			if (type->kind == TypeKind::STRUCT ||
					type->kind == TypeKind::OAK_SLICE) {
				auto si = static_cast<const StructInfo*>(type);
				for (auto member : Slice{ si->members, si->memberCount }) {
					if (member.name == name) {
						return { add_ptr(ptr, member.offset), member.type };
					}
				}
			}
			return { nullptr, &noTypeInfo };
		}

		inline Any get_element(int64_t index) {
			if (type->kind == TypeKind::ARRAY) {
				auto ai = static_cast<const ArrayInfo*>(type);
				if (index < ai->count) {
					return { add_ptr(ptr, index * ai->of->size), ai->of };
				}
			} else if (type->kind == TypeKind::OAK_SLICE) {
				auto data = get_member("data");
				auto count = get_member("count");
				if (index < count.to_value<int64_t>()) {
					auto pi = static_cast<const PtrInfo*>(data.type);
					return { add_ptr(data.to_value<void*>(), index * pi->of->size), pi->of };
				}
			}
			return { nullptr, &noTypeInfo };
		}

		template<typename T>
		inline T& to_value() {
			TypeInfo const *typeInfo = type_info<T>();
			assert(typeInfo == type ||
					(type->kind == TypeKind::ENUM && static_cast<EnumInfo const*>(type)->underlyingType == typeInfo) ||
					(type->kind == TypeKind::PTR && typeInfo->kind == TypeKind::PTR &&
						 static_cast<PtrInfo const*>(typeInfo)->of == type_info<void>()));
			assert(ptr);
			return *static_cast<T*>(ptr);
		}
	};

}

