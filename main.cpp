#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

template <typename T>
struct type_identity {
    using type = T;
};

template <typename T>
using type_identity_t = typename type_identity<T>::type;

template <typename T>
using remove_cvref_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename Base, typename U, bool = std::is_nothrow_constructible_v<Base, std::in_place_t, U>>
struct ScopeConstructible {
    using Type = decltype(std::as_const(std::declval<U&>()));

    static constexpr bool Enable = std::is_constructible_v<Base, std::in_place_t, Type>;
    static constexpr bool NoExcept = std::is_nothrow_constructible_v<Base, std::in_place_t, Type>;
};

template <typename Base, typename U>
struct ScopeConstructible<Base, U, true> {
    using Type = type_identity_t<U>;

    static constexpr bool Enable = true;
    static constexpr bool NoExcept = true;
};

struct FunctionBoxSMF {
    FunctionBoxSMF() = default;

    FunctionBoxSMF(const FunctionBoxSMF&) = delete;

    FunctionBoxSMF(FunctionBoxSMF&& Other) = default;

    FunctionBoxSMF& operator=(const FunctionBoxSMF&) = delete;

    FunctionBoxSMF& operator=(FunctionBoxSMF&&) = delete;
};

template <typename T, bool = std::is_nothrow_move_constructible_v<T>>
struct BaseFunctionBox : FunctionBoxSMF {
    template <typename U>
    explicit BaseFunctionBox(std::in_place_t, U&& Data) noexcept(std::is_nothrow_constructible_v<T, U>) :
        Data(std::forward<U>(Data)) {}

    BaseFunctionBox(BaseFunctionBox&& Other) noexcept(std::is_nothrow_copy_constructible_v<T>) : Data(Other.Data) {}

    void operator()() noexcept(std::is_nothrow_invocable_v<T&>) {
        std::invoke(Data);
    }

    T Data;
};

template <typename T>
struct BaseFunctionBox<T, true> : FunctionBoxSMF {
    template <typename U>
    explicit BaseFunctionBox(std::in_place_t, U&& Data) noexcept(std::is_nothrow_constructible_v<T, U>) :
        Data(std::forward<U>(Data)) {}

    BaseFunctionBox(BaseFunctionBox&& Other) noexcept : Data(std::move(Other.Data)) {}

    void operator()() noexcept(std::is_nothrow_invocable_v<T&>) {
        std::invoke(Data);
    }

    T Data;
};

template <typename T>
struct FunctionBox : public BaseFunctionBox<std::decay_t<T>> {
    using U = std::decay_t<T>;

    static_assert(std::is_invocable_v<U&>);
    static_assert(std::is_nothrow_move_constructible_v<U> || std::is_copy_constructible_v<U>);
    static_assert(std::is_destructible_v<U>);

    using BaseFunctionBox<U>::BaseFunctionBox;
};

template <typename T>
struct FunctionBox<T&> : FunctionBox<std::reference_wrapper<T>> {
    using FunctionBox<std::reference_wrapper<T>>::FunctionBox;
};

template <typename T>
struct ExitPolicy : FunctionBox<T> {
    using Super = FunctionBox<T>;
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
struct SuccessPolicy : FunctionBox<T> {
    using Super = FunctionBox<T>;
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
struct FailPolicy : FunctionBox<T> {
    using Super = FunctionBox<T>;
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

template <typename T>
class ScopeExit final : public ScopeGuard<ExitPolicy<T>> {
    using Super = ScopeGuard<ExitPolicy<T>>;

public:
    template <
        typename U,
        typename Constructible = ScopeConstructible<Super, U>,
        typename F = typename Constructible::Type,
        typename std::enable_if_t<!std::is_same_v<remove_cvref_t<U>, ScopeExit> && Constructible::Enable, int> = 0>
    explicit ScopeExit(U&& Function) noexcept(Constructible::NoExcept) try : Super(std::in_place, std::forward<F>(Function)) {
        static_assert(std::is_invocable_v<U&>);
    } catch (...) {
        std::invoke(Function);
    }
};

template <typename T>
ScopeExit(T)->ScopeExit<T>;

template <typename T>
class ScopeSuccess final : public ScopeGuard<SuccessPolicy<T>> {
    using Super = ScopeGuard<SuccessPolicy<T>>;

public:
    template <
        typename U,
        typename Constructible = ScopeConstructible<Super, U>,
        typename F = typename Constructible::Type,
        typename std::enable_if_t<!std::is_same_v<remove_cvref_t<U>, ScopeSuccess> && Constructible::Enable, int> = 0>
    explicit ScopeSuccess(U&& Function) noexcept(Constructible::NoExcept) : Super(std::in_place, std::forward<F>(Function)) {
        static_assert(std::is_invocable_v<U&>);
    }
};

template <typename T>
ScopeSuccess(T)->ScopeSuccess<T>;

template <typename T>
class ScopeFail final : public ScopeGuard<FailPolicy<T>> {
    using Super = ScopeGuard<FailPolicy<T>>;

public:
    template <
        typename U,
        typename Constructible = ScopeConstructible<Super, U>,
        typename F = typename Constructible::Type,
        typename std::enable_if_t<!std::is_same_v<remove_cvref_t<U>, ScopeFail> && Constructible::Enable, int> = 0>
    explicit ScopeFail(U&& Function) noexcept(Constructible::NoExcept) try : Super(std::in_place, std::forward<F>(Function)) {
        static_assert(std::is_invocable_v<U&>);
    } catch (...) {
        std::invoke(Function);
    }
};

template <typename T>
ScopeFail(T)->ScopeFail<T>;

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
        ScopeExit s1(std::move(a1));
        ScopeSuccess s2(std::cref(a2));
        ScopeFail s3(a3);
        a2.x = 56;
    } catch (...) {
    }
    return 0;
}