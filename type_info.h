#pragma once

#include <cinttypes>
#include <cstddef>

#include "string.h"
#include "ptr.h"

#ifdef __OSIG__
#define _reflect(x) __attribute__((annotate("reflect:"#x)))
#else
#define _reflect(x)
#endif

#define STRUCT_INFO(T) static_cast<const oak::StructInfo*>(oak::type_info<T>())

namespace oak {

	enum class TypeKind {
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

	struct PtrInfo : TypeInfo {
		const TypeInfo *of = nullptr;
	};

	struct ArrayInfo : TypeInfo {
		const TypeInfo *of = nullptr;
		size_t count = 0;
	};

	struct VarInfo {
		const TypeInfo *type = nullptr;
		String name;
		uint32_t flags = 0;
		uint64_t version = 0;
	};

	struct Any : VarInfo {
		void *ptr = nullptr;
	};

	struct StructInfo : TypeInfo {
		struct MemberList {
			struct Iterator {
				const VarInfo *var;
				const VarInfo *end;
				void *ptr;

				inline Any operator*() const {
					return { { *var }, ptr };
				}

				inline Iterator& operator++() {
					ptr = ptr::add(ptr, var->type->size);
					var++;
					if (var != end) {
						ptr = ptr::align_address(ptr, var->type->align);
					}
					return *this;
				}

				inline bool operator!=(const Iterator& other) {
					return var != other.var;
				}
			};

			const VarInfo *var;
			size_t count;
			void *ptr;

			inline Iterator begin() const { return { var, var + count, ptr }; }
			inline Iterator end() const { return { var + count, var + count, ptr }; }
		};

		const VarInfo *members = nullptr;
		size_t memberCount = 0;
		size_t tid = 0;

		MemberList operator()(void *ptr) const {
			return { members, memberCount, ptr };
		}

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

	template<typename T> size_t type_tid() {
		return STRUCT_INFO(T)->tid;
	}

}
