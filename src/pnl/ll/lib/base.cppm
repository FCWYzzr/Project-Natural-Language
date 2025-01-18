//
// Created by FCWY on 24-12-16.
//

export module pnl.ll.base;
import pnl.ll.meta_prog;
export import <string>;
export import <type_traits>;
export import <vector>;
export import <memory>;
export import <optional>;
export import <functional>;
export import <iostream>;
export import <unordered_map>;
export import <unordered_set>;
import <span>;
export import <cstdint>;
export import <variant>;
export import <queue>;
export import <memory_resource>;
export import <regex>;


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
        using Char      = wchar_t;

        using Int       = std::int32_t;
        using Long      = std::int64_t;
        using Float     = float;
        using Double    = double;
        template<typename T=unsigned char>
        using Ref       = T*;
        template<typename T=unsigned char>
        using View      = const T*;


        consteval Char      operator""_chr      (const wchar_t c) { return c; }
        consteval Byte      operator""_byte     (const size_t v) { return static_cast<Byte>(v); }
        consteval Int       operator""_int      (const size_t v) { return static_cast<Int>(v); }
        consteval Long      operator""_long     (const size_t v) { return static_cast<Long>(v); }
        consteval Float     operator""_float    (const long double v) { return static_cast<Float>(v); }
        consteval Double    operator""_double   (const long double v) { return static_cast<Double>(v); }
    }





    // native types
    inline namespace native {
        using UByte     = unsigned char;
        using IStream   = std::basic_istream<UByte>;
        using OStream   = std::basic_ostream<UByte>;

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
        using NStr = std::pmr::string;
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

        template<typename T>
        auto& TFlag = std::in_place_type<T>;
        template<typename T>
        using ListV = std::span<T>;

        using StrV = std::basic_string_view<Char>;

        using NOPType = std::monostate;




        using Path = std::filesystem::path;
        using MManager = std::pmr::memory_resource;
        using Regex = std::basic_regex<Char>;
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
    concept Trivial  =  std::is_trivially_copy_constructible_v<T>;

}

export namespace pnl::ll::inline conditions{
    PNL_LIB_PREFIX
    void assert(bool condition, const Str& err_desc) noexcept;
    PNL_LIB_PREFIX
    void assert(bool condition, const NStr& err_desc) noexcept;
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
    requires !std::is_default_constructible_v<std::hash<C>>
        && ReadonlyIterable<C> && Hashable<typename C::value_type>
    struct Hasher<C> {
        constexpr size_t operator() (const C& lv) const noexcept {
            auto hash = std::hash<typename C::value_type>{};

            size_t h = 0;
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

        RETURN,



        ARG_FLAG,



        JUMP,
        JUMP_IF_ZERO,
        JUMP_IF_NOT_ZERO,
        JUMP_IF_POSITIVE,
        JUMP_IF_NEGATIVE,


        ALL(LOAD),
        ALL(STORE),
        REF_AT,
        ALL(FROM_REF_LOAD),
        ALL(TO_REF_STORE),

        REF_MEMBER,
        REF_STATIC_MEMBER,

        // invoke top's mem func $i 's first override
        INVOKE_OVERRIDE,

        // push top's mem func $i
        REF_METHOD,
        REF_STATIC_METHOD,

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


        template<std::integral T>
        explicit operator T () = delete;
        template<std::integral T>
        explicit Instruction(T) = delete;
    };

    Instruction operator + (const OPCode rv1, const std::uint32_t rv2) noexcept {
        return Instruction{rv1, rv2};
    }
    Instruction operator ++ (const OPCode rv1, const int) noexcept {
        return Instruction{rv1, 0};
    }

}
