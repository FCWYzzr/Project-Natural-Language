//
// Created by FCWY on 24-12-16.
//
module;
#include "project-nl.h"
export module pnl.ll.base;

export namespace pnl::ll::inline meta_prog{
    template<typename T, typename ...Ts>
    struct TypePack{
        static std::variant<T, Ts...>
            construct(const std::size_t idx) {
            if (idx == 0) [[unlikely]]
                return {std::in_place_type<T>};
            if constexpr (sizeof...(Ts) > 0)
                return std::visit([]<typename U>(U&& v) -> std::variant<T, Ts...> {
                    return {std::in_place_type<U>, v};
                }, TypePack<Ts...>::construct(idx - 1));
            else
                std::unreachable();
        }

        template<typename Visitor, std::size_t I=0>
        static void foreach(Visitor&& visitor) {
            visitor.template operator()<T, I>();
            if constexpr (sizeof...(Ts) > 0)
                TypePack<Ts...>::template foreach<Visitor, I+1>(std::forward<Visitor>(visitor));
        }

        template<typename U>
        constexpr static bool contains = std::same_as<T, U> ||
            (std::same_as<Ts, U> || ...);
    };
}

export namespace pnl::ll::inline hash{
    template<typename T>
    struct Hasher;
}

export namespace pnl::ll::inline base {

    // nl primitives
    inline namespace nl {
        enum class
              Unit:       bool;
        using Bool      = bool;
        using Byte      = signed char;
        using Char      = char32_t;

        using Int       = std::int32_t;
        using Long      = std::int64_t;
        using Float     = float;
        using Double    = double;
        template<typename T=unsigned char>
        using Ref       = T*;
        template<typename T=unsigned char>
        using View      = const T*;


    }





    // native types
    inline namespace native {
        using UByte     = unsigned char;
        using BIStream   = std::basic_istream<UByte>;
        using BOStream   = std::basic_ostream<UByte>;
        using BIFStream   = std::basic_fstream<UByte>;
        using BOFStream   = std::basic_fstream<UByte>;

        using USize     = std::uint64_t;
        using SSize     = std::int64_t;
        template<typename T>
        auto size_cast(const T& v) noexcept {
            return static_cast<USize>(v);
        }

        enum class TypeTag: UByte {
            BOOL, BYTE, CHAR,
            INT, LONG,
            FLOAT, DOUBLE,
            OBJECT
        };

        template <typename T>
        using List = std::pmr::vector<T>;

        using Str = std::pmr::basic_string<Char>;
        using MBStr = std::pmr::string;
        template<typename KT, typename VT, typename Hasher = Hasher<KT>, typename KeyEq = std::equal_to<KT>>
        using Map = std::pmr::unordered_map<KT, VT, Hasher, KeyEq>;
        template<typename T>
        using Dict = std::pmr::unordered_map<Str, T, Hasher<Str>>;
        template<typename T, typename Hasher = Hasher<T>, typename KeyEq = std::equal_to<T>>
        using Set  = std::pmr::unordered_set<T, Hasher, KeyEq>;
        template<typename T>
        using PriQue = std::priority_queue<T, List<T>>;


        template<typename T, typename U>
        using Pair = std::pair<T, U>;

        template<std::size_t I>
        auto& IFlag = std::in_place_index<I>;
        template<typename T>
        auto& TFlag = std::in_place_type<T>;
        template<typename T>
        using ListV = std::span<T>;

        using StrV = std::basic_string_view<Char>;

        using NOPType = std::monostate;


        using Path = std::filesystem::path;
        using MManager = std::pmr::memory_resource;
        template<typename ...Ts>
        using Tuple = std::tuple<Ts...>;
    }

}

export namespace pnl::ll::inline base_traits{
    template<typename T>
    concept Primitive   =   std::same_as<T, Char>
                        or  std::same_as<T, Bool>
                        or  std::same_as<T, Byte>
                        or  std::same_as<T, Int>
                        or  std::same_as<T, Long>
                        or  std::same_as<T, Float>
                        or  std::same_as<T, Double>
                        or  std::same_as<T, Ref<>>
                        or  std::same_as<T, View<>>;

