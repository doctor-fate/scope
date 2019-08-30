#include <cstdio>
#include <iostream>

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

void f() {
    std::cout << "f()\n";
}

int main() {
    A a1(6), a2(5), a3(10);
    try {
        stdx::ScopeExit<void()> s1(f);
        stdx::ScopeSuccess s2(std::cref(a2));
        stdx::ScopeFail s3(a3);
        a2.x = 56;
    } catch (...) {
    }

    std::string a = "hello", b = "world";
    auto x = [](std::string& p) {
    };
    stdx::UniqueResource<std::string&, decltype(std::cref(x))> b1(a, std::cref(x)), b2(std::move(b1));
    b1.Reset(b);
    b2 = std::move(b1);
    std::string& i = b2.Get();

    stdx::ScopeSuccess s([&a]() { std::cout << a << '\n'; });

    auto file = stdx::MakeUniqueResourceChecked(
             std::fopen("potentially_nonexistent_file.txt", "r"),
             nullptr,
             [](auto f) { std::fclose(f); }),
         f2(std::move(file));

    return 0;
}