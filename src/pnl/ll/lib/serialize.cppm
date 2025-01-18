//
// Created by FCWY on 24-12-28.
//
// ReSharper disable CppNonExplicitConvertingConstructor
export module pnl.ll.serialize;
import pnl.ll.string;
import pnl.ll.base;
import pnl.ll.runtime;
import pnl.ll.comtime;
import pnl.ll.meta_prog;

import <unordered_map>;
import <iostream>;
import <ranges>;
import <unordered_set>;
import <type_traits>;
import <concepts>;
import <memory>;
import <utility>;
import <variant>;

export namespace pnl::ll::inline serialize::universal_binary{

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
    IStream& operator >> (IStream& is, const Serializer<T>& obj) noexcept {
        obj.deserialize(is);
        return is;
    }
    template<typename T>
    OStream& operator << (OStream& os, const Serializer<T>& obj) noexcept {
        obj.serialize(os);
        return os;
    }


    template<typename T>
    requires Delegatable<T>
        && Serializable<typename T::Delegated>
    struct Serializer<T> final: Serializer<> {
        using D = typename T::Delegated;
        constexpr static bool functional = Serializer<D>::functional;
        using value_type = T;


        value_type& bind;

        Serializer(value_type &bind) noexcept:
            bind(bind) {}

        void serialize(OStream& os) const noexcept {
            os << Serializer<D>{bind};
        }
        void deserialize(IStream& is) const noexcept {
            is >> Serializer<D>{bind};
        }
    };


    template<std::integral T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = T;
        using RInt = typename Repr<T>::Int;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        // enable const or not
        void serialize(OStream& os) const noexcept {
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
        void deserialize(IStream& is) const noexcept {
            if constexpr (std::is_const_v<T>)
                std::unreachable();
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

            for (auto i = start; i != stop; i += step)
                v = (v | repr[i]) << 1;

            bind = reinterpret_cast<const value_type&>(v);
        }
    };



    template<typename E>
    requires std::is_enum_v<E>
    struct Serializer<E> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = E;
        using RInt = std::underlying_type_t<E>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(OStream& os) const noexcept {
            os << Serializer<const RInt>{static_cast<RInt>(bind)};
        }
        void deserialize(IStream& is) const noexcept {
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

        void serialize(OStream& os) const noexcept {
            os << Serializer<std::uint32_t>{bind.content};
        }
        void deserialize(IStream& is) const noexcept {
            is >> Serializer<std::uint32_t>{bind.content};
        }
    };



    template<std::floating_point T>
    struct Serializer<T> final: Serializer<> {
        constexpr static bool functional = true;
        using value_type = T;
        using RInt = typename Repr<T>::Int;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(OStream& os) const noexcept {
            os << Serializer<RInt>{reinterpret_cast<RInt&>(bind)};
        }
        void deserialize(IStream& is) const noexcept {
            is >> Serializer<RInt>{reinterpret_cast<RInt&>(bind)};
        }
    };



    template<typename C>
    requires SequencialContainer<C>
    struct Serializer<C> final: Serializer<> {
        constexpr static bool functional = Serializable<typename C::value_type>;
        using value_type = C;
        using T = typename C::value_type;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}


        void serialize(OStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};

            for (auto& elem: bind)
                os << Serializer<const T>{elem};
        }

        void deserialize(IStream& is) const noexcept {
            USize size;
            is >> Serializer<USize>{size};
            bind.resize(size);


            for (auto& e: bind)
                is >> Serializer<T>{e};
        }
    };



    template<typename C>
    requires AssociativeContainer<C>
    struct Serializer<C> final: Serializer<> {
        constexpr static bool functional = Serializable<typename C::value_type>;
        using value_type = C;
        using T = typename C::value_type;


        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(OStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};

            for (auto& elem: bind)
                os << Serializer<const T>{elem};
        }

        void deserialize(IStream& is) noexcept  {
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

        void serialize(OStream& os) const noexcept {
            os << Serializer<const USize>{bind.size()};
            for (auto& elem: bind) {
                os << Serializer<const T>{elem.first};
                os << Serializer<const U>{elem.second};
            }
        }
        void deserialize(IStream& is) noexcept {
            USize size;
            is >> Serializer<USize>{size};
            for (auto& elem: bind) {
                T key{};
                U value{};
                is >> Serializer<T>{key};
                is >> Serializer<U>{value};
                bind.emplace(std::move(key), std::move(value));
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

        void serialize(OStream& os) const noexcept {
            TypePack<Ts>::foreach([&]<typename T, size_t I>() {
                os << Serializer<const T>{std::get<I>(bind)};
            });
        }
        void deserialize(IStream& is) noexcept {
            TypePack<Ts>::foreach([&]<typename T>(const USize index) {
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

        void serialize(OStream& os) const noexcept {
            os << Serializer<const K>{bind.first};
            os << Serializer<const V>{bind.first};
        }
        void deserialize(IStream& is) noexcept {
            is >> Serializer<K>{bind.first};
            is >> Serializer<V>{bind.first};
        }
    };



    template<typename ...Ts>
    struct Serializer<std::variant<Ts...>> final: Serializer<> {
        constexpr static bool functional = AllSerializable<Ts...>;
        using value_type = std::variant<Ts ...>;

        value_type& bind;

        Serializer(value_type& bind) noexcept:
            bind(bind) {}

        void serialize(OStream& os) const noexcept {
            os << Serializer<const USize>{bind.index()};

            std::visit([&]<typename T>(const T& v) {
                os << Serializer<T>{v};
            }, bind);
        }
        void deserialize(IStream& is) noexcept {
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

        void serialize(OStream& os) const noexcept {
            os << Serializer<Str>{bind};
        }
        void deserialize(IStream& is) const noexcept {
            is >> Serializer<Str>{bind};
        }
    };

    static_assert(Serializable<Package>);

}

export namespace pnl::ll::inline serialize::platform_binary{
    // todo
}