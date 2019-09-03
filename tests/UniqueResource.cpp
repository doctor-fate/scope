#include <cstdint>
#include <exception>
#include <functional>

#include <gtest/gtest.h>

#include <Scope/UniqueResource.h>

using namespace std::string_literals;

namespace {
    template <typename T>
    struct ThrowCopyCallable {
        template <typename U>
        explicit ThrowCopyCallable(U Function) noexcept : ThrowCopyCallable(true, std::invoke(Function)) { }

        explicit ThrowCopyCallable(bool bThrow, T& Value) noexcept : bThrow(bThrow), Value(Value) { }

        ThrowCopyCallable(const ThrowCopyCallable& Other) : bThrow(Other.bThrow), Value(Other.Value) {
            if (bThrow) {
                throw std::logic_error{"oops"};
            }
        }

        ThrowCopyCallable& operator=(const ThrowCopyCallable& Other) {
            bThrow = Other.bThrow;
            if (bThrow) {
                throw std::logic_error{"oops"};
            }
            return *this;
        }

        void operator()(const T& R) const noexcept {
            Value += R;
        }

        bool bThrow;
        T& Value;
    };

    template <typename T>
    struct ThrowCopyResource {
        template <typename U>
        explicit ThrowCopyResource(U Function) noexcept : ThrowCopyResource(true, std::invoke(Function)) { }

        template <typename U>
        explicit ThrowCopyResource(bool bThrow, U&& Value) noexcept : bThrow(bThrow), Value(std::forward<U>(Value)) { }

        ThrowCopyResource(const ThrowCopyResource& Other) : bThrow(Other.bThrow), Value(Other.Value) {
            if (bThrow) {
                throw std::logic_error{"oops"};
            }
        }

        ThrowCopyResource& operator=(const ThrowCopyResource& Other) {
            bThrow = Other.bThrow;
            if (bThrow) {
                throw std::logic_error{"oops"};
            }
            return *this;
        }

        ThrowCopyResource& operator+=(const ThrowCopyResource& Other) noexcept {
            Value += Other.Value;
            return *this;
        }

        bool bThrow;
        T Value;
    };

    template <typename U>
    ThrowCopyResource(bool, U)->ThrowCopyResource<U>;
}

namespace stdx::tests {
    TEST(Scope, UniqueResource_Construction) {
        {
            UniqueResource<void*, std::function<void(void*)>> Resource;
            ASSERT_EQ(Resource.Get(), nullptr);
            ASSERT_FALSE(Resource.GetDeleter());
        }

        {
            int Value = 0;
            {
                UniqueResource Resource(3, [&Value](int R) { Value = R; });
            }
            ASSERT_EQ(Value, 3);
        }

        {
            int Value = 42;
            {
                ThrowCopyCallable Callable(false, Value);
                UniqueResource Resource(3, Callable);
            }
            ASSERT_EQ(Value, 45);
        }

        {
            int Value = 42;
            {
                UniqueResource<int, ThrowCopyCallable<int>> Resource(3, [&Value]() -> int& { return Value; });
            }
            ASSERT_EQ(Value, 45);
        }

        {
            int Value = 42;
            {
                ThrowCopyCallable Callable(true, Value);
                ASSERT_THROW(UniqueResource Resource(10, Callable), std::logic_error);
            }
            ASSERT_EQ(Value, 52);
        }

        {
            int Value = 42;
            {
                using U = UniqueResource<int, std::function<void(int)>>;
                ThrowCopyCallable Callable(true, Value);
                ASSERT_THROW(U Resource(11, Callable), std::logic_error);
            }
            ASSERT_EQ(Value, 53);
        }

        {
            std::string Value = "Hello";
            {
                ThrowCopyCallable Callable(false, Value);
                const auto World = ", world!"s;
                UniqueResource Resource(World, Callable);
            }
            ASSERT_EQ(Value, "Hello, world!");
        }

        {
            std::string Value = "Hello";
            {
                ThrowCopyCallable Callable(true, Value);
                const auto World = ", world!"s;
                ASSERT_THROW(UniqueResource Resource(World, Callable), std::logic_error);
            }
            ASSERT_EQ(Value, "Hello, world!");
        }

        {
            std::string Value = "Hello";
            {
                const auto World = ", world!"s;
                UniqueResource Resource(World, [&Value](const std::string& R) { Value += R; });
            }
            ASSERT_EQ(Value, "Hello, world!");
        }

        {
            int Value = 42;
            {
                ASSERT_THROW(
                    UniqueResource Resource(
                        ThrowCopyResource(true, 42),
                        [&Value](const ThrowCopyResource<int>& R) { Value += R.Value; }),
                    std::logic_error);
            }
            ASSERT_EQ(Value, 84);
        }

        {
            ThrowCopyResource R(false, 42);
            {
                ThrowCopyCallable Callable(true, R);
                ASSERT_THROW(UniqueResource Resource(ThrowCopyResource(true, 42), Callable), std::logic_error);
            }
            ASSERT_EQ(R.Value, 84);
        }

        {
            ThrowCopyResource R(false, 42);
            {
                ThrowCopyCallable Callable(true, R);
                ASSERT_THROW(UniqueResource Resource(ThrowCopyResource(false, 42), Callable), std::logic_error);
            }
            ASSERT_EQ(R.Value, 84);
        }

        {
            ThrowCopyResource R(false, 42);
            {
                ThrowCopyCallable Callable(true, R);
                UniqueResource Resource(std::ref(R), std::ref(Callable));
            }
            ASSERT_EQ(R.Value, 84);
        }

        {
            std::string Value = "Hello";
            {
                std::string R = ", world!";
                UniqueResource<std::string&, std::function<void(std::string&)>> Resource(R, [&Value](std::string& V) {
                    Value += V;
                });
            }
            ASSERT_EQ(Value, "Hello, world!");
        }

        {
            int Value = 10;
            {
                UniqueResource<int, ThrowCopyCallable<int>> R1(42, [&Value]() -> int& { return Value; });
                ASSERT_THROW(UniqueResource R2 = std::move(R1), std::logic_error);
            }
            ASSERT_EQ(Value, 52);
        }

        {
            int Value = 10;
            {
                UniqueResource R1(ThrowCopyResource(false, 13), [&Value](const auto& R) { Value += R.Value; });
                UniqueResource R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 23);
        }
    }

