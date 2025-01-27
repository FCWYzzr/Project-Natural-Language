//
// Created by FCWY on 24-12-28.
//
// ReSharper disable CppNonExplicitConvertingConstructor
module;
#include "project-nl.h"

export module pnl.ll.serialize;
import pnl.ll.string;
import pnl.ll.base;
import pnl.ll.collections;
import pnl.ll.runtime;
import pnl.ll.comtime;



export namespace pnl::ll::inline serialize{

    template<typename T=void>
    struct Serializer;

    template<typename S>
    concept FitSerializer
        =  requires{typename S::value_type;}
        && std::derived_from<S, Serializer<>>
        && std::constructible_from<S, typename S::value_type&>;

    template<typename T>
    concept Serializable = FitSerializer<Serializer<T>>
        && Serializer<T>::functional;
    template<typename ...Ts>
    concept AllSerializable = (Serializable<Ts> && ...);


    template<>
    struct Serializer<void> {
        constexpr static bool functional = false;
        enum class Endian {
            BIG,
            LITTLE
        };

        inline static auto endian = Endian::LITTLE;
    };

    template<typename T>
    BIStream& operator >> (BIStream& is, const Serializer<T>& obj) noexcept {
        obj.deserialize(is);
        return is;
    }
    template<typename T>
    BOStream& operator << (BOStream& os, const Serializer<T>& obj) noexcept {
        obj.serialize(os);
        return os;
    }


    template<typename T>
    requires std::is_const_v<T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = Serializer<std::remove_const_t<T>>::functional;
        using value_type = T;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        // enable const or not
        void serialize(BOStream& os) const noexcept {
            Serializer<std::remove_const_t<T>>{const_cast<std::remove_const_t<T>&>(bind)}.serialize(os);
        }

