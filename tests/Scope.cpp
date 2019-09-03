#include <cstdint>
#include <exception>
#include <functional>

#include <gtest/gtest.h>

#include <Scope/Scope.h>

namespace {
    struct ThrowCopyCallable {
        template <typename T>
        explicit ThrowCopyCallable(T Function) noexcept : ThrowCopyCallable(true, std::invoke(Function)) { }

        explicit ThrowCopyCallable(bool bThrow, std::uint8_t& bWasCalled) noexcept : bThrow(bThrow), bWasCalled(bWasCalled) { }

        ThrowCopyCallable(const ThrowCopyCallable& Other) : bThrow(Other.bThrow), bWasCalled(Other.bWasCalled) {
            if (bThrow) {
                throw std::logic_error{"oops"};
            }
        }

        void operator()() const noexcept {
            ++bWasCalled;
        }

        bool bThrow;
        std::uint8_t& bWasCalled;
    };
}

namespace stdx::tests {
    TEST(Scope, ScopeExit) {
        {
            bool bWasCalled = false;
            {
                ScopeExit Scope([&bWasCalled]() { bWasCalled = true; });
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                ScopeExit Scope([&bWasCalled]() { bWasCalled = true; });
                Scope.Release();
            }
            ASSERT_FALSE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            const auto Callable = [&bWasCalled]() {
                bWasCalled = true;
            };
            { ScopeExit Scope(std::cref(Callable)); }
            ASSERT_TRUE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                ScopeExit<std::function<void()>> Scope([&bWasCalled]() { bWasCalled = true; });
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            std::uint8_t bWasCalled = 0;
            ThrowCopyCallable Callable(false, bWasCalled);
            { ScopeExit Scope(Callable); }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            ThrowCopyCallable Callable(true, bWasCalled);
            { ASSERT_THROW(ScopeExit Scope(Callable), std::logic_error); }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            ThrowCopyCallable Callable(true, bWasCalled);
            { ScopeExit<const ThrowCopyCallable&> Scope(Callable); }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            ThrowCopyCallable Callable(true, bWasCalled);
            { ASSERT_THROW(ScopeExit Scope(std::move(Callable)), std::logic_error); }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            ThrowCopyCallable Callable(false, bWasCalled);
            {
                ScopeExit Scope1(std::move(Callable));
                ScopeExit Scope2 = std::move(Scope1);
            }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            const auto Function = [&bWasCalled]() -> std::uint8_t& {
                return bWasCalled;
            };
            {
                ScopeExit<ThrowCopyCallable> Scope1(Function);
                ASSERT_THROW(ScopeExit Scope2 = std::move(Scope1), std::logic_error);
            }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            const auto Function = [&bWasCalled]() -> std::uint8_t& {
                return bWasCalled;
            };
            {
                ScopeExit<ThrowCopyCallable> Scope1(Function);
                Scope1.Release();
                ASSERT_THROW(ScopeExit Scope2 = std::move(Scope1), std::logic_error);
            }
            ASSERT_EQ(bWasCalled, 0);
        }

        {
            std::uint8_t bWasCalled = 0;
            {
                ScopeExit Scope1([&bWasCalled]() { ++bWasCalled; });
                ScopeExit Scope2 = std::move(Scope1);
            }
            ASSERT_EQ(bWasCalled, 1);
        }
    }

    TEST(Scope, ScopeSuccess) {
        {
            bool bWasCalled = false;
            {
                ScopeSuccess Scope([&bWasCalled]() { bWasCalled = true; });
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                ScopeSuccess Scope([&bWasCalled]() { bWasCalled = true; });
                Scope.Release();
            }
            ASSERT_FALSE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                try {
                    ScopeSuccess Scope([&bWasCalled]() { bWasCalled = true; });
                    throw std::logic_error{"oops"};
                } catch (...) { }
            }
            ASSERT_FALSE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                try {
                    ScopeSuccess Scope([&bWasCalled]() { bWasCalled = true; });
                    throw std::logic_error{"oops"};
                } catch (...) {
                    ScopeSuccess Scope([&bWasCalled]() { bWasCalled = true; });
                }
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            std::uint8_t bWasCalled = false;
            {
                ThrowCopyCallable Callable(true, bWasCalled);
                ASSERT_THROW(ScopeSuccess Scope(std::move(Callable)), std::logic_error);
            }
            ASSERT_EQ(bWasCalled, 0);
        }

        {
            std::uint8_t bWasCalled = 0;
            ASSERT_THROW(
                {
                    const auto Function = [&bWasCalled]() -> std::uint8_t& {
                        return bWasCalled;
                    };
                    ScopeSuccess<ThrowCopyCallable> Scope1(Function);
                    ScopeSuccess Scope2 = std::move(Scope1);
                },
                std::logic_error);
            ASSERT_EQ(bWasCalled, 0);
        }
    }

    TEST(Scope, ScopeFail) {
        {
            bool bWasCalled = false;
            {
                ScopeFail Scope([&bWasCalled]() { bWasCalled = true; });
            }
            ASSERT_FALSE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                ScopeFail Scope([&bWasCalled]() { bWasCalled = true; });
                Scope.Release();
            }
            ASSERT_FALSE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                try {
                    ScopeFail Scope([&bWasCalled]() { bWasCalled = true; });
                    throw std::logic_error{"oops"};
                } catch (...) { }
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                try {
                    ScopeFail Scope([&bWasCalled]() { bWasCalled = true; });
                    throw std::logic_error{"oops"};
                } catch (...) {
                    ScopeFail Scope([&bWasCalled]() { bWasCalled = true; });
                }
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            std::uint8_t bWasCalled = false;
            {
                ThrowCopyCallable Callable(true, bWasCalled);
                ASSERT_THROW(ScopeFail Scope(std::move(Callable)), std::logic_error);
            }
            ASSERT_EQ(bWasCalled, 1);
        }

        {
            std::uint8_t bWasCalled = 0;
            ASSERT_THROW(
                {
                    const auto Function = [&bWasCalled]() -> std::uint8_t& {
                        return bWasCalled;
                    };
                    ScopeFail<ThrowCopyCallable> Scope1(Function);
                    ScopeFail Scope2 = std::move(Scope1);
                },
                std::logic_error);
            ASSERT_EQ(bWasCalled, 1);
        }
    }
}