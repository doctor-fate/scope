#pragma once

#include "Scope/Details/ResourceBox.h"

#if defined(__GNUC__) && !defined(__clang__)
    #define MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D)                                                                          \
        noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R> && std::is_nothrow_constructible_v<std::decay_t<D>, D>)
#else
    #define MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D)
#endif

namespace stdx {
    template <typename R, typename D>
    class UniqueResource {
        using TResource = details::ResourceBox<R>;
        using TDeleter = details::ResourceBox<D>;

        using R1 = typename TResource::Type;
        using D1 = typename TDeleter::Type;

        static_assert(std::is_invocable_v<D1&, decltype(std::declval<TResource&>().Get())>);

        template <typename T>
        static constexpr bool NoExceptMoveConstructible = std::is_nothrow_constructible_v<T, T&&, details::EmptyScope>;

        template <typename T>
        static constexpr bool NoExceptMoveAssignable = std::is_nothrow_move_assignable_v<T>;

        template <typename T>
        static constexpr bool ResourceNoExceptAssignable =
            std::is_nothrow_assignable_v<R1&, T> ||
            std::is_nothrow_assignable_v<R1&, decltype(std::as_const(std::declval<T&>()))>;

    public:
        UniqueResource() = default;

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDeleter, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable, int> = 0>
        UniqueResource(R2&& Resource, D2&& Deleter) noexcept(CR::NoExcept&& CD::NoExcept) :
            Resource(
                std::in_place, std::forward<typename CR::Type>(Resource), ScopeExit([&]() { std::invoke(Deleter, Resource); })),
            Deleter(std::in_place, std::forward<typename CD::Type>(Deleter), ScopeExit([this, &Deleter]() {
                        std::invoke(Deleter, this->Resource.Get());
                    })),
            bExecuteOnReset(true) {
            static_assert(std::is_invocable_v<D2&, R2>);
            static_assert(std::is_invocable_v<D2&, decltype(std::declval<TResource&>().Get())>);
        }

        UniqueResource(UniqueResource&& Other) noexcept(
            NoExceptMoveConstructible<TResource>&& NoExceptMoveConstructible<TDeleter>) :
            Resource(std::move(Other.Resource), GetEmptyScope()),
            Deleter(std::move(Other.Deleter), GetSafeMoveScope(Other)),
            bExecuteOnReset(Other.bExecuteOnReset) {}

        ~UniqueResource() {
            Reset();
        }

        UniqueResource&
        operator=(UniqueResource&& Other) noexcept(NoExceptMoveAssignable<TResource>&& NoExceptMoveAssignable<TDeleter>) {
            Reset();
            if constexpr (std::is_nothrow_move_assignable_v<TResource> && !std::is_nothrow_move_assignable_v<TDeleter>) {
                Deleter = std::move(Other.Deleter);
                Resource = std::move(Other.Resource);
            } else {
                Resource = std::move(Other.Resource);
                Deleter = std::move(Other.Deleter);
            }
            bExecuteOnReset = std::exchange(Other.bExecuteOnReset, false);
        }

        void Release() noexcept {
            bExecuteOnReset = false;
        }

        void Reset() noexcept {
            if (bExecuteOnReset) {
                Release();
                std::invoke(Deleter.Get(), Resource.Get());
            }
        }

        template <
            typename T,
            typename std::enable_if_t<
                std::is_nothrow_assignable_v<R1&, T> || std::is_assignable_v<R1&, decltype(std::as_const(std::declval<T&>()))>,
                int> = 0>
        void Reset(T&& Value) noexcept(ResourceNoExceptAssignable<T>) {
            static_assert(std::is_invocable_v<D1&, T&>);

            Reset();

            if constexpr (std::is_nothrow_assignable_v<R1&, T>) {
                Resource.Get() = std::forward<T>(Value);
            } else {
                ScopeExit Scope([this, &Value]() { std::invoke(Deleter.Get(), Value); });
                Resource.Get() = std::as_const(Value);
                Scope.Release();
            }
            bExecuteOnReset = true;
        }

        decltype(auto) Get() const noexcept {
            return Resource.Get();
        }

        decltype(auto) GetDeleter() const noexcept {
            return Deleter.Get();
        }

        template <typename T = R1, typename std::enable_if_t<std::is_pointer_v<T>, int> = 0>
        auto operator-> () const noexcept {
            return Get();
        }

        template <
            typename T = R1,
            typename std::enable_if_t<std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>, int> = 0>
        decltype(auto) operator*() const noexcept {
            return *Get();
        }

    private:
        template <typename R2, typename D2, typename S>
        friend auto MakeUniqueResourceChecked(R2&& Resource, const S& Sentinel, D2&& Deleter)
            MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R2, D2);

        static auto GetEmptyScope() noexcept {
            return ScopeExit([]() {});
        }

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDeleter, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable, int> = 0>
        UniqueResource(R2&& Resource, D2&& Deleter, bool bExecuteOnReset) noexcept(CR::NoExcept&& CD::NoExcept) :
            Resource(std::in_place, std::forward<typename CR::Type>(Resource), ScopeExit([&]() {
                         if (bExecuteOnReset) {
                             std::invoke(Deleter, Resource);
                         }
                     })),
            Deleter(std::in_place, std::forward<typename CD::Type>(Deleter), ScopeExit([this, &Deleter, bExecuteOnReset]() {
                        if (bExecuteOnReset) {
                            std::invoke(Deleter, this->Resource.Get());
                        }
                    })),
            bExecuteOnReset(bExecuteOnReset) {
            static_assert(std::is_invocable_v<D2&, R2>);
            static_assert(std::is_invocable_v<D2&, decltype(std::declval<TResource&>().Get())>);
        }

        auto GetSafeMoveScope(UniqueResource& Other) noexcept {
            return ScopeExit([this, &Other]() {
                if constexpr (std::is_nothrow_move_constructible_v<R1>) {
                    if (Other.bExecuteOnReset) {
                        std::invoke(Other.Deleter.Get(), Resource.Get());
                        Other.Release();
                    }
                }
            });
        }

        TResource Resource;
        TDeleter Deleter;
        bool bExecuteOnReset = false;
    };

    template <typename R, typename D>
    UniqueResource(R, D)->UniqueResource<R, D>;

    template <typename R, typename D>
    UniqueResource(R, D, bool)->UniqueResource<R, D>;

    template <typename R, typename D, typename S = std::decay_t<R>>
    auto MakeUniqueResourceChecked(R&& Resource, const S& Sentinel, D&& Deleter) MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D) {
        return UniqueResource(std::forward<R>(Resource), std::forward<D>(Deleter), !bool(Resource == Sentinel));
    }
}

#undef MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT