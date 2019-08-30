#pragma once

#include "Details/Policy.h"
#include "Details/ScopeGuard.h"
#include "Details/Traits.h"

namespace stdx {
    template <typename T>
    class ScopeExit final : public details::ScopeGuard<details::ExitPolicy<T>> {
        using Super = details::ScopeGuard<details::ExitPolicy<T>>;

    public:
        template <
            typename U,
            typename Constructible = details::ScopeConstructible<Super, U>,
            typename F = typename Constructible::Type,
            typename std::enable_if_t<!std::is_same_v<details::RemoveCVRef<U>, ScopeExit> && Constructible::Enable, int> = 0>
        explicit ScopeExit(U&& Function) noexcept(Constructible::NoExcept) try : Super(std::in_place, std::forward<F>(Function)) {
            static_assert(std::is_invocable_v<U&>);
        } catch (...) { std::invoke(Function); }
    };

    template <typename T>
    ScopeExit(T)->ScopeExit<T>;

    template <typename T>
    class ScopeSuccess final : public details::ScopeGuard<details::SuccessPolicy<T>> {
        using Super = details::ScopeGuard<details::SuccessPolicy<T>>;

    public:
        template <
            typename U,
            typename Constructible = details::ScopeConstructible<Super, U>,
            typename F = typename Constructible::Type,
            typename std::enable_if_t<!std::is_same_v<details::RemoveCVRef<U>, ScopeSuccess> && Constructible::Enable, int> = 0>
        explicit ScopeSuccess(U&& Function) noexcept(Constructible::NoExcept) : Super(std::in_place, std::forward<F>(Function)) {
            static_assert(std::is_invocable_v<U&>);
        }
    };

    template <typename T>
    ScopeSuccess(T)->ScopeSuccess<T>;

    template <typename T>
    class ScopeFail final : public details::ScopeGuard<details::FailPolicy<T>> {
        using Super = details::ScopeGuard<details::FailPolicy<T>>;

    public:
        template <
            typename U,
            typename Constructible = details::ScopeConstructible<Super, U>,
            typename F = typename Constructible::Type,
            typename std::enable_if_t<!std::is_same_v<details::RemoveCVRef<U>, ScopeFail> && Constructible::Enable, int> = 0>
        explicit ScopeFail(U&& Function) noexcept(Constructible::NoExcept) try : Super(std::in_place, std::forward<F>(Function)) {
            static_assert(std::is_invocable_v<U&>);
        } catch (...) { std::invoke(Function); }
    };

    template <typename T>
    ScopeFail(T)->ScopeFail<T>;
}