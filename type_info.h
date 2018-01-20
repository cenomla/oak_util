#pragma once

#include <cinttypes>
#include <cstddef>

#include "string.h"
#include "ptr.h"
#include "osig_defs.h"

#define STRUCT_INFO(T) static_cast<const oak::StructInfo*>(oak::type_info<T>())

namespace oak {

	enum class TypeKind {
		NONE,
		VOID,
		BOOL,
		UINT8,
		UINT16,
		UINT32,
		UINT64,
		INT8,
		INT16,
		INT32,
		INT64,
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

	struct Any : VarInfo {
		void *ptr = nullptr;
	};

	struct PtrInfo : TypeInfo {
		const TypeInfo *of = nullptr;
	};

	struct ArrayInfo : TypeInfo {
		const TypeInfo *of = nullptr;
		size_t count = 0;
	};

	struct StructInfo : TypeInfo {
		const VarInfo *members = nullptr;
		size_t memberCount = 0;
		size_t tid = 0;

		inline const VarInfo* begin() const { return members; }
		inline const VarInfo* end() const { return members + memberCount; }
	};

	struct EnumInfo : TypeInfo {
		const String *members = nullptr;
		size_t memberCount = 0;

		inline const String* begin() const {
			return members;
		}

		inline const String* end() const {
			return members + memberCount; 
		}
	};

	template<typename T> const TypeInfo* type_info();

	template<typename T> size_t type_id() {
		return STRUCT_INFO(T)->tid;
	}

}
