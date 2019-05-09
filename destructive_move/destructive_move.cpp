// destructive_move.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <utility>
#include <type_traits>
#include <new>
#include <assert.h>

// Should prolly get rid of array and all pointers?  Should I get rid of pointers?  Dunno...
template <typename T>
using bare_t = std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<T>>>;

template <typename T, typename U> struct fwd_cv_impl                      { using type = U               ; };
template <typename T, typename U> struct fwd_cv_impl<T       volatile, U> { using type = U       volatile; };
template <typename T, typename U> struct fwd_cv_impl<T const         , U> { using type = U const         ; };
template <typename T, typename U> struct fwd_cv_impl<T const volatile, U> { using type = U const volatile; };

template <typename T, typename U>
using fwd_cv_t = typename fwd_cv_impl<std::remove_reference_t<T>, bare_t<U>>::type;

template <typename T, typename U> struct fwd_ref_impl         { using type = U  ; };
template <typename T, typename U> struct fwd_ref_impl<T& , U> { using type = U& ; };
template <typename T, typename U> struct fwd_ref_impl<T&&, U> { using type = U&&; };

template <typename T, typename U>
using fwd_ref_t = typename fwd_ref_impl<T, std::remove_reference_t<U>>::type;

template <typename T, typename U>
using fwd_like_t = fwd_ref_t<T, fwd_cv_t<T, U>>;

static_assert(std::is_same_v<fwd_like_t<int const&&, float&>, float const&&>, "");

template <typename T, typename U>
U&& fwd_like(T&& to_forward)
{
    //static_assert(std::is_same<T, nullptr_t>::value, "");
    return std::forward<fwd_like_t<T, U>>(to_forward);
}

template <typename T>
class destructive_move_container
{
    bool m_isValid = false;
    char m_storage[sizeof(T)];

    template <typename U>
    static T&& object(U&& this_ref) noexcept
    {
        assert(this_ref.m_isValid);
        return fwd_like<U, T>(*reinterpret_cast<fwd_cv_t<U, T>*>(this_ref.m_storage));
    }

public:

    template <typename...Ts>
    destructive_move_container(Ts&&...args)
    {
        construct(std::forward<Ts>(args)...);
    }

    ~destructive_move_container()
    {
        if (m_isValid) {
            object().~T();
        }
    }

    template <typename...Ts>
    void construct(Ts&...args) {
        assert(!m_isValid);
        new (m_storage) T(std::forward<Ts>(args)...);
        m_isValid = true;
    }

    void destruct() {
        m_isValid = false;
        object().~destructive_move_container();
    }

    bool is_valid() const { return m_isValid; }

    // Access object as lvalue reference
    T&  object()                &  noexcept { return object(          *this ); }
    T&  object()       volatile &  noexcept { return object(          *this ); }
    T&  object() const          &  noexcept { return object(          *this ); }
    T&  object() const volatile &  noexcept { return object(          *this ); }

    // Access object as rvalue reference
    T&& object()                && noexcept { return object(std::move(*this)); }
    T&& object()       volatile && noexcept { return object(std::move(*this)); }
    T&& object() const          && noexcept { return object(std::move(*this)); }
    T&& object() const volatile && noexcept { return object(std::move(*this)); }

    // Member of operators
    T               * operator->()                noexcept { return &object(); }
    T       volatile* operator->()       volatile noexcept { return &object(); }
    T const         * operator->() const          noexcept { return &object(); }
    T const volatile* operator->() const volatile noexcept { return &object(); }

    // Address of operators
    T               * operator& ()                noexcept { return &object(); }
    T       volatile* operator& ()       volatile noexcept { return &object(); }
    T const         * operator& () const          noexcept { return &object(); }
    T const volatile* operator& () const volatile noexcept { return &object(); }

    // Conversion operators
             operator T                &&() && noexcept { return                  object() ; }
             operator T       volatile &&() && noexcept { return                  object() ; }
             operator T const          &&() && noexcept { return                  object() ; }
             operator T const volatile &&() && noexcept { return                  object() ; }

             operator T                & () &  noexcept { return                  object() ; }
             operator T       volatile & () &  noexcept { return                  object() ; }
             operator T const          & () &  noexcept { return                  object() ; }
             operator T const volatile & () &  noexcept { return                  object() ; }

    explicit operator T                &&() &  noexcept { return object(std::move(*this)); }
    explicit operator T       volatile &&() &  noexcept { return object(std::move(*this)); }
    explicit operator T const          &&() &  noexcept { return object(std::move(*this)); }
    explicit operator T const volatile &&() &  noexcept { return object(std::move(*this)); }

    explicit operator T                & () && noexcept { return object(); }
    explicit operator T       volatile & () && noexcept { return object(); }
    explicit operator T const          & () && noexcept { return object(); }
    explicit operator T const volatile & () && noexcept { return object(); }
};

struct X {
    X()  { std::cout << "X constructed\n"; }
    ~X() { std::cout << "X destructed\n"; }

    void test()                && { std::cout << "               rvalue\n"; }
    void test()       volatile && { std::cout << "      volatile rvalue\n"; }
    void test() const          && { std::cout << "const          rvalue\n"; }
    void test() const volatile && { std::cout << "const volatile rvalue\n"; }

    void test()                &  { std::cout << "               lvalue\n"; }
    void test()       volatile &  { std::cout << "      volatile lvalue\n"; }
    void test() const          &  { std::cout << "const          lvalue\n"; }
    void test() const volatile &  { std::cout << "const volatile lvalue\n"; }
};
int main()
{
    destructive_move_container<X> x;

    std::cout << "Hello World!\n"; 
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
