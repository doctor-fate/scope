#pragma once

#include <Scope/Scope.h>

namespace stdx::details {
    template <typename T>
    struct BaseResourceBox {
        using Type = TypeIdentity<T>;

        template <typename U = T, typename std::enable_if_t<std::is_default_constructible_v<U>, int> = 0>
        BaseResourceBox() noexcept(std::is_nothrow_default_constructible_v<T>) : Data() {}

        template <typename U, typename std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
        explicit BaseResourceBox(std::in_place_t, U&& Data) noexcept(std::is_nothrow_constructible_v<T, U>) :
            Data(std::forward<U>(Data)) {}

        template <typename U, typename F, typename std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
        explicit BaseResourceBox(std::in_place_t, U&& Data, ScopeExit<F>&& Scope) noexcept(
            std::is_nothrow_constructible_v<T, U>) :
            Data(std::forward<U>(Data)) {
            Scope.Release();
        }

        BaseResourceBox(const BaseResourceBox&) = delete;

        BaseResourceBox(BaseResourceBox&& Other) = delete;

        BaseResourceBox& operator=(const BaseResourceBox&) = delete;

        BaseResourceBox& operator=(BaseResourceBox&&) = delete;

        Type Data;
    };

    template <typename T, bool = std::is_nothrow_move_constructible_v<T>>
    struct ResourceBoxMove : BaseResourceBox<T> {
        using Super = BaseResourceBox<T>;
        using Super::Super;

        ResourceBoxMove() = default;

        ResourceBoxMove(ResourceBoxMove&& Other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            Super(std::in_place, std::as_const(Other.Data)) {}

        template <typename F>
        ResourceBoxMove(ResourceBoxMove&& Other, ScopeExit<F>&& Scope) noexcept(std::is_nothrow_copy_constructible_v<T>) :
            Super(std::in_place, std::as_const(Other.Data), std::move(Scope)) {}
    };

    template <typename T>
    struct ResourceBoxMove<T, true> : BaseResourceBox<T> {
        using Super = BaseResourceBox<T>;
        using Super::Super;

        ResourceBoxMove() = default;

        ResourceBoxMove(ResourceBoxMove&& Other) noexcept : Super(std::in_place, std::move(Other.Data)) {}
    };

    template <typename T, bool = std::is_nothrow_move_assignable_v<T>>
    struct ResourceBoxMoveAssign : ResourceBoxMove<T> {
        using Super = ResourceBoxMove<T>;
        using Super::Super;

        ResourceBoxMoveAssign() = default;

        ResourceBoxMoveAssign(ResourceBoxMoveAssign&&) = default;

        ResourceBoxMoveAssign& operator=(ResourceBoxMoveAssign&& Other) noexcept(std::is_nothrow_copy_assignable_v<T>) {
            Super::Data = std::as_const(Other.Data);
            return *this;
        }
    };

    template <typename T>
    struct ResourceBoxMoveAssign<T, true> : ResourceBoxMove<T> {
        using Super = ResourceBoxMove<T>;
        using Super::Super;

        ResourceBoxMoveAssign() = default;

        ResourceBoxMoveAssign(ResourceBoxMoveAssign&&) = default;

        ResourceBoxMoveAssign& operator=(ResourceBoxMoveAssign&& Other) noexcept {
            Super::Data = std::move(Other.Data);
            return *this;
        }
    };

    template <typename T>
    using SelectBaseMoveAssign = std::conditional_t<
        std::is_nothrow_move_assignable_v<T> || std::is_copy_assignable_v<T>,
        ResourceBoxMoveAssign<T>,
        ResourceBoxMove<T>>;

    template <typename T>
    struct ResourceBox : SelectBaseMoveAssign<std::decay_t<T>> {
        using Super = SelectBaseMoveAssign<std::decay_t<T>>;
        using Super::Super;
        using typename Super::Type;

        static_assert(std::is_nothrow_move_constructible_v<Type> || std::is_copy_constructible_v<Type>);
        static_assert(std::is_destructible_v<Type>);

        decltype(auto) Get() const noexcept {
            return (Super::Data);
        }

        decltype(auto) Get() noexcept {
            return (Super::Data);
        }
    };

    template <typename T>
    struct ResourceBox<T&> : ResourceBoxMoveAssign<std::reference_wrapper<T>> {
        using Super = ResourceBoxMoveAssign<std::reference_wrapper<T>>;
        using Super::Super;
        using typename Super::Type;

        decltype(auto) Get() const noexcept {
            return Super::Data.get();
        }
    };
}