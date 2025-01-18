//
// Created by FCWY on 25-1-1.
//

export module pnl.ll.meta_prog;
import <concepts>;
import <variant>;

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
                }, TypePack<Ts>::construct(idx - 1));
            else
                std::unreachable();
        }

        template<typename Visitor, size_t I=0>
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
