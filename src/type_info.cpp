#include <oak_util/type_info.h>

#include <cstring>

namespace oak {

	/*
	Any Any::get_member(String name) noexcept {
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

	Any Any::get_member(String name) const noexcept {
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

	Any Any::get_element(i64 index) noexcept {
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

	void Any::construct() noexcept {
		if (type->kind == TypeKind::STRUCT) {
			auto si = static_cast<StructInfo const*>(type);
			if (si->construct) {
				si->construct(ptr);
			}
		}
	}

	void Any::set_enum_value(i64 ev) noexcept {
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

	i64 Any::get_enum_value() const noexcept {
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

	bool operator==(Any const& lhs, Any const& rhs) noexcept {
		if (lhs.type != rhs.type) {
			return false;
		}

		switch (lhs.type->kind) {
			case TypeKind::NONE: return true;
			case TypeKind::VOID: return true;
			case TypeKind::BOOL: return lhs.to_value<bool>() == rhs.to_value<bool>();
			case TypeKind::INT8: return lhs.to_value<i8>() == rhs.to_value<i8>();
			case TypeKind::INT16: return lhs.to_value<i16>() == rhs.to_value<i16>();
			case TypeKind::INT32: return lhs.to_value<i32>() == rhs.to_value<i32>();
			case TypeKind::INT64: return lhs.to_value<i64>() == rhs.to_value<i64>();
			case TypeKind::UINT8: return lhs.to_value<u8>() == rhs.to_value<u8>();
			case TypeKind::UINT16: return lhs.to_value<u16>() == rhs.to_value<u16>();
			case TypeKind::UINT32: return lhs.to_value<u32>() == rhs.to_value<u32>();
			case TypeKind::UINT64: return lhs.to_value<u64>() == rhs.to_value<u64>();
			case TypeKind::FLOAT32: return lhs.to_value<f32>() == rhs.to_value<f32>();
			case TypeKind::FLOAT64: return lhs.to_value<f64>() == rhs.to_value<f64>();
			case TypeKind::PTR: return lhs.to_value<void*>() == rhs.to_value<void*>();
			case TypeKind::ARRAY:
			{
				bool ret = true;
				auto ai = static_cast<ArrayInfo const*>(lhs.type);
				for (i32 i = 0; i < ai->count; ++i) {
					if (Any{ add_ptr(lhs.ptr, i * ai->of->size), ai->of }
							!= Any{ add_ptr(rhs.ptr, i * ai->of->size), ai->of }) {
						ret = false;
						break;
					}
				}
				return ret;
			}
			case TypeKind::STRUCT:
			{
				bool ret = true;
				auto si = static_cast<StructInfo const*>(lhs.type);
				for (auto& member : Slice{ si->members, si->memberCount }) {
					if (Any{ add_ptr(lhs.ptr, member.offset), member.type }
							!= Any{ add_ptr(rhs.ptr, member.offset), member.type }) {
						ret = false;
						break;
					}
				}
				return ret;
			}
			case TypeKind::ENUM: return lhs.get_enum_value() == rhs.get_enum_value();
			case TypeKind::OAK_SLICE:
			{
				bool ret = true;
				auto pi = static_cast<PtrInfo const*>(lhs.get_member("data").type);
				auto& data0 = lhs.get_member("data").to_value<void*>();
				auto& count0 = lhs.get_member("count").to_value<i64>();
				auto& data1 = rhs.get_member("data").to_value<void*>();
				auto& count1 = rhs.get_member("count").to_value<i64>();
				if (count0 == count1) {
					for (i32 i = 0; i < count0; ++i) {
						if (Any{ add_ptr(data0, i * pi->of->size), pi->of }
								!= Any{ add_ptr(data1, i * pi->of->size), pi->of }) {
							ret = false;
							break;
						}
					}
				} else {
					ret = false;
				}
				return ret;
			}
			default:
				return false;
		}
	}

	void copy_fields(Any dst, Any src) {
		assert(dst.type && src.type);

		if (dst.type->kind != TypeKind::STRUCT
				|| src.type->kind != TypeKind::STRUCT) {
			return;
		}

		auto si0 = static_cast<StructInfo const*>(dst.type);
		auto si1 = static_cast<StructInfo const*>(src.type);
		for (auto mem0 : Slice{ si0->members, si0->memberCount }) {
			for (auto mem1 : Slice{ si1->members, si1->memberCount }) {
				if (mem0.name == mem1.name && !(mem0.flags & VAR_VOLATILE)) {
					if (mem0.type->kind == TypeKind::STRUCT && mem1.type->kind == TypeKind::STRUCT) {
						copy_fields(
								{ add_ptr(dst.ptr, mem0.offset), mem0.type },
								{ add_ptr(src.ptr, mem1.offset), mem1.type });
						break;
					} else if (mem0.type == mem1.type) {
						std::memcpy(add_ptr(dst.ptr, mem0.offset), add_ptr(src.ptr, mem1.offset), mem0.type->size);
						break;
					}
				}
			}
		}
	}

	*/
}

