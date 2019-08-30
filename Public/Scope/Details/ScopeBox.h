#pragma once

#include <functional>

#include "Traits.h"

namespace stdx::details {
    template <typename T>
    struct BaseScopeBox {
        using Type = TypeIdentity<T>;

        static_assert(std::is_invocable_v<Type&>);

        template <typename U, std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
        explicit BaseScopeBox(std::in_place_t, U&& Data) noexcept(std::is_nothrow_constructible_v<T, U>) :
            Data(std::forward<U>(Data)) { }

        BaseScopeBox(const BaseScopeBox&) = delete;

        BaseScopeBox(BaseScopeBox&& Other) = delete;

        BaseScopeBox& operator=(const BaseScopeBox&) = delete;

        BaseScopeBox& operator=(BaseScopeBox&&) = delete;

        void operator()() noexcept(std::is_nothrow_invocable_v<T&>) {
            std::invoke(Data);
        }

        Type Data;
    };

    template <typename T, bool = std::is_nothrow_move_constructible_v<T>>
    struct ScopeBoxMove : BaseScopeBox<T> {
        using Super = BaseScopeBox<T>;
        using Super::Super;

        ScopeBoxMove(ScopeBoxMove&& Other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            Super(std::in_place, std::as_const(Other.Data)) { }
    };

    template <typename T>
    struct ScopeBoxMove<T, true> : BaseScopeBox<T> {
        using Super = BaseScopeBox<T>;
        using Super::Super;

        ScopeBoxMove(ScopeBoxMove&& Other) noexcept : Super(std::in_place, std::move(Other.Data)) { }
    };

    template <typename T>
    struct ScopeBox : ScopeBoxMove<std::decay_t<T>> {
        using Super = ScopeBoxMove<std::decay_t<T>>;
        using Super::Super;
        using typename Super::Type;

        static_assert(std::is_nothrow_move_constructible_v<Type> || std::is_copy_constructible_v<Type>);
        static_assert(std::is_destructible_v<Type>);
    };

    template <typename T>
    struct ScopeBox<T&> : ScopeBoxMove<std::reference_wrapper<T>> {
        using Super = ScopeBoxMove<std::reference_wrapper<T>>;
        using Super::Super;
        using typename Super::Type;
    };
}