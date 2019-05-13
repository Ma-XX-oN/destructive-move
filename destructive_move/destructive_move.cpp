// destructive_move.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "destructive_move.hpp"
#include <iostream>

static int i = 0;
struct X {
    int m_i = ++i;
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
    afh::destructively_movable<X> x;
    std::cout << "\nlvalues\n";
    x->test();

    std::cout << "\nlvalues\n";
    static_cast<X                & >(x).test();
    static_cast<X       volatile & >(x).test();
    static_cast<X const          & >(x).test();
    static_cast<X const volatile & >(x).test();
    x.destruct();
    x.construct(afh::emplace<X>());

    std::cout << "\nrvalues\n";
    static_cast<X                &&>(x).test();
    static_cast<X       volatile &&>(x).test();
    static_cast<X const          &&>(x).test();
    static_cast<X const volatile &&>(x).test();

    std::cout << "\nlvalues\n";
    static_cast<afh::destructively_movable<X>                & >(x)->test();
    static_cast<afh::destructively_movable<X>       volatile & >(x)->test();
    static_cast<afh::destructively_movable<X> const          & >(x)->test();
    static_cast<afh::destructively_movable<X> const volatile & >(x)->test();

    std::cout << "\nlvalues\n";
    static_cast<afh::destructively_movable<X>                &&>(x)->test();
    static_cast<afh::destructively_movable<X>       volatile &&>(x)->test();
    static_cast<afh::destructively_movable<X> const          &&>(x)->test();
    static_cast<afh::destructively_movable<X> const volatile &&>(x)->test();

    afh::destructively_movable<X> x1(std::move(x));
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
