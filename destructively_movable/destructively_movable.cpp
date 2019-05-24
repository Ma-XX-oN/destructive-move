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
    using Is_tombstoned = void;
    //constexpr static auto destructive_move_exempt = afh::destructive_move_exempt(&X::m_i, &X::m_j);
};


int main()
{
    afh::destructively_movable<X> x;
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
    static_cast<afh::destructively_movable<X>                & >(x)->test();
    static_cast<afh::destructively_movable<X>       volatile & >(x)->test();
    static_cast<afh::destructively_movable<X> const          & >(x)->test();
    static_cast<afh::destructively_movable<X> const volatile & >(x)->test();

    std::cout << "\n--] lvalues [----------\n";
    static_cast<afh::destructively_movable<X>                &&>(x)->test();
    static_cast<afh::destructively_movable<X>       volatile &&>(x)->test();
    static_cast<afh::destructively_movable<X> const          &&>(x)->test();
    static_cast<afh::destructively_movable<X> const volatile &&>(x)->test();

    afh::destructively_movable<X> x1;
    afh::destructively_movable<X> x2;
    x1->test();
    x1 = x2;
    x1 = std::move(x2);

    std::cout << "Hello World!\n";
}

//To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
//  clang -std=c++17 -Wall -Xclang -flto-visibility-public-std destructive_move.cpp


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
