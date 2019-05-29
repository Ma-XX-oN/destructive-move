/// \file
// optional_v2 library
//
//  Copyright Adrian Hawryluk 2019.
//
//  Use, modification and distribution is subject to the
//  MIT License. (See accompanying
//  file LICENSE.txt or copy at
//  https://opensource.org/licenses/MIT)
//
// Project home: https://github.com/Ma-XX-oN/destructive-move
//

// destructive_move.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "destructively_movable.hpp"
#include <iostream>

static int i = 0;

struct Y;
template <typename T, std::enable_if_t<std::is_same_v<afh::remove_cvref_t<T>, Y>, int> = 0>
std::ostream& operator<<(std::ostream& os, T const& obj);

struct Y {
    int m_i = ++i;
    float m_j = (float)++i;
    Y()           { std::cout << "Y default constructed " << *this << "\n"; }
    
    Y(Y const &x) { std::cout << "Y    copy constructed " << *this << "\n"; }
    Y& operator=(Y const& x) {
        std::cout             << "Y       copy assigned " << *this << " = " << x << "\n";
        m_i = x.m_i;
        return *this;
    }

    Y(Y      &&x) { std::cout << "Y    move constructed " << *this << "\n"; }
    Y& operator=(Y     && x) {
        std::cout             << "Y       move assigned " << *this << " = " << x << "\n";
        m_i = x.m_i;
        return *this;
    }
    
    ~Y()          { std::cout << "Y          destructed " << *this << "\n"; }

    void test()                && { std::cout << "               rvalue " << *this << "\n"; }
    void test()       volatile && { std::cout << "      volatile rvalue " << *this << "\n"; }
    void test() const          && { std::cout << "const          rvalue " << *this << "\n"; }
    void test() const volatile && { std::cout << "const volatile rvalue " << *this << "\n"; }

    void test()                &  { std::cout << "               lvalue " << *this << "\n"; }
    void test()       volatile &  { std::cout << "      volatile lvalue " << *this << "\n"; }
    void test() const          &  { std::cout << "const          lvalue " << *this << "\n"; }
    void test() const volatile &  { std::cout << "const volatile lvalue " << *this << "\n"; }
};

struct X;
template <typename T, std::enable_if_t<std::is_same_v<afh::remove_cvref_t<T>, X>, int> = 0>
std::ostream& operator<<(std::ostream& os, T const& obj);

template <typename T, std::enable_if_t<std::is_same_v<afh::remove_cvref_t<T>, Y>, int> /*= 0*/>
std::ostream& operator<<(std::ostream& os, T const& obj)
{
    return os << "(*" << ((void*)&obj) << " = { " << obj.m_i << ", " << obj.m_j << " })";
}

struct X {
    int m_i = ++i;
    float m_j = (float)++i;
    Y m_y;
    X()           { std::cout << "X default constructed " << *this << "\n"; }
    
    X(X const &x) { std::cout << "X    copy constructed " << *this << "\n"; }
    X& operator=(X const& x) {
        std::cout             << "X       copy assigned " << *this << " = " << x << "\n";
        m_i = x.m_i;
        return *this;
    }

    X(X      &&x) { std::cout << "X    move constructed " << *this << "\n"; }
    X& operator=(X     && x) {
        std::cout             << "X       move assigned " << *this << " = " << x << "\n";
        m_i = x.m_i;
        return *this;
    }
    
    ~X()          { std::cout << "X          destructed " << *this << "\n"; }

    void test()                && { std::cout << "               rvalue " << *this << "\n"; }
    void test()       volatile && { std::cout << "      volatile rvalue " << *this << "\n"; }
    void test() const          && { std::cout << "const          rvalue " << *this << "\n"; }
    void test() const volatile && { std::cout << "const volatile rvalue " << *this << "\n"; }

    void test()                &  { std::cout << "               lvalue " << *this << "\n"; }
    void test()       volatile &  { std::cout << "      volatile lvalue " << *this << "\n"; }
    void test() const          &  { std::cout << "const          lvalue " << *this << "\n"; }
    void test() const volatile &  { std::cout << "const volatile lvalue " << *this << "\n"; }
};

