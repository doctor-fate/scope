#pragma once

#include <tuple>
#include <utility>

#include "ResourceBox.h"
#include "Traits.h"

namespace stdx::details {
    template <typename R, typename D>
    struct BaseUniqueResource {
        using TResource = ResourceBox<R>;
        using TDestruct = ResourceBox<D>;

        using R1 = typename TResource::Type;
        using D1 = typename TDestruct::Type;

        static_assert(std::is_invocable_v<D1&, decltype(std::declval<TResource&>().Get())>);

        template <typename... T1, typename... T2>
        static constexpr bool IsNoExceptConstructible(TypePack<T1...>, TypePack<T2...>) noexcept {
            return std::is_nothrow_constructible_v<TResource, T1...> && std::is_nothrow_constructible_v<TDestruct, T2...>;
        }

        static constexpr bool IsAssignable() noexcept {
            return std::is_move_assignable_v<TResource> && std::is_move_assignable_v<TDestruct>;
        }

        static constexpr bool IsNoExceptAssignable() noexcept {
            return std::is_nothrow_move_assignable_v<TResource> && std::is_nothrow_move_assignable_v<TDestruct>;
        }

        static constexpr bool IsResourceMoveNoExcept() noexcept {
            return std::is_nothrow_move_constructible_v<TResource>;
        }

        static constexpr bool IsDestructMoveNoExcept() noexcept {
            return std::is_nothrow_move_constructible_v<TDestruct>;
        }

        BaseUniqueResource() = default;

        template <typename... T1, typename... T2>
        BaseUniqueResource(std::tuple<T1...>&& Resource, std::tuple<T2...>&& Destruct, bool bExecuteOnReset) noexcept(
            IsNoExceptConstructible(details::TypePack<T1...>{}, details::TypePack<T2...>{})) :
            Data(std::piecewise_construct, std::move(Resource), std::move(Destruct)),
            bExecuteOnReset(bExecuteOnReset) { }

        BaseUniqueResource(const BaseUniqueResource&) = delete;

        BaseUniqueResource(BaseUniqueResource&&) = delete;

        ~BaseUniqueResource() {
            Reset();
        }

        BaseUniqueResource& operator=(const BaseUniqueResource&) = delete;

        BaseUniqueResource& operator=(BaseUniqueResource&& Other) = delete;

        void Release() noexcept {
            bExecuteOnReset = false;
        }

        void Reset() noexcept {
            if (bExecuteOnReset) {
                Release();
                std::invoke(Destruct().Get(), Resource().Get());
            }
        }

        template <
            typename T,
            typename std::enable_if_t<
                std::is_nothrow_assignable_v<R1&, T> || std::is_assignable_v<R1&, decltype(std::as_const(std::declval<T&>()))>,
                int> = 0>
        void Reset(T&& Value) noexcept(std::is_nothrow_assignable_v<R1&, decltype(std::as_const(std::declval<T&>()))>) {
            static_assert(std::is_invocable_v<D1&, T&>);

            Reset();

            if constexpr (std::is_nothrow_assignable_v<R1&, T>) {
                Resource() = std::forward<T>(Value);
            } else {
                ScopeExit Scope([this, &Value]() { std::invoke(Destruct().Get(), Value); });
                Resource() = std::as_const(Value);
                Scope.Release();
            }

            bExecuteOnReset = true;
        }

        TResource& Resource() noexcept {
            return Data.first;
        }

        const TResource& Resource() const noexcept {
            return Data.first;
        }

        TDestruct& Destruct() noexcept {
            return Data.second;
        }

        const TDestruct& Destruct() const noexcept {
            return Data.second;
        }

        std::pair<TResource, TDestruct> Data;
        bool bExecuteOnReset = false;
    };

    template <typename R, typename D, bool = BaseUniqueResource<R, D>::IsDestructMoveNoExcept()>
    struct UniqueResourceMove : BaseUniqueResource<R, D> {
        using Super = BaseUniqueResource<R, D>;
        using Super::Super;

        UniqueResourceMove() = default;

        UniqueResourceMove(UniqueResourceMove&& Other) :
            Super(
                std::forward_as_tuple(std::move(Other.Resource())),
                std::forward_as_tuple(std::move(Other.Destruct()), GetSafeScope(Other)),
                false) {
            Super::bExecuteOnReset = std::exchange(Other.bExecuteOnReset, false);
        }

        UniqueResourceMove& operator=(UniqueResourceMove&&) = default;

        auto GetSafeScope(UniqueResourceMove& Other) noexcept {
            return ScopeExit([this, &Other]() {
                if constexpr (std::is_nothrow_move_constructible_v<R>) {
                    if (Other.bExecuteOnReset) {
                        std::invoke(Other.Destruct().Get(), Super::Resource().Get());
                        Other.Release();
                    }
                }
            });
        }
    };

    template <typename R, typename D>
    struct UniqueResourceMove<R, D, true> : BaseUniqueResource<R, D> {
        using Super = BaseUniqueResource<R, D>;
        using Super::Super;

        UniqueResourceMove() = default;

        UniqueResourceMove(UniqueResourceMove&& Other) noexcept(Super::IsResourceMoveNoExcept()) :
            Super(
                std::forward_as_tuple(std::move(Other.Resource())),
                std::forward_as_tuple(std::move(Other.Destruct())),
                std::exchange(Other.bExecuteOnReset, false)) { }

        UniqueResourceMove& operator=(UniqueResourceMove&&) = default;
    };

    template <typename R, typename D, bool = BaseUniqueResource<R, D>::IsAssignable()>
    struct UniqueResourceMoveAssign : UniqueResourceMove<R, D> {
        using Super = UniqueResourceMove<R, D>;
        using Super::Super;
    };

    template <typename R, typename D>
    struct UniqueResourceMoveAssign<R, D, true> : UniqueResourceMove<R, D> {
        using Super = UniqueResourceMove<R, D>;
        using Super::Super;

        UniqueResourceMoveAssign() = default;

        UniqueResourceMoveAssign(UniqueResourceMoveAssign&&) = default;

        UniqueResourceMoveAssign& operator=(UniqueResourceMoveAssign&& Other) noexcept(Super::IsNoExceptAssignable()) {
            constexpr auto IsResourceNoExceptAssignable = std::is_nothrow_move_assignable_v<typename Super::TResource>;
            constexpr auto IsDestructNoExceptAssignable = std::is_nothrow_move_assignable_v<typename Super::TDestruct>;

            Super::Reset();

            if constexpr (IsResourceNoExceptAssignable && !IsDestructNoExceptAssignable) {
                Super::Destruct() = std::move(Other.Destruct());
                Super::Resource() = std::move(Other.Resource());
            } else {
                Super::Resource() = std::move(Other.Resource());
                Super::Destruct() = std::move(Other.Destruct());
            }

            Super::bExecuteOnReset = std::exchange(Other.bExecuteOnReset, false);

            return *this;
        }
    };
}