#include <cstdio>
#include <iostream>

#include <Scope/Details/ResourceBox.h>
#include <Scope/Scope.h>
#include <Scope/UniqueResource.h>

struct A {
    A() = default;

    A(int x) : x(x) {}

    A(const A& a) : x(a.x) {
        if (x == 42) {
            throw x;
        }
    }

    A(A&& Other) noexcept : x(Other.x) {}

    void operator()() const noexcept {
        std::printf("%d\n", x);
    }

    int x = 0;
};

int main() {
    A a1(6), a2(5), a3(10);
    try {
        stdx::ScopeExit<const A> s1(std::move(a1));
        stdx::ScopeSuccess s2(std::cref(a2));
        stdx::ScopeFail s3(a3);
        a2.x = 56;
    } catch (...) {
    }

    std::string a = "hello";
    auto x = [](auto p) {
        std::free(p);
    };
    stdx::UniqueResource b1(malloc(10), std::cref(x));

    stdx::ScopeSuccess s([&a]() { std::cout << a << '\n'; });

    return 0;
}