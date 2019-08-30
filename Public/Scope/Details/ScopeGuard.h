#pragma once

#include <utility>

namespace stdx::details {
    template <typename TPolicy>
    class ScopeGuard : private TPolicy {
    public:
        ScopeGuard(ScopeGuard&& Other) noexcept(std::is_nothrow_move_constructible_v<TPolicy>) : TPolicy(std::move(Other)) {
            Other.Release();
        }

        using TPolicy::Release;

    protected:
        using TPolicy::TPolicy;
    };
}