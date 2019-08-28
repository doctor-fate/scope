#pragma once

#include <utility>

namespace stdx::details {
    template <typename TPolicy>
    class ScopeGuard : private TPolicy {
    public:
        ScopeGuard(ScopeGuard&& Other) noexcept(noexcept(TPolicy(std::move(Other)))) : TPolicy(std::move(Other)) {
            Other.Release();
        }

        using TPolicy::Release;

    protected:
        using TPolicy::TPolicy;
    };
}