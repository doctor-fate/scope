#pragma once

#include "Scope/Details/ResourceBox.h"

namespace stdx {
    template <typename R, typename D>
    class UniqueResource {
        using TResource = details::ResourceBox<R>;
        using TDeleter = details::ResourceBox<D>;

        using R1 = typename TResource::Type;
        using D1 = typename TDeleter::Type;

        static_assert(std::is_invocable_v<D1&, decltype(std::declval<TResource&>().Get())>);

        static constexpr auto NoexceptMoveConstructible =
            std::is_nothrow_constructible_v<TResource, TResource&&, details::EmptyScope> &&
            std::is_nothrow_constructible_v<TDeleter, TDeleter&&, details::EmptyScope>;

        static constexpr auto NoexceptMoveAssignable = std::is_nothrow_move_assignable_v<TResource> &&
                                                       std::is_nothrow_move_assignable_v<TDeleter>;

        template <typename T>
        static constexpr auto ResourceNoexceptAssignable =
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
            Resource(std::in_place, std::forward<typename CR::Type>(Resource), ScopeExit([&Resource, &Deleter]() {
                         std::invoke(Deleter, Resource);
                     })),
            Deleter(std::in_place, std::forward<typename CD::Type>(Deleter), ScopeExit([this, &Deleter]() {
                        std::invoke(Deleter, this->Resource.Get());
                    })),
            bExecuteOnReset(true) {
            static_assert(std::is_invocable_v<D2&, R2>);
            static_assert(std::is_invocable_v<D2&, decltype(std::declval<TResource&>().Get())>);
        }

        UniqueResource(UniqueResource&& Other) noexcept(NoexceptMoveConstructible) :
            Resource(std::move(Other.Resource), EmptyScope()),
            Deleter(std::move(Other.Deleter), MoveScope(Other)),
            bExecuteOnReset(Other.bExecuteOnReset) {}

        ~UniqueResource() {
            Reset();
        }

        UniqueResource& operator=(UniqueResource&& Other) noexcept(NoexceptMoveAssignable) {
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
        void Reset(T&& Value) noexcept(ResourceNoexceptAssignable<T>) {
            static_assert(std::is_invocable_v<D1&, T&>);

            Reset();

            if constexpr (std::is_nothrow_assignable_v<R1&, T>) {
                Resource.Get() = std::forward<T>(Value);
            } else {
                ScopeFail Scope([this, &Value]() { std::invoke(Deleter.Get(), Value); });
                Resource.Get() = std::as_const(Value);
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
        static auto EmptyScope() noexcept {
            return ScopeExit([]() {});
        }

        auto MoveScope(UniqueResource& Other) noexcept {
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
}