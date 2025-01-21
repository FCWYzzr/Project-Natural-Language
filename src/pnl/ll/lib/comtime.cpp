//
// Created by FCWY on 25-1-8.
//
module;
#include "project-nl.h"
module pnl.ll.comtime;
import pnl.ll.base;
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
        exports.emplace(data.size(), Str{VM_TEXT("RTUObject"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("RTUObject"), mem},
                false,
                {},
                {{
                    {VM_TEXT("::RTUObject::()"), mem},
                    {VM_TEXT("::RTUObject::~()"), mem}
                }, {mem}}, {}, {}});
    }

    // rtt object class
    {
        exports.emplace(data.size(), Str{VM_TEXT("RTTObject"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("RTTObject"), mem},
                true,
                List<Pair<Str, ObjRefRepr>>{{
                    Pair{VM_TEXT("type"), VM_TEXT("::address")}
                }, {mem}},
                {{
                    {VM_TEXT("::RTTObject::()")},
                    {VM_TEXT("::RTTObject::~()")}
                }, {mem}}, {}, {}});
    }

    // class of all types: type class
    {
        exports.emplace(data.size(), Str{VM_TEXT("Type"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("Type"), mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("__super"), VM_TEXT("::RTTObject")},
                Pair{VM_TEXT("rtt"), VM_TEXT("::bool")},
                Pair{VM_TEXT(""), VM_TEXT("   ")},
                Pair{VM_TEXT("size"), VM_TEXT("::int")},
                Pair{VM_TEXT("maker"), VM_TEXT("::address")},
                Pair{VM_TEXT("collector"), VM_TEXT("::address")}
            }, {mem}},
            {{
                {VM_TEXT("::Type::()"), mem},
                {VM_TEXT("::Type::~()"), mem}
            }, {mem}}, {}, {}});
    }

    // class of all named types: named type class
    {
        exports.emplace(data.size(), Str{VM_TEXT("NamedType"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("NamedType"), mem},
                true,
                List<Pair<Str, ObjRefRepr>>{{
                    Pair{VM_TEXT("__super"), VM_TEXT("::Type")},
                    Pair{VM_TEXT("size"), VM_TEXT("::long")},
                    Pair{VM_TEXT("maker"), VM_TEXT("::address")},
                    Pair{VM_TEXT("collector"), VM_TEXT("::address")}
                }, {mem}},
                {{
                    {VM_TEXT("::NamedType::()"), mem},
                    {VM_TEXT("::NamedType::~()"), mem}
                }, {mem}}, {}, {}});
    }

    // class of all classes: class class
    {
        exports.emplace(data.size(), Str{VM_TEXT("Class"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("Class"), mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("__super"), VM_TEXT("::NamedType")},
                Pair{VM_TEXT("members"), VM_TEXT("::address")},
                Pair{VM_TEXT("methods"), VM_TEXT("::address")},
                Pair{VM_TEXT("static_members"), VM_TEXT("::address")},
                Pair{VM_TEXT("static_methods"), VM_TEXT("::address")}
            }, {mem}},
            List<ObjRefRepr>{{
                {VM_TEXT("::Class::()"), mem},
                {VM_TEXT("::Class::~()"), mem}
            }, {mem}}, {}, {}});
    }

    // named types - primitives

    TypePack<Bool, Byte, Char, Int, Long, Float, Double>::foreach([&]<typename T, std::size_t I>() {
        const Char* name;
        if constexpr (std::same_as<T, Bool>)
            name = VM_TEXT("bool");
        else if constexpr (std::same_as<T, Byte>)
            name = VM_TEXT("byte");
        else if constexpr (std::same_as<T, Char>)
            name = VM_TEXT("char");
        else if constexpr (std::same_as<T, Int>)
            name = VM_TEXT("int");
        else if constexpr (std::same_as<T, Long>)
            name = VM_TEXT("long");
        else if constexpr (std::same_as<T, Float>)
            name = VM_TEXT("float");
        else if constexpr (std::same_as<T, Double>)
            name = VM_TEXT("double");

        exports.emplace(data.size(), name);
        data.emplace_back(TFlag<NamedTypeRepr>,
            name,
            false,
            static_cast<Int>(sizeof(T)),
            Str{VM_TEXT("::"), mem}.append(name).append(VM_TEXT("::()")),
            Str{VM_TEXT("::"), mem}.append(name).append(VM_TEXT("::~()"))
        );
    });

    // named type - unit
    {
        exports.emplace(data.size(), Str{VM_TEXT("unit"), mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{VM_TEXT("unit"), mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{VM_TEXT("::unit::()"), mem},
            Str{VM_TEXT("::unit::~()"), mem}
        );
    }
    // named type address
    {
        exports.emplace(data.size(), Str{VM_TEXT("address"), mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{VM_TEXT("address"), mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{VM_TEXT("::address::()"), mem},
            Str{VM_TEXT("::address::~()"), mem}
        );
    }

    // named type instruction
    {
        exports.emplace(data.size(), Str{VM_TEXT("instruction"), mem});
        data.emplace_back(TFlag<NamedTypeRepr>,
            Str{VM_TEXT("instruction"), mem},
            false,
            static_cast<Int>(sizeof(std::uint64_t)),
            Str{VM_TEXT("::instruction::()"), mem},
            Str{VM_TEXT("::instruction::~()"), mem}
        );
    }

    // f-family class
    {
        exports.emplace(data.size(), Str{VM_TEXT("FFamily"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("FFamily"), mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("__super"), VM_TEXT("::RTTObject")},
                Pair{VM_TEXT("base_addr"), VM_TEXT("::address")},
                Pair{VM_TEXT("overrides"), VM_TEXT("::address")}
            }, {mem}},
            List<ObjRefRepr>{{
                {VM_TEXT("::FFamily::()"), mem},
                {VM_TEXT("::FFamily::~()"), mem}
            }}, {}, {}});
    }

    // class of all f-override types
    {
        exports.emplace(data.size(), Str{VM_TEXT("FOverride"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("FOverride"), mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("__super"), VM_TEXT("::Type")},
                Pair{VM_TEXT("param_types"), VM_TEXT("::address")},
                Pair{VM_TEXT("return_type"), VM_TEXT("::address")}
            }, {mem}},
            List<ObjRefRepr>{{
                {VM_TEXT("::FOverride::()"), mem},
                {VM_TEXT("::FOverride::~()"), mem}
            }}, {}, {}});
    }

    // class of all array types
    {
        exports.emplace(data.size(), Str{VM_TEXT("ArrayType"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("ArrayType"), mem},
            true,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("__super"), VM_TEXT("::Type")},
                Pair{VM_TEXT("element_type"), VM_TEXT("::address")},
                Pair{VM_TEXT("length"), VM_TEXT("::long")}
            }, {mem}},
            List<ObjRefRepr>{{
                {VM_TEXT("::ArrayType::()"), mem},
                {VM_TEXT("::ArrayType::~()"), mem},
                {VM_TEXT("::ArrayType::constructor"), mem},
                {VM_TEXT("::ArrayType::destructor"), mem}
            }}, {}, {}});
    }

    // member-info class
    {
        exports.emplace(data.size(), Str{VM_TEXT("MemberInfo"), mem});
        data.emplace_back(TFlag<ClassRepr>, ClassRepr{{VM_TEXT("MemberInfo"), mem},
            false,
            List<Pair<Str, ObjRefRepr>>{{
                Pair{VM_TEXT("name"), VM_TEXT("::address")},
                Pair{VM_TEXT("type"), VM_TEXT("::address")},
                Pair{VM_TEXT("offset"), VM_TEXT("::long")}
            }, {mem}},
            List<ObjRefRepr>{{
                {VM_TEXT("::MemberInfo::()"), mem},
                {VM_TEXT("::MemberInfo::~()"), mem}
            }}, {}, {}});
    }

    // single override f-families
    {
        const auto address_t = Str{VM_TEXT("::address"), mem};
        const auto unit_t = Str{VM_TEXT("::unit"), mem};

        exports.emplace(data.size(), Str{VM_TEXT("RTUObject::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("RTUObject::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("RTTObject::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTTObject::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("RTTObject::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "RTUObject::~()", mem}
        }}, mem});


        const auto bool_t = Str{VM_TEXT("::bool"), mem};
        const auto int_t = Str{VM_TEXT("::int"), mem};


        exports.emplace(data.size(), Str{VM_TEXT("Type::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Type::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("Type::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Type::~()", mem}
        }}, mem});


        const auto long_t = Str{VM_TEXT("::long"), mem};


        exports.emplace(data.size(), Str{VM_TEXT("ArrayType::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t, long_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("ArrayType::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("ArrayType::constructor"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::constructor", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("ArrayType::destructor"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "ArrayType::destructor", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("NamedType::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "NamedType::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("NamedType::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "NamedType::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("MemberInfo::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t, address_t, long_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "MemberInfo::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("MemberInfo::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "MemberInfo::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("Class::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, int_t, address_t, address_t, address_t, address_t, address_t, address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Class::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("Class::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "Class::~()", mem}
        }}, mem});




        exports.emplace(data.size(), Str{VM_TEXT("FOverride::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{bool_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FOverride::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("FOverride::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FOverride::~()", mem}
        }}, mem});



        exports.emplace(data.size(), Str{VM_TEXT("FFamily::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{address_t, address_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FFamily::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("FFamily::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "FFamily::~()", mem}
        }}, mem});


        const auto
            byte_t = Str{VM_TEXT("::byte"), mem},
            char_t = Str{VM_TEXT("::char"), mem},
            float_t = Str{VM_TEXT("::float"), mem},
            double_t = Str{VM_TEXT("::double"), mem};