    template<typename T>
    concept Numeric
                    =   std::same_as<T, Byte>
                    or  std::same_as<T, Int>
                    or  std::same_as<T, Long>
                    or  std::same_as<T, Float>
                    or  std::same_as<T, Double>;
    template<typename T>
    concept Integral
                        =   std::same_as<T, Int>
                        or  std::same_as<T, Long>;

    template<typename T>
    concept Computable
                    =   std::same_as<T, Int>
                    or  std::same_as<T, Long>
                    or  std::same_as<T, Float>
                    or  std::same_as<T, Double>;

    template<typename T>
    concept Trivial  =  std::is_trivially_copy_constructible_v<T>
                        && std::is_standard_layout_v<T>;

}

export namespace pnl::ll::inline codecvt{
    auto ntv_encoding = "UTF-8";
    constexpr auto vm_encoding = "UTF-32LE";

    PNL_LIB_PREFIX
    Str cvt(const std::string& in, MManager& mem) noexcept;
    PNL_LIB_PREFIX
    Str cvt(const MBStr& in) noexcept;
    PNL_LIB_PREFIX
    MBStr cvt(const Str& in) noexcept;

    PNL_LIB_PREFIX
    std::size_t code_cvt(char* out, std::size_t out_size, const char* in, std::size_t in_size, const char* code_in, const char* code_out) noexcept;
}

export namespace pnl::ll::inline conditions{
    PNL_LIB_PREFIX
    void assert(bool condition, const Str& err_desc) noexcept;
    PNL_LIB_PREFIX
    void assert(bool condition, const MBStr& err_desc) noexcept;
}

export namespace pnl::ll::inline native_traits{
    template<typename>
    struct Repr;
    template<typename I>
    requires std::integral<I> && (sizeof(I) == sizeof(std::uint8_t))
    struct Repr<I> {
        using Int = std::uint8_t;
    };
    template<typename I>
    requires std::integral<I> && (sizeof(I) == sizeof(std::uint16_t))
    struct Repr<I> {
        using Int = std::uint16_t;
    };
    template<typename I>
    requires std::integral<I> && (sizeof(I) == sizeof(std::uint32_t))
    struct Repr<I> {
        using Int = std::uint32_t;
    };
    template<typename I>
    requires std::integral<I> && (sizeof(I) == sizeof(std::uint64_t))
    struct Repr<I> {
        using Int = std::uint64_t;
    };
    template<typename F>
    requires std::floating_point<F> && (sizeof(F) == sizeof(std::uint32_t))
    struct Repr<F>{
        using Int = std::uint32_t;
    };
    template<typename F>
    requires std::floating_point<F> && (sizeof(F) == sizeof(std::uint64_t))
    struct Repr<F> {
        static_assert(sizeof(int64_t) == sizeof(F));
        using Int = std::uint64_t;
    };

    template<typename C>
    concept ReadonlyIterable = std::input_iterator<typename C::const_iterator>
        && std::forward_iterator<typename C::const_iterator>;

    template<typename C>
    concept Iterable = ReadonlyIterable<C>
        && std::input_or_output_iterator<typename C::iterator>
        && std::forward_iterator<typename C::iterator>;


    template<typename C>
    concept AAContainer = Iterable<C>
        && requires(
            C c,
            const C cc,
            USize neo_size,
            MManager* mem){

        C{mem};
        c.clear();
        static_cast<USize>(c.size());
        {c.begin()} -> std::same_as<typename C::iterator>;
        {c.end()} -> std::same_as<typename C::iterator>;
        {cc.begin()} -> std::same_as<typename C::const_iterator>;
        {cc.end()} -> std::same_as<typename C::const_iterator>;
    };

    template<typename C>
    concept SequencialContainer = AAContainer<C> && requires(
        C c, USize neo_size){
        c.reserve(neo_size);
        c.resize(neo_size);

    };

    template<typename C>
    concept AssociativeContainer = AAContainer<C> && requires(
        C c,
        typename C::value_type&& v){
        c.emplace(v);
    };

    template<typename T>
    concept Delegatable
        =  requires{typename T::Delegated;}
        && std::convertible_to<T, typename T::Delegated>
        ;

}

export namespace pnl::ll::inline hash{
    template<typename T>
    concept Hashable = std::is_default_constructible_v<Hasher<T>>;

