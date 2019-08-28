#pragma once

#include <exception>

#include "ScopeBox.h"

namespace stdx::details {
    template <typename T>
    struct ExitPolicy : ScopeBox<T> {
        using Super = ScopeBox<T>;
        using Super::Super;

        void Release() noexcept {
            bExecuteOnDestruction = false;
        }

        ~ExitPolicy() {
            if (bExecuteOnDestruction) {
                std::invoke(*this);
            }
        }

        bool bExecuteOnDestruction = true;
    };

    template <typename T>
    struct SuccessPolicy : ScopeBox<T> {
        using Super = ScopeBox<T>;
        using Super::Super;

        void Release() noexcept {
            UnchaughtOnCreation = -1;
        }

        ~SuccessPolicy() {
            if (std::uncaught_exceptions() <= UnchaughtOnCreation) {
                std::invoke(*this);
            }
        }

        int UnchaughtOnCreation = std::uncaught_exceptions();
    };

    template <typename T>
    struct FailPolicy : ScopeBox<T> {
        using Super = ScopeBox<T>;
        using Super::Super;

        void Release() noexcept {
            UnchaughtOnCreation = std::numeric_limits<int>::max();
        }

        ~FailPolicy() {
            if (std::uncaught_exceptions() > UnchaughtOnCreation) {
                std::invoke(*this);
            }
        }

        int UnchaughtOnCreation = std::uncaught_exceptions();
    };
}