    TEST(Scope, UniqueResource_Assignment) {
        {
            int Value = 10;
            {
                ThrowCopyCallable Callable(false, Value);
                UniqueResource<int, ThrowCopyCallable<int>> R1(42, Callable);
                UniqueResource R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 52);
        }

        {
            int Value = 10;
            {
                UniqueResource R1(42, [&Value](int R) { Value += R; });
                UniqueResource R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 52);
        }

        {
            int Value = 10;
            {
                ThrowCopyCallable Callable(false, Value);
                UniqueResource<int, std::function<void(int)>> R1(42, Callable), R2(34, Callable);
                R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 86);
        }

        {
            int Value = 10;
            {
                ThrowCopyCallable Callable(false, Value);
                UniqueResource<int, std::function<void(int)>> R1(42, Callable), R2(34, Callable);
                R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 86);
        }

        {
            int Value = 10;
            {
                ThrowCopyCallable Callable(false, Value);
                UniqueResource R1(42, std::ref(Callable)), R2(34, std::ref(Callable));
                R2 = std::move(R1);
            }
            ASSERT_EQ(Value, 86);
        }

        {
            int Value = 10;
            {
                const auto Callable = [&Value]() -> int& {
                    return Value;
                };
                UniqueResource<int, ThrowCopyCallable<int>> R1(42, Callable), R2(34, Callable);
                ASSERT_THROW(R2 = std::move(R1), std::logic_error);
            }
            ASSERT_EQ(Value, 86);
        }

        {
            ThrowCopyResource R(false, 10);
            {
                ThrowCopyCallable Callable(false, R);
                const auto GetResource = [&R]() -> int& {
                    return R.Value;
                };
                UniqueResource<ThrowCopyResource<int>, ThrowCopyCallable<ThrowCopyResource<int>>> R1(GetResource, Callable),
                    R2(GetResource, Callable);
                ASSERT_THROW(R2 = std::move(R1), std::logic_error);
            }
            ASSERT_EQ(R.Value, 30);
        }
    }

    TEST(Scope, UniqueResource_Reset) {
        {
            int Value = 0;
            {
                UniqueResource Resource(3, [&Value](int R) { Value += R; });
                Resource.Reset();
            }
            ASSERT_EQ(Value, 3);
        }

        {
            int Value = 0;
            {
                UniqueResource Resource(3, [&Value](int R) { Value += R; });
                Resource.Reset();
                Resource.Reset();
            }
            ASSERT_EQ(Value, 3);
        }

        {
            int Value = 0;
            {
                UniqueResource Resource(3, [&Value](int R) { Value += R; });
                Resource.Release();
                Resource.Reset();
            }
            ASSERT_EQ(Value, 0);
        }

        {
            int Value = 0;
            {
                UniqueResource Resource(3, [&Value](int R) { Value += R; });
                Resource.Reset(45);
            }
            ASSERT_EQ(Value, 48);
        }

        {
            std::string Value;
            {
                UniqueResource Resource("Hello"s, [&Value](const std::string& R) { Value += R; });
                Resource.Reset(", world!!!");
            }
            ASSERT_EQ(Value, "Hello, world!!!");
        }

        {
            int Value = 9;
            {
                UniqueResource Resource(ThrowCopyResource(false, Value), [&Value](const auto& R) { Value += R.Value; });
                ASSERT_THROW(Resource.Reset(ThrowCopyResource(true, Value)), std::logic_error);
            }
            ASSERT_EQ(Value, 27);
        }
    }

    TEST(Scope, UniqueResource_Accessors) {
        {
            const auto Value = new int(5);
            UniqueResource<int*, void(int*)> Resource(Value, [](int* P) { delete P; });
            ASSERT_EQ(Value, Resource.Get());
        }

        {
            const auto Value = new std::string("hello");
            UniqueResource Resource(Value, [](std::string* P) { delete P; });
            ASSERT_EQ(*Resource, "hello");
            ASSERT_EQ(Resource->size(), 5);
        }
    }

    TEST(Scope, UniqueResource_Make) {
        {
            bool bWasCalled = false;
            {
                const auto Resource = MakeUniqueResourceChecked(42, 40.0, [&bWasCalled](int) { bWasCalled = true; });
            }
            ASSERT_TRUE(bWasCalled);
        }

        {
            bool bWasCalled = false;
            {
                const auto Resource = MakeUniqueResourceChecked(42, 42.0, [&bWasCalled](int) { bWasCalled = true; });
            }
            ASSERT_FALSE(bWasCalled);
        }
    }
}