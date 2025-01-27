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


    struct ReferenceRepr {
        using Delegated = Str;
        Str v;

        ReferenceRepr(Str s={}) noexcept:
            v(std::move(s)) {}

        bool operator==(const ReferenceRepr &o) const noexcept {
            return v == o.v;
        }

        bool operator==(const Str &o) const noexcept {
            return v == o;
        }
        // ReSharper disable once CppNonExplicitConversionOperator
        operator Str& () & {
            return v;
        }
        operator const Str& () const & {
            return v;
        }
    };
    struct CharArrayRepr {
        using Delegated = Str;
        Str v;

        CharArrayRepr(Str s={}) noexcept:
            v(std::move(s)) {}

        bool operator==(const CharArrayRepr &o) const noexcept {
            return v == o.v;
        }

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Str& () & {
            return v;
        }
        operator const Str& () const & {
            return v;
        }
    };
    using NtvId = MBStr;


    using CTValue = std::variant<
        Bool, Byte, Char,
        Int, Long, Float, Double,
        ReferenceRepr
    >;





    struct PNL_LIB_PREFIX FOverrideRepr {
        using ImplRepr = std::variant<List<Instruction>, NtvId>;
        List<ReferenceRepr> arg_ts{};
        ReferenceRepr ret_t{{}};
        ImplRepr   impl{};

        using Delegated = std::tuple<
            List<ReferenceRepr>&,
            ReferenceRepr&,
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
        Str name{};
        ReferenceRepr maker{{}};
        ReferenceRepr collector{{}};
        // name -> type
        List<Pair<Str, ReferenceRepr>>
            member_list{};
        List<ReferenceRepr>
            method_list{};
        // name -> global obj
        List<ReferenceRepr>
            static_member_list{};
        List<ReferenceRepr>
            static_method_list{};

        bool operator==(const ClassRepr &) const noexcept = default;

        using Delegated = std::tuple<
            Str&,
            ReferenceRepr&,
            ReferenceRepr&,
            List<Pair<Str, ReferenceRepr>>&,
            List<ReferenceRepr>&,
            List<ReferenceRepr>&,
            List<ReferenceRepr>&
        >;
        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;
    };

    struct PNL_LIB_PREFIX NamedTypeRepr {
        Str name{};
        Long size{};
        ReferenceRepr maker{{}};
        ReferenceRepr collector{{}};

        using Delegated = std::tuple<
            Str&,
            Long&,
            ReferenceRepr&,
            ReferenceRepr&
        >;

        bool operator==(const NamedTypeRepr &) const noexcept = default;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept {
            return std::tie(name, size, maker, collector);
        }
    };

    struct PNL_LIB_PREFIX ObjectRepr{
        ReferenceRepr
            type{{}};
        USize
            constructor_override_id{};
        List<CTValue>
            constructor_params{};

        bool operator==(const ObjectRepr &) const noexcept = default;

        using Delegated = std::tuple<ReferenceRepr&, USize&, List<CTValue>&>;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;
    };





    struct PNL_LIB_PREFIX Package{
        using Content = std::variant<
            Bool, Byte, Char,
            Int, Long, Float, Double,
            NamedTypeRepr,

            ReferenceRepr,
            FFamilyRepr,
            ClassRepr,
            ObjectRepr,
            CharArrayRepr
        >;

        List<Content>           data;
        Map<USize, ReferenceRepr>  exports;

        explicit Package(MManager* mem) noexcept;

        Package(
            List<Content> data,
            Map<USize, ReferenceRepr> exports) noexcept;


        Package(const Package &other) noexcept = default;

        Package(Package &&other) noexcept;


        Package & operator=(const Package &other) noexcept;

        Package & operator=(Package &&other) noexcept;

        // for package equivalant check
        bool operator == (const Package &) const noexcept = default;


        using Delegated = std::tuple<List<Content>&, Map<USize, ReferenceRepr>&>;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;
    };

    // format: Arr<T>
    PNL_LIB_PREFIX
    Str typename_arr(
        MManager*
            mem,
        const Char* type,
        USize size);

    // format: (p1t, p2t, <1>, p3t, <2>, ...) -> ret/<i>
    PNL_LIB_PREFIX
    Str typename_fun(
            MManager*
                mem,
            const List<ReferenceRepr>&   param_sigs,
            const ReferenceRepr&         ret_sig) noexcept;


}