#pragma once

#include <cstring>

#include "string.h"
#include "ptr.h"

namespace oak {

	struct Type {
		enum Enum {
			VOID,
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
			STRUCT,
		};
	};

	struct TypeInfo {
		Type::Enum type;
		String name;
		size_t size;
	};

	struct VarFlag { 
		enum Enum {
			POINTER = 0x01,
			CONST = 0x02,
			VOLATILE = 0x04,
		};
	};

	struct Var {
		const TypeInfo *info;
		String name;
		uint32_t flags = 0;
		uint32_t count = 0; //greater than zero if var is an array
	};

	struct Any : Var {
		void *ptr;
	};

	struct StructInfo : TypeInfo {
		size_t tid = 0;
		struct {
			struct VarMemberList {
				struct Iterator {
					const Var *var; //the current variable
					void *ptr;

					inline Any operator*() const {
						return { { *var }, ptr };
					}
					inline Iterator& operator++() { 
						if (var->flags & VarFlag::POINTER) {
							ptr = ptrutil::add(ptr, sizeof(void*));
						} else {
							ptr = ptrutil::add(ptr, var->info->size);
						}
						var++;
						return *this;
					}

					inline bool operator!=(const Iterator& other) const {
						return var != other.var;
					}

				};
				const Var *var;
				size_t count; //the number of vars in the list
				void *ptr; //the pointer to the struct

				inline const Iterator begin() const {
					return Iterator{ var, ptr };
				}
				inline const Iterator end() const {
					return Iterator{ var + count, ptr };
				}
			};
			const Var *data = nullptr;
			size_t count = 0;

			inline VarMemberList operator()(void *pm) const {
				return VarMemberList{ data, count, pm };
			}

			inline const Var* begin() const { return data; }
			inline const Var* end() const { return data + count; }
		} members;
	};


	template<typename T>
	constexpr StructInfo make_struct_info(size_t id, const String& name) {
		return StructInfo { 
			{ Type::STRUCT, name, sizeof(T) }, id
		};
	}

	template<typename T, size_t N>
	constexpr StructInfo make_struct_info(size_t id, const String& name, const Var (&array)[N]) {
		return StructInfo { 
			{ Type::STRUCT, name, sizeof(T) }, id,
			{ &array[0], N }
		};
	}

	inline TypeInfo types[] = {
		{ Type::VOID, "void", 0 },
		{ Type::INT8, "int8", 1 },
		{ Type::INT16, "int16", 2 },
		{ Type::INT32, "int32", 4 },
		{ Type::INT64, "int64", 8 },
		{ Type::UINT8, "uint8", 1 },
		{ Type::UINT16, "uint16", 2 },
		{ Type::UINT32, "uint32", 4 },
		{ Type::UINT64, "uint64", 8 },
		{ Type::FLOAT32, "float32", 4 },
		{ Type::FLOAT64, "float64", 8 },
	};
} 
