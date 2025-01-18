//
// Created by FCWY on 25-1-8.
//
module pnl.ll.comtime;
import pnl.ll.meta_prog;
import pnl.ll.string;

using namespace pnl::ll;


ClassRepr::operator ClassRepr::Delegated() noexcept {
    return std::tie(
        member_list,
        method_list,
        static_member_list,
        static_method_list
    );
}

ObjectRepr::operator ObjectRepr::Delegated () noexcept {
    return std::tie(
        type,
        constructor_override_id,
        constructor_params
    );
}

Package::operator Package::Delegated () noexcept {
    return std::tie(
        data,
        exports
    );
}

Package Package::anonymous_builtin(MManager* const mem) noexcept {
    auto data = List<Content>{mem};
    auto exports = Map<USize, ObjRefRepr>{mem};

    // rtt object class
    {
        exports.emplace(data.size(), Str{L"RTUObject", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"RTUObject", mem},
                false,
                {},
                {{
                    {L"::RTUObject::()", mem},
                    {L"::RTUObject::~()", mem}
                }, {mem}}, {}, {}});
    }

    // rtt object class
    {
        exports.emplace(data.size(), Str{L"RTTObject", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"RTTObject", mem},
                true,
                List<Pair<Str, ObjRefRepr>>{{
                    Pair{L"type", L"::address"}
                }, {mem}},
                {{
                    {L"::RTTObject::()", mem},
                    {L"::RTTObject::~()", mem}
                }, {mem}}, {}, {}});
    }

    // class of all types: type class
    {
        exports.emplace(data.size(), Str{L"Type", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"Type", mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"__super", L"::RTTObject"},
                Pair{L"rtt", L"::bool"},
                Pair{L"", L"___"},
                Pair{L"size", L"::int"},
                Pair{L"maker", L"::address"},
                Pair{L"collector", L"::address"}
            }, {mem}},
            {{
                {L"::Type::()", mem},
                {L"::Type::~()", mem}
            }, {mem}}, {}, {}});
    }

    // class of all named types: named type class
    {
        exports.emplace(data.size(), Str{L"NamedType", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"NamedType", mem},
                true,
                List<Pair<Str, ObjRefRepr>>{{
                    Pair{L"__super", L"::Type"},
                    Pair{L"size", L"::long"},
                    Pair{L"maker", L"::address"},
                    Pair{L"collector", L"::address"}
                }, {mem}},
                {{
                    {L"::NamedType::()", mem},
                    {L"::NamedType::~()", mem}
                }, {mem}}, {}, {}});
    }

    // class of all classes: class class
    {
        exports.emplace(data.size(), Str{L"Class", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"Class", mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"__super", L"::NamedType"},
                Pair{L"members", L"::address"},
                Pair{L"methods", L"::address"},
                Pair{L"static_members", L"::address"},
                Pair{L"static_methods", L"::address"}
            }, {mem}},
            List<ObjRefRepr>{{
                {L"::Class::()", mem},
                {L"::Class::~()", mem}
            }, {mem}}, {}, {}});
    }

    // named types - primitives

    TypePack<Bool, Byte, Char, Int, Long, Float, Double>::foreach([&]<typename T, size_t I>() {
        const Char* name;
        if constexpr (std::same_as<T, Bool>)
            name = L"bool";
        else if constexpr (std::same_as<T, Byte>)
            name = L"byte";
        else if constexpr (std::same_as<T, Char>)
            name = L"char";
        else if constexpr (std::same_as<T, Int>)
            name = L"int";
        else if constexpr (std::same_as<T, Long>)
            name = L"long";
        else if constexpr (std::same_as<T, Float>)
            name = L"float";
        else if constexpr (std::same_as<T, Double>)
            name = L"double";

        exports.emplace(data.size(), name);
        data.emplace_back(TFlag<NamedTypeRepr>,
            name,
            false,
            static_cast<Int>(sizeof(T)),
            Str{L"::", mem}.append(name).append(L"::()"),
            Str{L"::", mem}.append(name).append(L"::~()")
        );
    });

    // named type - unit
    {
        exports.emplace(data.size(), Str{L"unit", mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{L"unit", mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{L"::unit::()", mem},
            Str{L"::unit::~()", mem}
        );
    }
    // named type address
    {
        exports.emplace(data.size(), Str{L"address", mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{L"address", mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{L"::address::()", mem},
            Str{L"::address::~()", mem}
        );
    }

    // named type instruction
    {
        exports.emplace(data.size(), Str{L"instruction", mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{L"instruction", mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{L"::instruction::()", mem},
            Str{L"::instruction::~()", mem}
        );
    }

    // f-family class
    {
        exports.emplace(data.size(), Str{L"FFamily", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"FFamily", mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"__super", L"::RTTObject"},
                Pair{L"base_addr", L"::address"},
                Pair{L"overrides", L"::address"}
            }, {mem}},
            List<ObjRefRepr>{{
                {L"::FFamily::()", mem},
                {L"::FFamily::~()", mem}
            }}, {}, {}});
    }

    // class of all f-override types
    {
        exports.emplace(data.size(), Str{L"FOverride", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"FOverride", mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"__super", L"::Type"},
                Pair{L"param_types", L"::address"},
                Pair{L"return_type", L"::address"}
            }, {mem}},
            List<ObjRefRepr>{{
                {L"::FOverride::()", mem},
                {L"::FOverride::~()", mem}
            }}, {}, {}});
    }

    // class of all array types
    {
        exports.emplace(data.size(), Str{L"ArrayType", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"ArrayType", mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"__super", L"::Type"},
                Pair{L"element_type", L"::address"},
                Pair{L"length", L"::long"}
            }, {mem}},
            List<ObjRefRepr>{{
                {L"::ArrayType::()", mem},
                {L"::ArrayType::~()", mem},
                {L"::ArrayType::constructor", mem},
                {L"::ArrayType::destructor", mem}
            }}, {}, {}});
    }

    // member-info class
    {
        exports.emplace(data.size(), Str{L"MemberInfo", mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{L"MemberInfo", mem},
            false,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{L"name", L"::address"},
                Pair{L"type", L"::address"},
                Pair{L"offset", L"::long"}
            }, {mem}},
            List<ObjRefRepr>{{
                {L"::MemberInfo::()", mem},
                {L"::MemberInfo::~()", mem}
            }}, {}, {}});
    }

    // todo f-override type
    {

    }

    // single override f-families
    {
        const auto address_t = Str{L"::address", mem};
        const auto unit_t = Str{L"::unit", mem};

        exports.emplace(data.size(), Str{L"RTUObject::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"RTUObject::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"RTTObject::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTTObject::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"RTTObject::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::~()", mem}
        }}, mem});


        const auto bool_t = Str{L"::bool", mem};
        const auto int_t = Str{L"::int", mem};


        exports.emplace(data.size(), Str{L"Type::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Type::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"Type::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Type::~()", mem}
        }}, mem});


        const auto long_t = Str{L"::long", mem};


        exports.emplace(data.size(), Str{L"ArrayType::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t, long_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"ArrayType::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"ArrayType::constructor", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::constructor", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"ArrayType::destructor", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::destructor", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"NamedType::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "NamedType::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"NamedType::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "NamedType::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"MemberInfo::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t, address_t, long_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "MemberInfo::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"MemberInfo::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "MemberInfo::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"Class::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t, address_t, address_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Class::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"Class::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Class::~()", mem}
        }}, mem});




        exports.emplace(data.size(), Str{L"FOverride::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FOverride::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"FOverride::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FOverride::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{L"FFamily::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FFamily::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"FFamily::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FFamily::~()", mem}
        }}, mem});


        const auto
            byte_t = Str{L"::byte", mem},
            char_t = Str{L"::char", mem},
            float_t = Str{L"::float", mem},
            double_t = Str{L"::double", mem};

