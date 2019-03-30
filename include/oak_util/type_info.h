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
	constexpr decltype(auto) enum_int(T val) noexcept {
		if constexpr (std::is_enum_v<T>) {
			return static_cast<std::underlying_type_t<T>>(val);
		} else {
			static_assert("\"enum_int\" must be used with an enum type");
		}
	}

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

	TypeInfo const* get_type_info_by_name(String name);

	inline const TypeInfo noTypeInfo{ oak::TypeKind::NONE, "none", 0, 0 };

	struct _reflect(oak::catagory::none) Any {
		void *ptr = nullptr;
		TypeInfo const *type _opaque = nullptr;

		Any() = default;
		Any(void *ptr_, TypeInfo const *type_) : ptr{ ptr_ }, type{ type_ } {}

		template<typename T, typename DT = std::decay_t<T>, typename = std::enable_if_t<!std::is_same_v<DT, Any>>>
		Any(T&& thing) : ptr{ &thing }, type{ type_info<DT>() } {}

		Any get_member(String name) noexcept {
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

		Any get_element(i64 index) noexcept {
			if (type->kind == TypeKind::ARRAY) {
				auto ai = static_cast<ArrayInfo const*>(type);
				if (index < ai->count) {
					return { add_ptr(ptr, index * ai->of->size), ai->of };
				}
			} else if (type->kind == TypeKind::OAK_SLICE) {
				auto data = get_member("data");
				auto count = get_member("count");
				if (index < count.to_value<i64>()) {
					auto pi = static_cast<PtrInfo const*>(data.type);
					return { add_ptr(data.to_value<void*>(), index * pi->of->size), pi->of };
				}
			}
			return { nullptr, &noTypeInfo };
		}

		void construct() noexcept {
			if (type->kind == TypeKind::STRUCT) {
				auto si = static_cast<StructInfo const*>(type);
				if (si->construct) {
					si->construct(ptr);
				}
			}
		}

		template<typename T>
		T& to_value() noexcept {
			TypeInfo const *typeInfo = type_info<T>();
			assert(typeInfo == type ||
					(type->kind == TypeKind::ENUM && static_cast<EnumInfo const*>(type)->underlyingType == typeInfo) ||
					(type->kind == TypeKind::PTR && typeInfo->kind == TypeKind::PTR &&
						 static_cast<PtrInfo const*>(typeInfo)->of == type_info<void>()));
			assert(ptr);
			return *static_cast<T*>(ptr);
		}

		inline void set_enum_value(i64 ev) noexcept {
			assert(type->kind == TypeKind::ENUM);
			auto ei = static_cast<EnumInfo const*>(type);
			switch (ei->underlyingType->kind) {
				case TypeKind::INT8: to_value<i8>() = static_cast<i8>(ev); break;
				case TypeKind::INT16: to_value<i16>() = static_cast<i16>(ev); break;
				case TypeKind::INT32: to_value<i32>() = static_cast<i32>(ev); break;
				case TypeKind::INT64: to_value<i64>() = static_cast<i64>(ev); break;
				case TypeKind::UINT8: to_value<u8>() = static_cast<u8>(ev); break;
				case TypeKind::UINT16: to_value<u16>() = static_cast<u16>(ev); break;
				case TypeKind::UINT32: to_value<u32>() = static_cast<u32>(ev); break;
				case TypeKind::UINT64: to_value<u64>() = static_cast<u64>(ev); break;
				default: break;
			}
		}

		inline i64 get_enum_value() noexcept {
			assert(type->kind == TypeKind::ENUM);
			auto ei = static_cast<EnumInfo const*>(type);
			i64 ev = 0;
			switch (ei->underlyingType->kind) {
				case oak::TypeKind::INT8: ev = to_value<i8>(); break;
				case oak::TypeKind::INT16: ev = to_value<i16>(); break;
				case oak::TypeKind::INT32: ev = to_value<i32>(); break;
				case oak::TypeKind::INT64: ev = to_value<i64>(); break;
				case oak::TypeKind::UINT8: ev = static_cast<i64>(to_value<u8>()); break;
				case oak::TypeKind::UINT16: ev = static_cast<i64>(to_value<u16>()); break;
				case oak::TypeKind::UINT32: ev = static_cast<i64>(to_value<u32>()); break;
				case oak::TypeKind::UINT64: ev = static_cast<i64>(to_value<u64>()); break;
				default: break;
			}
			return ev;
		}
	};

}

