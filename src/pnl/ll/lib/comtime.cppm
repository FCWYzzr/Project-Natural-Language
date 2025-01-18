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

export module pnl.ll.comtime;
import pnl.ll.base;
import pnl.ll.collections;
import <utility>;
import <variant>;
import <ranges>;


export namespace pnl::ll::inline comtime{
    // objects live in compile time
    struct ObjectRepr;
    struct ClassRepr;
    struct NamedTypeRepr;
    struct FOverrideRepr;
    using FFamilyRepr = List<FOverrideRepr>;


    using ObjRefRepr = Str;
    using NtvId = NStr;


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
        bool rtt;
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


        ClassRepr(Str name, const bool rtt, List<Pair<Str, ObjRefRepr>> member_list, List<ObjRefRepr> method_list,
            List<ObjRefRepr> static_member_list, List<ObjRefRepr> static_method_list)
            noexcept: name(std::move(name)),
              rtt(rtt),
              member_list(std::move(member_list)),
              method_list(std::move(method_list)),
              static_member_list(std::move(static_member_list)),
              static_method_list(std::move(static_method_list)) {
        }

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
        bool rtt;
        Int size;
        ObjRefRepr maker;
        ObjRefRepr collector;

        using Delegated = std::tuple<
            Str&,
            bool&,
            Int&,
            ObjRefRepr&,
            ObjRefRepr&
        >;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept {
            return std::tie(name, rtt, size, maker, collector);
        }
    };

    struct PNL_LIB_PREFIX ObjectRepr{
        ObjRefRepr
            type;
        USize
            constructor_override_id;
        List<CTValue>
            constructor_params;

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
            Map<USize, ObjRefRepr> exports) noexcept:
            data(std::move(data)),
            exports(std::move(exports)){}



        Package(const Package &other) noexcept = default;

        Package(Package &&other) noexcept:
            data(std::move(other.data)),
            exports(std::move(other.exports))
        {}


        Package & operator=(const Package &other) noexcept{
            if (this == &other)
                return *this;
            data = other.data;
            exports = other.exports;
            return *this;
        }

        Package & operator=(Package &&other) noexcept {
            if (this == &other)
                return *this;
            data = std::move(other.data);
            exports = std::move(other.exports);
            return *this;
        }


        using Delegated = std::tuple<List<Content>&, Map<USize, ObjRefRepr>&>;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept;

        static Package anonymous_builtin(MManager*) noexcept;
        static Package io(MManager*) noexcept;
    };
}