#define OP(TYPE)\
        exports.emplace(data.size(), cvt(NStr{#TYPE, mem}) += L"::()");\
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{\
            List<ObjRefRepr>{{TYPE##_t}, mem},\
            unit_t,\
            FOverrideRepr::ImplRepr{TFlag<NtvId>, #TYPE "::()", mem}\
        }}, mem});\
        exports.emplace(data.size(), cvt(NStr{#TYPE, mem}) += L"::~()");\
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{\
            {},\
            unit_t,\
            FOverrideRepr::ImplRepr{TFlag<NtvId>, #TYPE  "::~()", mem}\
        }}, mem});

        OP(bool)
        OP(byte)
        OP(char)
        OP(int)
        OP(long)
        OP(float)
        OP(double)
        OP(address)

#undef OP

        exports.emplace(data.size(), Str{L"unit::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "unit::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"unit::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "unit::~()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"instruction::()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{byte_t, int_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "instruction::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{L"instruction::~()", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "instruction::~()", mem}
        }}, mem});

    }

    return {
        std::move(data),
        std::move(exports)
    };
}

Package Package::io(MManager *const mem) noexcept {
    auto data = List<Content>{mem};
    auto exports = Map<USize, ObjRefRepr>{mem};

    {
        exports.emplace(data.size(), Str{L"write", mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{
            FOverrideRepr{
                List<ObjRefRepr>{{{L"::char", mem}}, mem},
                ObjRefRepr{L"::unit", mem},
                FOverrideRepr::ImplRepr{TFlag<NtvId>, "write::0", mem}
            }
        }, mem});
    }

    return {
        std::move(data),
        std::move(exports)
    };
}
