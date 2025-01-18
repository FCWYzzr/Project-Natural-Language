//
// Created by FCWY on 24-12-28.
//

export module pnl.ll.collections;
import pnl.ll.base;
import <stack>;
import <queue>;
import <functional>;
import <ranges>;
import <variant>;

export namespace pnl::ll::inline collections{
    template<typename T>
    using Stack = std::stack<T, std::pmr::deque<T>>;

    template<typename T>
    using Queue = std::queue<T, std::pmr::deque<T>>;

    template<typename T>
    using Deque = std::pmr::deque<T>;

    namespace pmr{
        using ObjRepr = UByte*;

        using Maker = std::function<void(ObjRepr dst)>;
        using Destructor = std::function<void(ObjRepr obj)>;

        struct PNL_LIB_PREFIX Stack {
            List<USize>         obj_offset;
            List<UByte>         dense_pool;


            Stack(MManager*) noexcept;

            Stack(const Stack& other) noexcept;

            Stack(Stack&& other) noexcept;

            ~Stack() noexcept = default;

            template<typename T, typename... Args>
            requires Trivial<T>
                && std::constructible_from<T, Args...>
            void emplace_top(std::in_place_type_t<T>, Args&&... args) noexcept {
                placeholder_push(sizeof(T));
                const auto offset = obj_offset.back();

                new (&dense_pool[offset]) T{std::forward<Args>(args)...};
            }


            void placeholder_push(USize size) noexcept;


            void waste_top() noexcept;


            void waste_since(USize milestone) noexcept;

            [[nodiscard]]
            USize milestone() const noexcept {
                return obj_offset.size();
            }

            template<typename T>
            T& ref(const USize idx) noexcept {
                const auto offset = obj_offset[idx];
                return reinterpret_cast<T&>(dense_pool[offset]);
            }

            template<typename T>
            T& ref_top() noexcept {
                return ref<T>(obj_offset.size()-1);
            }


            UByte* operator [] (USize id) noexcept;



            void concat(const Stack& other) noexcept;


            [[nodiscard]]
            auto iterate_erased() noexcept {
                using namespace std::views;
                return obj_offset
                       | transform([&pool=dense_pool](auto idx) -> auto&{
                           return pool[idx];
                       });
            }

            [[nodiscard]]
            auto iterate_erased() const noexcept {
                using namespace std::views;
                return obj_offset
                       | transform([&pool=dense_pool](auto idx) -> auto&{
                           return pool[idx];
                       });
            }


            void reserve(USize target_size) noexcept;

            void clear() noexcept {

            }
        };
    }


    template<typename T, typename U>
        requires !std::same_as<T, U>
        struct BiMap {
            Map<T, U> t2u;
            Map<U, T> u2t;

            template<typename ...Args>
            explicit BiMap(Args... args) noexcept :
                t2u(std::forward<Args>(args)...),
                u2t(std::forward<Args>(args)...)
            {}

            void emplace(T t, U u) noexcept {
                erase(t);
                erase(u);
                t2u.emplace(t, u);
                u2t.emplace(std::move(u), std::move(t));
            }

            template<typename I>
            void erase(const I& i) {
                if constexpr (std::same_as<I, T>){
                    if (t2u.contains(i)) {
                        u2t.erase(t2u.at(i));
                        t2u.erase(i);
                    }
                }
                else
                    if (u2t.contains(i)) {
                        t2u.erase(u2t.at(i));
                        u2t.erase(i);
                    }
            }

            template<typename I>
            std::conditional_t<
                std::same_as<std::remove_cvref_t<I>, T>,
                const U&,
                std::conditional_t<
                    std::same_as<std::remove_cvref_t<I>, U>,
                    const T&,
                    void
                >
            >
                at(const I& i) const {
                if constexpr (std::same_as<I, T>)
                    return t2u.at(i);
                else
                    return u2t.at(i);
            }

            template<typename I>
            std::conditional_t<std::same_as<I, T>, U, T>
                drain(const I& i) const {
                if constexpr (std::same_as<I, T>) {
                    auto cache = std::move(t2u.at(i));
                    erase(i);
                    return cache;
                }
                else {
                    auto cache = std::move(u2t.at(i));
                    erase(i);
                    return cache;
                }
            }


            template<typename I>
            bool contains(const I& i) const {
                if constexpr (std::same_as<I, T>)
                    return t2u.contains(i);
                else
                    return u2t.contains(i);
            }

            BiMap(BiMap &&other) noexcept
                : t2u(std::move(other.t2u)),
                  u2t(std::move(other.u2t)) {
            }

            BiMap & operator=(BiMap &&other) noexcept {
                if (this == &other)
                    return *this;
                t2u = std::move(other.t2u);
                u2t = std::move(other.u2t);
                return *this;
            }

            void clear() noexcept {
                t2u.clear();
                u2t.clear();
            }

            ~BiMap() noexcept {
                clear();
            }
        };
}
