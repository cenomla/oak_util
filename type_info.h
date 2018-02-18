#pragma once

#include <cinttypes>
#include <cstddef>
#include <type_traits>

#include "string.h"
#include "ptr.h"
#include "osig_defs.h"

#define STRUCT_INFO(T) static_cast<const oak::StructInfo*>(oak::type_info<T>())

namespace oak {

	template<typename T>
	struct ArrayView;

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
	};

	struct TypeInfo {
		TypeKind kind;
		String name;
		size_t size = 0;
		size_t align = 0;
	};

	struct VarInfo {
		String name;
		const TypeInfo *type = nullptr;
		size_t offset = 0;
		uint32_t flags = 0;
		uint64_t version = 0;
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

		inline const VarInfo* begin() const { return members; }
		inline const VarInfo* end() const { return members + memberCount; }
	};

	struct EnumInfo : TypeInfo {
		const EnumConstant *members = nullptr;
		size_t memberCount = 0;

		inline const EnumConstant* begin() const {
			return members;
		}

		inline const EnumConstant* end() const {
			return members + memberCount; 
		}
	};

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

	template<typename T> size_t type_id() {
		return STRUCT_INFO(T)->tid;
	}

	template<typename T>
	ArrayView<const TypeInfo*> types_in_catagory();

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