        // enable only non-const
        void deserialize(BIStream&) const noexcept=delete;
    };

    template<typename T>
    requires Delegatable<T>
        && Serializable<typename T::Delegated>
        && !std::is_const_v<T>
    struct Serializer<T> final: Serializer<> {
        using D = typename T::Delegated;
        constexpr static bool functional = Serializer<D>::functional;
        using value_type = T;


        value_type& bind;

        Serializer(value_type &bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<D>{bind};
        }
        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<D>{bind};
        }
    };


    template<std::integral T>
    requires !std::is_const_v<T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = T;
        using RInt = typename Repr<T>::Int;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        // enable const or not
        void serialize(BOStream& os) const noexcept {
            UByte repr[sizeof(RInt)]{};
            auto value = reinterpret_cast<const RInt&>(bind);
            int start, step, stop;
            if (endian == Endian::BIG) {
                start = sizeof(T) - 1;
                step = -1;
                stop = -1;
            }
            else {
                start = 0;
                step = 1;
                stop = sizeof(T);
            }

            for (auto i = start; i != stop; i += step) {
                repr[i] = static_cast<UByte>(value & 0xff);
                value >>= 8;
            }
            os.write(repr, sizeof(T));
        }

        // enable only non-const
        void deserialize(BIStream& is) const noexcept {
            UByte repr[sizeof(RInt)]{};
            is.read(repr, sizeof(T));
            RInt v = 0;

            int start, step, stop;
            if (endian == Endian::LITTLE) {
                start = sizeof(T) - 1;
                step = -1;
                stop = -1;
            }
            else {
                start = 0;
                step = 1;
                stop = sizeof(T);
            }

            for (auto i = start; i != stop; i += step) {
                v <<= 8;
                v |= repr[i];
            }

            bind = reinterpret_cast<const value_type&>(v);
        }
    };



    template<typename E>
    requires std::is_enum_v<E> && !std::is_const_v<E>
    struct Serializer<E> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = E;
        using RInt = std::underlying_type_t<E>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const RInt>{static_cast<RInt>(bind)};
        }
        void deserialize(BIStream& is) const noexcept {
            RInt cache;
            is >> Serializer<RInt>{cache};
            bind = static_cast<E>(cache);
        }
    };



    template<>
    struct Serializer<Instruction> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = Instruction;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<std::uint32_t>{bind.content};
        }
        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<std::uint32_t>{bind.content};
        }
    };



    template<std::floating_point T>
    requires !std::is_const_v<T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = T;
        using RInt = typename Repr<T>::Int;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<RInt>{reinterpret_cast<RInt&>(bind)};
        }
        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<RInt>{reinterpret_cast<RInt&>(bind)};
        }
    };



    template<typename C>
    requires SequencialContainer<C> && !std::is_const_v<C>
    struct Serializer<C> final: Serializer<> {
        constexpr static bool functional = Serializable<typename C::value_type>;
        using value_type = C;
        using T = typename C::value_type;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}


        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};

            for (const auto& elem: bind)
                os << Serializer<const T>{elem};
        }

        void deserialize(BIStream& is) const noexcept {
            USize size;
            is >> Serializer<USize>{size};
            bind.resize(size);


            for (auto& e: bind)
                is >> Serializer<T>{e};
        }
    };

    template<typename Ch, typename Alloc>
    struct Serializer<std::basic_string<Ch, Alloc>> final: Serializer<> {
        constexpr static bool functional = Serializable<Ch>;
        using value_type = std::basic_string<Ch, Alloc>;
        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};
            for (auto& ch: bind)
                os << Serializer<const Ch>{ch};
        }

        void deserialize(BIStream& is) const noexcept {
            USize size;
            is >> Serializer<USize>{size};
            bind.resize_and_overwrite(size + 1, [&](Ch* buf, const std::size_t) {
                for (auto i: std::views::iota(0u, size))
                    is >> Serializer<Ch>{buf[i]};
                return size;
            });
        }
    };
    static_assert(Serializable<std::string>);



    template<typename C>
    requires AssociativeContainer<C> && !std::is_const_v<C>
    struct Serializer<C> final: Serializer<> {
        constexpr static bool functional = Serializable<typename C::value_type>;
        using value_type = C;
        using T = typename C::value_type;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};

            for (auto& elem: bind)
                os << Serializer<const T>{elem};
        }

        void deserialize(BIStream& is) const noexcept  {
            USize size;

            is >> Serializer<USize>{size};
            bind.clear();

            for (const auto [[maybe_unused]] _: std::views::iota(0u, size)) {
                T cache{};
                is >> Serializer<T>{cache};
                cache.emplace(std::move(cache));
            }
        }
    };

    template<typename T, typename U>
    struct Serializer<Map<T, U>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<T, U>;
        using value_type = Map<T, U>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};
            for (auto& elem: bind) {
                os << Serializer<const T>{elem.first};
                os << Serializer<const U>{elem.second};
            }
        }
        void deserialize(BIStream& is) const noexcept {
            USize size;
            is >> Serializer<USize>{size};
            for (auto [[maybe_unused]]_: std::views::iota(0u, size)) {
                T key{};
                U value{};
                is >> Serializer<T>{key};
                is >> Serializer<U>{value};
                bind.emplace(std::move(key), std::move(value));
            }
        }
    };

    template<typename T, typename U>
    struct Serializer<BiMap<T, U>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<T, U>;
        using value_type = BiMap<T, U>;

        value_type& bind;
        Serializer(value_type& bind) noexcept:
            bind{bind} {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<Map<T, U>>{bind.t2u};
        }

        void deserialize(BIStream& is) const noexcept {
            bind.clear();
            is >> Serializer<Map<T, U>>{bind.t2u};
            for (auto& [k, v]: bind.t2u)
                bind.u2t.emplace(v, k);
        }
    };

    template<>
    struct Serializer<Path> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = Path;
        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind{bind}{}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const std::string>{bind.string()};
        }
        void deserialize(BIStream& is) const noexcept {
            std::string s;
            is >> Serializer<std::string>{s};
            bind = s;
        }
    };

    template<typename T>
    struct Serializer<Queue<T>> final: Serializer<> {
        constexpr static bool functional = Serializer<T>::functional;
        using value_type = Queue<T>;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};
            for (auto [[maybe_unused]]_: std::views::iota(0u, bind.size())) {
                os << Serializer<const T>{bind.front()};
                bind.emplace(std::move(bind.front()));
                bind.pop();
            }
        }

        void deserialize(BIStream& is) const noexcept {
            while (!bind.empty())
                bind.pop();

            USize size;
            is >> Serializer<USize>{size};
            for (auto [[maybe_unused]]_: std::views::iota(0u, size)) {
                T cache;
                is >> Serializer<T>{cache};
                bind.emplace(std::move(cache));
            }
        }
    };

    template<typename T>
    struct Serializer<Stack<T>> final: Serializer<> {
        constexpr static bool functional = Serializer<T>::functional;
        using value_type = Stack<T>;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            auto v = bind;
            os << Serializer<const USize>{v.size()};
            auto o = Stack<T>{};
            while (!v.empty()) {
                o.emplace(std::move(v.top()));
                v.pop();
            }
            
            while (!o.empty()){
                os << Serializer<const T>{o.top()};
                o.pop();
            }
        }

        void deserialize(BIStream& is) const noexcept {
            while (!bind.empty())
                bind.pop();

            USize size;
            is >> Serializer<USize>{size};
            for (auto [[maybe_unused]]_: std::views::iota(0u, size)) {
                T cache;
                is >> Serializer<T>{cache};
                bind.emplace(std::move(cache));
            }
        }
    };

    template<typename ...Ts>
    struct Serializer<std::tuple<Ts&...>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<Ts...>;
        using value_type = std::tuple<Ts& ...>;

        value_type bind;

        Serializer(value_type bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            TypePack<Ts...>::foreach([&]<typename T, std::size_t I>() {
                os << Serializer<const T>{std::get<I>(bind)};
            });
        }
        void deserialize(BIStream& is) const noexcept {
            TypePack<Ts...>::foreach([&]<typename T, std::size_t index>() {
                is >> Serializer<T>{std::get<index>(bind)};
            });
        }
    };



    template<typename K, typename V>
    struct Serializer<std::pair<K, V>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<K, V>;
        using value_type = std::pair<K, V>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const K>{bind.first};
            os << Serializer<const V>{bind.second};
        }
        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<K>{bind.first};
            is >> Serializer<V>{bind.second};
        }
    };



    template<typename ...Ts>
    struct Serializer<std::variant<Ts...>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<Ts...>;
        using value_type = std::variant<Ts ...>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const USize>{bind.index()};

            std::visit([&]<typename T>(const T& v) {
                os << Serializer<const T>{v};
            }, bind);
        }
        void deserialize(BIStream& is) const noexcept {
            USize idx;
            is >> Serializer<USize>{idx};

            std::visit([&]<typename T>(T& v) {
                is >> Serializer<T>{v};
            }, bind = TypePack<Ts...>::construct(idx));
        }
    };

    template<>
    struct Serializer<CharArrayRepr> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = Str;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<Str>{bind};
        }
        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<Str>{bind};
        }
    };

    template<std::size_t N>
    struct Serializer<UByte[N]> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = UByte[N];
        value_type& bind;
        Serializer(value_type& bind) noexcept:
            bind{bind}
        {}
        void serialize(BOStream& os) const noexcept {
            for (const UByte v: bind)
                os.put(v);
        }
        void deserialize(BIStream& is) const noexcept {
            for (UByte& e: bind)
                e = static_cast<UByte>(is.get());
        }
    };

    template<typename T>
    requires std::is_standard_layout_v<T> && std::is_class_v<T> && !std::is_const_v<T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = Serializer<UByte[sizeof(T)]>::functional;
        using value_type = T;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(BOStream& os) const noexcept {
            os << Serializer<const UByte[sizeof(T)]>{reinterpret_cast<const UByte(&)[sizeof(T)]>(bind)};
        }

        void deserialize(BIStream& is) const noexcept {
            is >> Serializer<UByte[sizeof(T)]>{reinterpret_cast<UByte(&)[sizeof(T)]>(bind)};
        }

    };

    static_assert(Serializable<Package>);
    static_assert(Serializable<Patch>);
}