template <typename T, std::enable_if_t<std::is_same_v<afh::remove_cvref_t<T>, X>, int> /*= 0*/>
std::ostream& operator<<(std::ostream& os, T const& obj)
{
    return os << "(*" << ((void*)&obj) << " = { " << obj.m_i << ", " << obj.m_j << ", " << obj.m_y << " })";
}

template <>
struct ::afh::destructively_movable_traits<X>
{
    using Tombstone_functions = void;
    //constexpr static auto destructive_move_exempt = afh::destructive_move_exempt(&X::m_i, &X::m_j);
};

template <bool TL, typename T> decltype(auto) make_lvalue_conditionally(T&& x) { return static_cast<std::conditional_t<TL, T&, T&&>>(x); }

// T is what is being passed in.
// TL is if the type T is an lvalue
// U is what is being accepted.
// UL is if the type U is an lvalue
template <bool TL, typename T, std::enable_if_t< ( TL), int> = 0> void take(T&  x) {};
template <bool TL, typename T, std::enable_if_t< (!TL), int> = 0> void take(T&& x) {};
template <bool TL, typename T, bool UL, typename U, typename = void> struct  takes                                                                                                     : std::false_type {};
template <bool TL, typename T, bool UL, typename U>                  struct  takes<TL, T, UL, U, std::void_t<decltype(take<UL, U>(make_lvalue_conditionally<TL>(std::declval<T>())))>> : std::true_type  {};
template <bool TL, typename T, bool UL, typename U> constexpr bool takes_v = takes<TL, T, UL, U>::value;

int main()
{
    static_assert(std::is_void_v<afh::optional_v2_tombstone_functions<X>>, "");
    static_assert(!std::is_trivially_destructible_v<X>, "");

    afh::optional_v2<X> x;
    std::cout << "\n--] lvalues [----------\n";
    x->test();

    std::cout << "\n--] lvalues [----------\n";
    static_cast<X                & >(x).test();
    static_cast<X       volatile & >(x).test();
    static_cast<X const          & >(x).test();
    static_cast<X const volatile & >(x).test();
    x.destruct();
    x.construct(afh::emplace<X>());

    std::cout << "\n--] rvalues [----------\n";
    static_cast<X                &&>(x).test();
    static_cast<X       volatile &&>(x).test();
    static_cast<X const          &&>(x).test();
    static_cast<X const volatile &&>(x).test();

    std::cout << "\n--] lvalues [----------\n";
    static_cast<afh::optional_v2<X>                & >(x)->test();
    static_cast<afh::optional_v2<X>       volatile & >(x)->test();
    static_cast<afh::optional_v2<X> const          & >(x)->test();
    static_cast<afh::optional_v2<X> const volatile & >(x)->test();

    std::cout << "\n--] lvalues [----------\n";
    static_cast<afh::optional_v2<X>                &&>(x)->test();
    static_cast<afh::optional_v2<X>       volatile &&>(x)->test();
    static_cast<afh::optional_v2<X> const          &&>(x)->test();
    static_cast<afh::optional_v2<X> const volatile &&>(x)->test();

    X x3;
    take<true, X>(x3);
    take<false, X>(std::move(x3));
    //take<X, false>(x3);
    //take<X, true>(std::move(x3));

    
    auto f = [](X&&) {};
    static_assert(std::is_void_v<decltype(f(make_lvalue_conditionally<0>(std::declval<X>())))>, "");
    static_assert( takes_v<0, X, 0, X>, "");
    static_assert(!takes_v<1, X, 0, X>, "");
    static_assert(!takes_v<0, X, 1, X>, "");
    static_assert( takes_v<1, X, 1, X>, "");

    static_assert(takes_v<0, X, 0, X>, "");

    afh::optional_v2<X> x1;
    afh::optional_v2<X> x2;
    x1->test();
    x1 = x2;
    x1 = std::move(x2);

    std::cout << "Hello World!\n";
}

//To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
//  clang -std=c++17 -Wall -Xclang -flto-visibility-public-std destructive_move.cpp