    // make std's hashable hashable
    template<typename T>
    requires std::is_default_constructible_v<std::hash<T>>
    struct Hasher<T>: std::hash<T> {};

    // make hashable container hashable (except std already given)
    template<typename C>
    requires ((!std::is_default_constructible_v<std::hash<C>>)
        && (ReadonlyIterable<C> && Hashable<typename C::value_type>))
    struct Hasher<C> {
        constexpr std::size_t operator() (const C& lv) const noexcept {
            auto hash = std::hash<typename C::value_type>{};

            std::size_t h = 0;
            for (auto& v: lv) {
                constexpr auto BASE = 13;
                constexpr auto MASK = 0xffffff;
                h += hash(v);
                h *= BASE;
                h &= MASK;
            }
            return h;
        }
    };


    // fallback
    template<typename>
    struct Hasher {
        Hasher()=delete;
    };
}


#define INTEGRAL(prefix) \
prefix##_INT,        \
prefix##_LONG

#define COMPUTABLE(prefix)  \
INTEGRAL(prefix),       \
prefix##_FLOAT,         \
prefix##_DOUBLE

#define ALL(prefix)  \
prefix##_BOOL,       \
prefix##_CHAR,       \
prefix##_BYTE,       \
COMPUTABLE(prefix),  \
prefix##_REF


export namespace pnl::ll::inline vm{
    enum class OPCode: std::uint8_t {
        NOP,

        WASTE,

        CAST_C2I,

        CAST_B2I,

        CAST_I2B,
        CAST_I2C,
        CAST_I2L,
        CAST_I2F,
        CAST_I2D,

        CAST_L2I,
        CAST_L2F,
        CAST_L2D,

        CAST_F2I,
        CAST_F2L,
        CAST_F2D,

        CAST_D2I,
        CAST_D2L,
        CAST_D2F,

        // stack[-1] __ stack[-2]
        CMP,

        ADD,
        SUB,
        MUL,
        DIV,

        REM,

        NEG,
        SHL,
        SHR,
        USHR,
        BIT_AND,
        BIT_OR,
        BIT_XOR,
        BIT_INV,

        // invoke top's first override
        INVOKE_FIRST,

        // end proc & clear stack
        // note! alloced objs must destruct manually
        RETURN,

        // alloc an object which die as func return
        STACK_ALLOC,
        STACK_NEW,

        // alloc unmanaged
        WILD_ALLOC,
        WILD_NEW,
        WILD_COLLECT,


        DESTROY,


        ARG_FLAG,



        JUMP,
        JUMP_IF_ZERO,
        JUMP_IF_NOT_ZERO,
        JUMP_IF_POSITIVE,
        JUMP_IF_NEGATIVE,


        ALL(P_LOAD),
        ALL(F_LOAD),
        ALL(P_STORE),
        ALL(F_STORE),
        P_REF_AT,
        T_REF_AT,
        ALL(FROM_REF_LOAD),
        ALL(TO_REF_STORE),

        REF_MEMBER,
        REF_STATIC_MEMBER,

        // invoke top's mem func $i 's first override
        INVOKE_OVERRIDE,

        // push top's mem func $i
        REF_METHOD,
        REF_STATIC_METHOD,

        INSTATE,
    };

    struct alignas(std::uint32_t)
    Instruction{
        std::uint32_t content;

        Instruction(
            const OPCode opcode,
            const std::uint32_t arg) noexcept: content(
                (static_cast<std::uint8_t>(opcode) << 24)
                | (arg & 0xffffff)){}

        [[nodiscard]]
        OPCode opcode() const noexcept {
            return static_cast<OPCode>(content >> 24);
        }

        [[nodiscard]]
        std::uint32_t arg() const noexcept {
            return content & 0xffffff;
        }


        bool operator==(const Instruction &) const noexcept = default;
        template<std::integral T>
        explicit operator T () = delete;
        template<std::integral T>
        explicit Instruction(T) = delete;
    };
    static_assert(Trivial<Instruction>);

    Instruction operator + (const OPCode rv1, const std::uint32_t rv2) noexcept {
        return Instruction{rv1, rv2};
    }
    Instruction operator ++ (const OPCode rv1, const int) noexcept {
        return Instruction{rv1, 0};
    }
}
