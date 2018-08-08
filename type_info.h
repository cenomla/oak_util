#pragma once

#include <cinttypes>
#include <cstddef>
#include <type_traits>

#include "string.h"
#include "ptr.h"
#include "osig_defs.h"

#define ssizeof(x) static_cast<int64_t>(sizeof(x))

namespace oak {

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
		size_t count = 0;
	};

	struct StructInfo : TypeInfo {
		void (*construct)(void *) = nullptr;
		const VarInfo *members = nullptr;
		size_t memberCount = 0;
		size_t tid = 0;
		size_t catagoryId = 0;

		inline const VarInfo* begin() const { return members; }
		inline const VarInfo* end() const { return members + memberCount; }
	};

	struct EnumInfo : TypeInfo {
		const TypeInfo *underlyingType = nullptr;
		const EnumConstant *members = nullptr;
		size_t memberCount = 0;

		inline const EnumConstant* begin() const {
			return members;
		}

		inline const EnumConstant* end() const {
			return members + memberCount;
		}
	};

	template<typename T>
	constexpr decltype(auto) enum_int(T val) {
		return static_cast<std::underlying_type_t<T>>(val);
	}

	template<typename T> std::enable_if_t<
		!(std::is_pointer_v<T> || std::is_array_v<T>),
		const TypeInfo*> type_info();

	template<typename T> std::enable_if_t<std::is_pointer_v<T>, const TypeInfo*> type_info() {
		static const PtrInfo info{
			{ TypeKind::PTR, "pointer", sizeof(T), alignof(T) },
			type_info<std::remove_pointer_t<T>>()
		};
		return &info;
	}

	template<typename T> std::enable_if_t<std::is_array_v<T>, const TypeInfo*> type_info() {
		static const ArrayInfo info{
			{ TypeKind::ARRAY, "array", sizeof(T), alignof(T) },
			type_info<std::remove_extent_t<T>>(), std::extent_v<T>
		};
		return &info;
	}

	template<typename T>
	const StructInfo* struct_info() {
		auto typeInfo = type_info<T>();
		assert(typeInfo->kind == TypeKind::STRUCT);
		return static_cast<const StructInfo*>(typeInfo);
	}

	template<typename T> size_t type_id() {
		return struct_info<T>()->tid;
	}

	template<typename T>
	Slice<const TypeInfo*> types_in_catagory();

	template<typename T>
	size_t catagory_id();

	template<typename C>
	bool is_type_in_catagory(const TypeInfo *typeInfo) {
		auto catagoryId = typeInfo->kind == TypeKind::STRUCT ? static_cast<const StructInfo*>(typeInfo)->catagoryId : 0;
		return catagoryId == catagory_id<C>();
	}

	struct Any {
		void *ptr = nullptr;
		const TypeInfo *type = nullptr;

		Any() = default;
		Any(void *_ptr, const TypeInfo *_type) : ptr{ _ptr }, type{ _type } {}
		Any(const Any& other) : ptr{ other.ptr }, type{ other.type } {}
		template<typename T, typename DT = std::decay_t<T>, std::enable_if_t<!std::is_same_v<DT, Any>, int> = 0>
		Any(T&& thing) : ptr{ &thing }, type{ type_info<DT>() } {}
	};

}