#define OP(TYPE)\
        exports.emplace(data.size(), cvt(MBStr{#TYPE, mem}) += VM_TEXT("::()"));\
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{\
            List<ObjRefRepr>{{TYPE##_t}, mem},\
            unit_t,\
            FOverrideRepr::ImplRepr{TFlag<NtvId>, #TYPE "::()", mem}\
        }}, mem});\
        exports.emplace(data.size(), cvt(MBStr{#TYPE, mem}) += VM_TEXT("::~()"));\
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

        exports.emplace(data.size(), Str{VM_TEXT("unit::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "unit::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("unit::~()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            {},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "unit::~()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("instruction::()"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{FOverrideRepr{
            List<ObjRefRepr>{{byte_t, int_t}, mem},
            unit_t,
            FOverrideRepr::ImplRepr{TFlag<NtvId>, "instruction::()", mem}
        }}, mem});

        exports.emplace(data.size(), Str{VM_TEXT("instruction::~()"), mem});
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
        exports.emplace(data.size(), Str{VM_TEXT("write"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{
            FOverrideRepr{
                List<ObjRefRepr>{{{VM_TEXT("::char"), mem}}, mem},
                ObjRefRepr{VM_TEXT("::unit"), mem},
                FOverrideRepr::ImplRepr{TFlag<NtvId>, "write::0", mem}
            }
        }, mem});
        // todo more writable object
    }

    {
        exports.emplace(data.size(), Str{VM_TEXT("println"), mem});
        data.emplace_back(TFlag<FFamilyRepr>, FFamilyRepr{{
            FOverrideRepr{
                List<ObjRefRepr>{{{VM_TEXT("::address"), mem}}, mem},
                ObjRefRepr{VM_TEXT("::unit"), mem},
                FOverrideRepr::ImplRepr{TFlag<NtvId>, "println::0", mem}
            }
        }, mem});
    }

    return {
        std::move(data),
        std::move(exports)
    };
}
