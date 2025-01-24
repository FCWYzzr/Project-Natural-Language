//
// Created by FCWY on 24-12-31.
//

/**
 * different from runtime::xxx,
 * comtime::xxx is platform independent,
 * with rich meta info,
 * ready to co-operate with others' objects,
 * and serializable (using pnl.ll:serialise)
 */
module;
#include "project-nl.h"
export module pnl.ll.comtime;
import pnl.ll.base;
import pnl.ll.collections;



export namespace pnl::ll::inline comtime{
    // objects live in compile time
    struct ObjectRepr;
    struct ClassRepr;
    struct NamedTypeRepr;
    struct FOverrideRepr;
    using FFamilyRepr = List<FOverrideRepr>;


    using ObjRefRepr = Str;
    using NtvId = MBStr;


    using CTValue = std::variant<
        Bool, Byte, Char,
        Int, Long, Float, Double,
        ObjRefRepr
    >;





    struct PNL_LIB_PREFIX FOverrideRepr {
        using ImplRepr = std::variant<List<Instruction>, NtvId>;
        List<ObjRefRepr> arg_ts;
        ObjRefRepr ret_t;
        ImplRepr   impl;

        using Delegated = std::tuple<
            List<ObjRefRepr>&,
            ObjRefRepr&,
            ImplRepr&
        >;
        bool operator==(const FOverrideRepr &o) const noexcept {
            return arg_ts == o.arg_ts && ret_t == o.ret_t && impl == o.impl;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () {
            return std::tie(
                arg_ts,
                ret_t,
                impl
            );
        }

    };

    struct PNL_LIB_PREFIX ClassRepr{
        Str name;
        ObjRefRepr maker;
        ObjRefRepr collector;
        // name -> type
        List<Pair<Str, ObjRefRepr>>
            member_list;
        List<ObjRefRepr>
            method_list;
        // name -> global obj
        List<ObjRefRepr>
            static_member_list;
        List<ObjRefRepr>
            static_method_list;


        ClassRepr(Str name, ObjRefRepr maker, ObjRefRepr collector, List<Pair<Str, ObjRefRepr>> member_list,
            List<ObjRefRepr> method_list, List<ObjRefRepr> static_member_list, List<ObjRefRepr> static_method_list)
            noexcept: name(std::move(name)),
              maker(std::move(maker)),
              collector(std::move(collector)),
              member_list(std::move(member_list)),
              method_list(std::move(method_list)),
              static_member_list(std::move(static_member_list)),
              static_method_list(std::move(static_method_list)) {
        }

        bool operator==(const ClassRepr &) const noexcept = default;

        using Delegated = std::tuple<
            List<Pair<Str, ObjRefRepr>>&,
            List<ObjRefRepr>&,
            List<ObjRefRepr>&,
            List<ObjRefRepr>&
        >;
        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;
    };

    struct PNL_LIB_PREFIX NamedTypeRepr {
        Str name;
        Long size;
        ObjRefRepr maker;
        ObjRefRepr collector;

        using Delegated = std::tuple<
            Str&,
            Long&,
            ObjRefRepr&,
            ObjRefRepr&
        >;

        bool operator==(const NamedTypeRepr &) const noexcept = default;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept {
            return std::tie(name, size, maker, collector);
        }
    };

    struct PNL_LIB_PREFIX ObjectRepr{
        ObjRefRepr
            type;
        USize
            constructor_override_id;
        List<CTValue>
            constructor_params;

        bool operator==(const ObjectRepr &) const noexcept = default;

        using Delegated = std::tuple<ObjRefRepr&, USize&, List<CTValue>&>;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;
    };

    struct CharArrayRepr: Str {};



    struct PNL_LIB_PREFIX Package{
        using Content = std::variant<
            Bool, Byte, Char,
            Int, Long, Float, Double,
            NamedTypeRepr,

            ObjRefRepr,
            FFamilyRepr,
            ClassRepr,
            ObjectRepr,
            CharArrayRepr
        >;

        List<Content>           data;
        Map<USize, ObjRefRepr>  exports;


        Package(
            List<Content> data,
            Map<USize, ObjRefRepr> exports) noexcept;


        Package(const Package &other) noexcept = default;

        Package(Package &&other) noexcept;


        Package & operator=(const Package &other) noexcept;

        Package & operator=(Package &&other) noexcept;

        // for package equivalant check
        bool operator == (const Package &) const noexcept = default;


        using Delegated = std::tuple<List<Content>&, Map<USize, ObjRefRepr>&>;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;

        static Package anonymous_builtin(MManager*) noexcept;
        static Package io(MManager*) noexcept;
    };
}