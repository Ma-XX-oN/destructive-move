#pragma once
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
#ifndef AFH_DESTRUCTIVE_MOVE_HPP__
#define AFH_DESTRUCTIVE_MOVE_HPP__

#include "utility.hpp"
#include <new> // for launder
#include <tuple>
#include <cwchar>
#include <cassert>

namespace afh {

template<typename T, typename const_tag, typename...Ts>
struct emplace_params;

template <std::size_t I, typename...Ts>
auto&& get(emplace_params<Ts...>& params);

struct make_lvalue_const_tag {};
struct make_rvalue_copy_lvalue_const_tag {};

//=============================================================================
// template<typename T, typename const_tag, typename...Ts>
// struct emplace_params;
//
//  Used to construct on uninitialised memory by constructing a T object with
//  references to original values and then passing those references to the
//  constructor.
//
//  See template <typename T, typename...Ts> emplace<T>(Ts...) for more
//  details.
template<typename T, typename const_tag, typename...Ts>
struct emplace_params
{
    template <std::size_t I, typename...Us>
    friend auto&& get(emplace_params<Us...>& params);

    emplace_params(Ts&&...args)
        : ref_storage(std::forward<Ts>(args)...)
    {}

    auto* uninitialized_construct(void* storage)
        noexcept(noexcept(T(std::forward<Ts>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(std::forward<Ts>(args)...);
        }, ref_storage);
    }

    // If "this" is const, then pass the parameters as const lvalue references.
    auto* uninitialized_construct(void* storage) const
        noexcept(noexcept(uninitialized_construct(storage, const_tag{})))
    {
        return uninitialized_construct(storage, const_tag{});
    }

private:
    auto* uninitialized_construct(void* storage, make_lvalue_const_tag) const
        noexcept(noexcept(T(afh::rvalue_copy_or_lvalue_const<Ts>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(afh::rvalue_copy_or_lvalue_const<Ts>(args)...);
            }, ref_storage);
    }

    auto* uninitialized_construct(void* storage, make_rvalue_copy_lvalue_const_tag) const
        noexcept(noexcept(T(static_cast<to_const_lvalue<Ts>>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(static_cast<to_const_lvalue<Ts>>(args)...);
            }, ref_storage);
    }

    std::tuple<decltype(std::forward<Ts>(std::declval<Ts>()))...> ref_storage;
};

//-----------------------------------------------------------------------------
// template <typename T, typename const_tag = make_lvalue_const_tag, typename...Ts>
// auto emplace(Ts&& ...args)
//
//  Construct an emplace_params<T, Ts...> object to allow storing the
//  parameters to be passed onto the constructor when calling its
//  uninitialized_construct() member function.  This allows passing all the
//  required information as one parameter to be initiaised elsewhere.  Useful
//  for containers.
//
// Note:
//  If wanting to reuse the emplace_params object, make into a const object
//  and use that.  Otherwise, any rvalues that were there before the first
//  uninitialized_construct() call, will more than likely be moved out from
//  their source.  Calling uninitialized_construct() on a const object will
//  do one of two things, depending on the const_tag.
//
//  make_lvalue_const_tag (default)
//   Causes all the values to be passed as lvalue const references.  As this is
//   the nicest in terms of performance, this is the default.  If, however,
//   your constructor requrires something to be moved in, then you can try the
//   other option.
//
//  make_rvalue_copy_lvalue_const_tag
//   Cause the rvalues to generate a temporary copy and the lvalues will be
//   passed as const.  This may not be the best from a performance POV, but is
//   safer in the event that the rvalues have their guts ripped out and the
//   lvalues are modified after the first iteration.
//
//  If nither of these options sound good, then consider constructing them on
//  the fly.
template <typename T, typename const_tag = make_lvalue_const_tag, typename...Ts>
auto emplace(Ts&& ...args)
{
    return emplace_params<T, const_tag, Ts...>(std::forward<Ts>(args)...);
}

//-----------------------------------------------------------------------------
// template <std::size_t I, typename...Ts>
// auto&& get(emplace_params<Ts...>& params);
//
//  Gets the Ith stored reference in the emplace_params object.  As this
//  function doesn't care what the types are, it just puts them all into a
//  parameter pack.
template <std::size_t I, typename...Ts>
auto&& get(emplace_params<Ts...>& params)
{
    return std::forward<std::tuple_element_t<I, std::tuple<Ts...>>>(std::get<I>(params.ref_storage));
}

//=============================================================================
namespace detail
{
    template <typename T>
    class optional_v2_impl;
}

//-----------------------------------------------------------------------------
// struct tombstone_tag {};
//
//  Tag to tell optional_v2 to create Contained type in a tombstoned
//  state.  Also used in the Tombstone_functions function object to tell Contained
//  object to mark itself in a tombstoned state.
struct tombstone_tag {};

//-----------------------------------------------------------------------------
// template <typename T>
// struct destructively_movable_traits;
//
//  Externally specifies destructivly_movable traits for a type.  These traits
//  can also be added directly to the class.
//
//  NOTE: If they exist in the class and a destructivly_movable traits also
//        exists for the class, the the individual defined trait in the class
//        will take precedence.
//
////
// Traits to specialise on
////
//  Tombstone_functions (optional overloaded function object type, default external
//  tombstone)
//
//   This is an function object, which contains the following two overloads
//   (and this doubles if using in a volatile context):
//
//     bool operator()(Contained const         &) noexcept;
//     bool operator()(Contained const volatile&) noexcept;
//     void operator()(Contained               &, tombstone_tag) noexcept;
//     void operator()(Contained       volatile&, tombstone_tag) noexcept;
//
//   Declaring this means that the object has an internal tombstone marker.
//   This function object, states if that tombstone marker is set (first two
//   overloads) or will explictly set it (last two overloads).
//
//   The getters are used to confirm the state in debug mode, and when
//   determining if there is an object there to delete or assign to a
//   optional_v2.  The setters are only used when the
//   optional_v2 object is initialised with the tombstone_tag object
//   (means that the type is not constructed at definition).
//
//   When the contents of an object is moved, the getter must return false.
//   This state doesn't invalidate the "valid but otherwise indeterminate
//   state" mantra.  This tombstoned state is part of that state.  If,
//   however, the Contained class is never used without the
//   optional_v2 template wrapper, then it is concievable that one
//   could programme the a move such that the only valid operation is to query
//   the tombstone state, and any attempt to destruct the object would result
//   in UB.
//
////
//  is_destructive_move_disabled (optional constexpr static bool, default false)
//
//   Specifies if the object doesn't like being "dropped on the floor" after a
//   move operation.  This really shouldn't be necessary, but some types just
//   don't play that nice when their guts get ripped out.  Some actually even
//   allocate new objects (cou-*MicroSoft*-gh! ;)).  Because of this, this
//   workaround is a necessary evil.
//
////
//  destructive_move_exempt (optional constexpr static auto, default empty)
//
//   This is a means to specify members that are to be excempt from having
//   their internals being "dropped on the floor":
//
//     constexpr static auto destructive_move_exempt
//       = afh::destructive_move_exempt(&X::member_0, ..., &X::member_N);
//
//   This will result in each of the specified member pointers to have their
//   destructor called on it unless there the is_destructive_move_disabled
//   trait has a false value.  In that case, the destructor is still not called.
//
//   NOTE: Best to destruct the required objects in reverse order that they
//         are defined in the class, so they would be destroyed in the same
//         order as if the destructor called it.  Shouldn't be really necessary,
//         but may prevent possible weirdness.
template <typename T>
struct destructively_movable_traits
{
    using Tombstone_functions = void;
    // static constexpr bool is_destructive_move_disabled = true;
    // static constexpr auto destructive_move_exempt = afh::destructive_move_exempt(&X::m_i, &X::m_j);
};

//-----------------------------------------------------------------------------
namespace detail {
    template <typename T>
    void destruct(T& obj) noexcept(noexcept(obj.~T()))
    {
        obj.~T();
    }

    template<typename T>
    struct Destruct_item {
        T& obj;
        template<typename T_>
        void operator()(T_&& member) noexcept(noexcept(destruct(obj.*member))) {
            destruct(obj.*member);
        }
    };
    // deduction guide
    template<typename T>
    Destruct_item(T&) -> Destruct_item<T>;

    template <typename T, typename Take_from>
    struct Destruct_exempt_members {
        void operator()(T& obj) noexcept(noexcept(for_each(Take_from::destructive_move_exempt, Destruct_item{ obj }))) {
            // clang++ is having issues with the noexcept clause for lambdas
            // causing it to crash.  Works fine for a functor.
            //auto fn = [&obj](auto&& object) noexcept(noexcept(destruct(obj.*object))) {
            //    destruct(obj.*object);
            //};
            auto fn = Destruct_item{obj};
            // TTA: Should destructors not be allowed to throw or just wrap all
            //      destructor calls in a try/catch(...)?
            static_assert(noexcept(for_each(Take_from::destructive_move_exempt, fn)), "Destructors shouldn't throw.");
            for_each(Take_from::destructive_move_exempt, fn);
        }

        static constexpr bool is_empty = std::tuple_size_v<decltype(Take_from::destructive_move_exempt)> = 0;
    };

    template <typename T, typename = void>
    struct has_destructive_move_exempt : std::false_type {};

    template <typename T>
    struct has_destructive_move_exempt<T
        , std::void_t<decltype(T::destructive_move_exempt)>
    > : std::true_type {};

    // default - do nothing
    template <typename T, typename = void>
    struct get_destruct_exempt_members {
        void operator()(T& obj) noexcept {
        }

        static constexpr bool is_empty = true;
    };

    // Can exist in destructively_movable_traits<T> or T.  If exists in both,
    // the one in T overrides.
    template <typename T>
    struct get_destruct_exempt_members<T, std::enable_if_t<
        has_destructive_move_exempt<T>::value
    >> : Destruct_exempt_members<T, T>
    {
    };

    template <typename T>
    struct get_destruct_exempt_members<T, std::enable_if_t<
        !has_destructive_move_exempt<T>::value
        && has_destructive_move_exempt<destructively_movable_traits<T>>::value
    >> : Destruct_exempt_members<T, destructively_movable_traits<T>>
    {
    };
}
template <typename T>
using optional_v2_destruct = detail::get_destruct_exempt_members<T>;

//-----------------------------------------------------------------------------
namespace detail {
    template <typename Take_from>
    struct tombstone_functions
    {
        using type = typename Take_from::Tombstone_functions;
    };

    template <typename T, typename = void>
    struct has_tombstone_functions : std::false_type {};

    template <typename T>
    struct has_tombstone_functions<T
        , std::void_t<typename T::Is_tombstone>
    > : std::true_type {};

    // default
    template <typename T, typename = void>
    struct get_tombstone_functions
    {
        using type = void;
    };

    // Can exist in destructively_movable_traits<T> or T.  If exists in both,
    // the one in T overrides.
    template <typename T>
    struct get_tombstone_functions<T, std::enable_if_t<
        has_tombstone_functions<T>::value
    >> : tombstone_functions<T>
    {
    };

    template <typename T>
    struct get_tombstone_functions<T, std::enable_if_t<
        !has_tombstone_functions<T>::value
        && has_tombstone_functions<destructively_movable_traits<T>>::value
    >> : tombstone_functions<destructively_movable_traits<T>>
    {
    };
}
// By default, if there is no is_destructive_move_disabled trait defined in
// either the destructively_movable_traits<type> or the type itself, then
// it wil be set to true.
template <typename T>
using optional_v2_tombstone_functions = typename detail::get_tombstone_functions<T>::type;

//-----------------------------------------------------------------------------
namespace detail {
    template <typename Take_from>
    struct is_destructive_move_disabled {
        static constexpr bool value = Take_from::is_destructive_move_disabled;
    };

    template <typename T, typename = void>
    struct has_is_destructive_move_disabled : std::false_type {};

    template <typename T>
    struct has_is_destructive_move_disabled<T
        , std::void_t<decltype(T::is_destructive_move_disabled)>
    > : std::bool_constant<T::is_destructive_move_disabled> {};

    // default
    template <typename T, typename = void>
    struct is_destructive_move_disabled_impl
    {
        static constexpr bool value = true;
    };

    // Can exist in destructively_movable_traits<T> or T.  If exists in both,
    // the one in T overrides.
    template <typename T>
    struct is_destructive_move_disabled_impl<T, std::enable_if_t<
        has_is_destructive_move_disabled<T>::value
    >> : is_destructive_move_disabled<T>
    {
    };

    template <typename T>
    struct is_destructive_move_disabled_impl<T, std::enable_if_t<
        !has_is_destructive_move_disabled<T>::value
        && has_is_destructive_move_disabled<destructively_movable_traits<T>>::value
    >> : is_destructive_move_disabled<destructively_movable_traits<T>>
    {
    };
}
// By default, if there is no is_destructive_move_disabled trait defined in
// either the destructively_movable_traits<type> or the type itself, then
// it wil be set to true.  E.g. cannot destructively move that object type.
template <typename T>
constexpr bool is_destructive_move_disabled = detail::is_destructive_move_disabled_impl<T>::value;

//-----------------------------------------------------------------------------
template<typename C, typename MT
    , std::enable_if_t<!is_destructive_move_disabled<C>, int> = 0>
constexpr auto destructive_move_exempt(MT C::* mp)
{
    return std::tuple{ mp };
}

template<typename C, typename MT
    , std::enable_if_t< is_destructive_move_disabled<C>, int> = 0>
constexpr auto destructive_move_exempt(MT C::* mp)
{
    return std::tuple{ };
}

template<typename C, typename MT, typename...Ts
    , std::enable_if_t<!is_destructive_move_disabled<C>, int> = 0>
constexpr auto destructive_move_exempt(MT C::* mp, Ts...args)
{
    return std::tuple_cat(std::tuple{ mp }, destructive_move_exempt(args...));
}

template<typename C, typename MT, typename...Ts
    , std::enable_if_t< is_destructive_move_disabled<C>, int> = 0>
constexpr auto destructive_move_exempt(MT C::* mp, Ts...args)
{
    return std::tuple_cat(std::tuple{ }, destructive_move_exempt(args...));
}

//-----------------------------------------------------------------------------
// template <typename Contained>
// class optional_v2;
//
//  The will create a destructively movable container for an object.  This
//  means if this object is moved, the container's contents is also moved. Then
//  the container that was moved from will not call the destructor of its
//  contained object.  This can have performance benifits if the Contained
//  destructor has to call other destructors for other objects that it
//  contains, either iteratively or recersively.
//
//  Turns out that I made a glorified std::optional type.
//
////
// Template Parameters
////
//  Contained (required contained type)
//
//   This is the type to be contained.
//
////
// Constructors
////
//  optional_v2::optional_v2(Contained      && object_to_move_from);
//  optional_v2::optional_v2(Contained const & object_to_copy_from);
//
//   Forwards to optional_v2_impl::optional_v2_impl(...).
////
//  template<typename...Ts>
//  optional_v2_impl::optional_v2_impl(Ts&&...args);
//
//   Forwards to optional_v2_impl::emplace(...).
////
//  optional_v2_impl::optional_v2_impl(tombstone_tag);
//
//   Marks the contained object space or external marker as invalid.
////
//  template <typename...Ts>
//  optional_v2* optional_v2_impl::emplace(Ts&&...args)
//
//   Forwards to optional_v2_impl::emplace(emplace_params<Contained, Ts>(...).
////
//  template <typename T, typename const_tag, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  optional_v2* optional_v2_impl::emplace(emplace_params<T, const_tag, Ts...>&& emplace)
////
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
//  template <typename T, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  optional_v2* optional_v2_impl::emplace(emplace_params<T, Ts...> const& emplace)
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
////
// Destructors
////
//  optional_v2_impl::~optional_v2_impl()
//
//   Calls destructor on Contained only if is_tombstoned() is true.
//
//  void optional_v2_impl::reset()
//
//   Calls destructor on Contained object. is_tombstoned() must be true before
//   calling this function.
//
////
// Assignment
////
//  template <typename T> auto&& optional_v2_impl::operator=(T&& rhs)          & ;
//  template <typename T> auto&& optional_v2_impl::operator=(T&& rhs) volatile & ;
//
//   Assigns lrvalue reference object of type T to the lvalue Contained object.
//
//  template <typename T> auto&& optional_v2_impl::operator=(T&& rhs)          &&;
//  template <typename T> auto&& optional_v2_impl::operator=(T&& rhs) volatile &&;
//
//   Assigns lrvalue reference object of type T to the rvalue Contained object.
//
////
// Validity (is object constructed)
////
//  void optional_v2_impl::is_tombstoned()           const          noexcept;
//  void optional_v2_impl::is_tombstoned()           const volitile noexcept;
//
//   Returns if the Contained object is constructed.
//
//  bool optional_v2_impl::has_been_moved()          noexcept;
//  bool optional_v2_impl::has_been_moved() volatile noexcept;
//
//   If it's known that the object has actually been moved without the
//   container's knowledge, then call has_been_moved().  In debug, it will
//   confirm if the internal state is what it should be.  Further, it will
//   (in debug and non-debug) update the interal state correctly if required.
//
////
// Access object
////
//  auto&& object()                &  noexcept;
//  auto&& object()       volatile &  noexcept;
//  auto&& object() const          &  noexcept;
//  auto&& object() const volatile &  noexcept;
//
//   Access object as lvalue reference
//
//  auto&& object()                && noexcept;
//  auto&& object()       volatile && noexcept;
//  auto&& object() const          && noexcept;
//  auto&& object() const volatile && noexcept;
//
//   Access object as rvalue reference
//
////
// Member of operators
////
//  auto* operator->()                noexcept;
//  auto* operator->()       volatile noexcept;
//  auto* operator->() const          noexcept;
//  auto* operator->() const volatile noexcept;
//
//   Gets the cv qualified pointer to the object address.
//
////
// Address of operators
////
//  auto* operator& ()                noexcept;
//  auto* operator& ()       volatile noexcept;
//  auto* operator& () const          noexcept;
//  auto* operator& () const volatile noexcept;
//
//   Gets the cv qualified pointer to the object address.
//
////
// Conversion operators
////
//  There are 32 conversion operators that convert to the Contained type,
//  and I'm not going to write them all down here, but here is a table
//  that shows all the casts that can be done and if they can be done
//  implicitly, or must be done explicity.  T represents the type type
//  being converted to.
//
//                     | downcast   | is_stronger_or_same  ||           
//    lrvalue transfer | or upcast  | <T, decltype(*this)> || Cast is   
//   ==================+============+======================++===========
//     T&& from &&     | downcast   | true                 ||  implicit 
//     T&  from &      | downcast   | true                 ||  implicit 
//     T&& from &&     | upcast     | true                 ||  explicit  // This will not throw. Need a special
//     T&  from &      | upcast     | true                 ||  explicit  // X_dynamic_cast for that.
//   ------------------+------------+----------------------++-----------
//     T&& from &&     | don't care | false                ||  explicit 
//     T&  from &      | don't care | false                ||  explicit 
//   ------------------+------------+----------------------++-----------
//     T&& from &      | don't care | don't care           ||  explicit 
//     T&  from &&     | don't care | don't care           ||  explicit 
//
//  Although I state upcasting in the table, since this is a container with a
//  specific size, upcasting is only valid if the size of the type casted to is
//  the same.
//
////
// Detail
////
//  This library is in responce to the question in CppCon Grill the Committee
//  (https://youtu.be/cH0nJPbMFAY?t=1263) about having destructive moves added
//  to C++.  I thought that it was an interesting problem, and wondered if
//  a library solution could be made.  So I made this "toy" which got away from
//  me.  It is based on some other code which I will be also releasing soon,
//  but as this is done first, I'm prolly going to use this class as it's base.
//
////
// Caveats
////
//  It is always better to have the tombstone internal to the object.  Although
//  an external tombstone is allowed, two issues can arise:
//
//  1. If the Contained object gets moved without the container knowing about
//     it.  If this happens, then the Contained object will still get its
//     destructor called.  So long as the object maintains the "valid but
//     otherwise indeterminate state", this will result in still defined
//     behaviour, although it will negate the performance improvement.
//
//     This performance loss can be regained by calling has_been_moved(), which
//     will correct the state and ensure that the validity chaecks out
//     correctly.  It can be called even if it has an internal tombstone
//     without any performance penelty (with optimisations on). If called when
//     the contained object has not been moved, will most likely result in
//     resource leaks, so don't do that.
//
//  2. A more serious issue is, if after a move, the Contained object retains
//     any resources.  This can occur if resources are retained or reallocated
//     to the moved object (cou-*MicroSoft*-gh! ;)). In this case, resources
//     will then leak from the moved from object.
//
//     To get around this issue, one can define a destruct_exempted_members
//     static const auto type trait in the destructively_movable_traits
//     specialisation for Contained.  This would have to be implemented for any
//     Contained type that contains types that have this issue, which is
//     definitely not nice.  This is really a workaround.  I think that the
//     standard should be rewritten to say something to the effect that Objects
//     that have been moved, should not hang on to (or create) *ANY* resources.
//     Doing so causes problems here and other places (such as making a move
//     constructor to be not marked as noexcept because of allocating of memory
//     *cough-cough*).
template <typename Contained, typename Enabled = void>
class optional_v2
    : public detail::optional_v2_impl<Contained>
{
    using base = detail::optional_v2_impl<Contained>;
public:
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (optional_v2 const& over optional_v2_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    optional_v2(optional_v2     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    optional_v2(optional_v2 const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    optional_v2&  operator=(optional_v2     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    optional_v2&  operator=(optional_v2 const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    optional_v2&  operator=(optional_v2     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    optional_v2&  operator=(optional_v2 const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    optional_v2&& operator=(optional_v2     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    optional_v2&& operator=(optional_v2 const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    optional_v2&& operator=(optional_v2     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    optional_v2&& operator=(optional_v2 const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }

    void is_tombstoned(bool value)                noexcept { assert(value);        Tombstone_functions(*this, tombstone_tag()); }
    void is_tombstoned(bool value)       volatile noexcept { assert(value);        Tombstone_functions(*this, tombstone_tag()); }
    bool is_tombstoned(          ) const          noexcept {                return Tombstone_functions(*this); }
    bool is_tombstoned(          ) const volatile noexcept {                return Tombstone_functions(*this); }

    using base::base;
};

template <typename Contained>
class optional_v2<Contained
    , std::enable_if_t<
        std::is_trivially_destructible_v<Contained>
        && std::is_void_v<optional_v2_tombstone_functions<Contained>>
    >
>
    : public detail::optional_v2_impl<Contained>
{
    using base = detail::optional_v2_impl<Contained>;
public:
    // This copy/move constructor is needed because on copying/moving,
    // m_isTombstoned will be updated after is was already set by the
    // base copy/move constructor.  This either invalidates the value
    // or results in an extra copy.
    //
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (optional_v2 const& over optional_v2_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    optional_v2(optional_v2     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    optional_v2(optional_v2 const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    optional_v2&  operator=(optional_v2 const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    optional_v2&  operator=(optional_v2     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    optional_v2&  operator=(optional_v2 const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    optional_v2&  operator=(optional_v2     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    optional_v2&& operator=(optional_v2 const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    optional_v2&& operator=(optional_v2     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    optional_v2&& operator=(optional_v2 const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    optional_v2&& operator=(optional_v2     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }

    void is_tombstoned(bool value)                noexcept {               }
    void is_tombstoned(bool value)       volatile noexcept {               }
    bool is_tombstoned(          ) const          noexcept { return false; }
    bool is_tombstoned(          ) const volatile noexcept { return false; }

    using base::base;
};

template <typename Contained>
class optional_v2<Contained
    , std::enable_if_t<
        !std::is_trivially_destructible_v<Contained>
        && std::is_void_v<optional_v2_tombstone_functions<Contained>>
    >
>
    : public detail::optional_v2_impl<Contained>
{
    using base = detail::optional_v2_impl<Contained>;
    bool m_isTombstoned;
public:
    // This copy/move constructor is needed because on copying/moving,
    // m_isTombstoned will be updated after is was already set by the
    // base copy/move constructor.  This either invalidates the value
    // or results in an extra copy.
    //
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (optional_v2 const& over optional_v2_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    optional_v2(optional_v2     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    optional_v2(optional_v2 const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    optional_v2&  operator=(optional_v2 const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    optional_v2&  operator=(optional_v2     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    optional_v2&  operator=(optional_v2 const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    optional_v2&  operator=(optional_v2     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    optional_v2&& operator=(optional_v2 const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    optional_v2&& operator=(optional_v2     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    optional_v2&& operator=(optional_v2 const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    optional_v2&& operator=(optional_v2     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }

    void is_tombstoned(bool value)                noexcept {        m_isTombstoned = value; }
    void is_tombstoned(bool value)       volatile noexcept {        m_isTombstoned = value; }
    bool is_tombstoned(          ) const          noexcept { return m_isTombstoned; }
    bool is_tombstoned(          ) const volatile noexcept { return m_isTombstoned; }

    using base::base;
};

template <typename T>
struct is_optional_v2 : std::false_type {};

template <typename T, typename I>
struct is_optional_v2<optional_v2<T, I>> : std::true_type {};

template <typename T>
using is_optional_v2_t = is_optional_v2<T>;

template <typename T>
constexpr bool is_optional_v2_v = is_optional_v2<T>::value;

namespace detail {

// Helper class for storage
template <typename Contained, typename = void>
class storage
{
protected:
    union {
        Contained value;
    };
public:
     storage() {}
    ~storage() {}
};

template <typename Contained>
class storage<Contained, std::enable_if_t<std::is_empty_v<Contained>>>
{
    // Stores nothing.
};

// Helper class for destructor for non-trivial types.
template <typename Contained, typename = void>
class destruct_class {
public:
    ~destruct_class();
};

template <typename Contained>
class destruct_class<Contained, std::enable_if_t<
        std::is_void_v<optional_v2_tombstone_functions<Contained>> // doesn't have tombstone functions
        && std::is_trivially_destructible_v<Contained>
    >
>
{
    // not having a destructor makes this a trivial type
};

template <typename Contained>
class alignas(Contained) optional_v2_impl
    : storage<Contained>
    , destruct_class<Contained>
{
    // So that the destrutor can call optional_v2_impl::destruct_exempted_members()
    template <typename Contained, typename>
    friend class afh::detail::destruct_class;

    static_assert(afh::is_destructive_move_disabled<Contained>, "Cannot wrap Contained in a optional_v2 template as it is marked disabled");

    using optional_v2  = ::afh::optional_v2<Contained>;
    static constexpr bool has_tombstone = !std::is_void_v<optional_v2_tombstone_functions<Contained>>;
    using Destruct_exempt_members = optional_v2_destruct<Contained>;

    optional_v2                * derived()                noexcept { return static_cast<optional_v2                *>(this); }
    optional_v2       volatile * derived()       volatile noexcept { return static_cast<optional_v2       volatile *>(this); }
    optional_v2 const          * derived() const          noexcept { return static_cast<optional_v2 const          *>(this); }
    optional_v2 const volatile * derived() const volatile noexcept { return static_cast<optional_v2 const volatile *>(this); }

    static constexpr bool is_trivially_destructible_without_tombstone
        = !has_tombstone && std::is_trivially_destructible_v<Contained>;

    // Not constexpr if Contained is empty class
    template <typename U>
    constexpr static auto&& value(U&& this_ref) noexcept
    {
        assert(this_ref.is_trivially_destructible_without_tombstone || !this_ref.is_tombstoned());
        if constexpr (!std::is_empty_v<Contained>)
            return this_ref.storage<Contained>::value;
        else
            return
                fwd_like<U>(
                    *std::launder(
                        reinterpret_cast<copy_cv_t<U, Contained*>>(
                            std::addressof(this_ref)
                        )
                    )
                );
    }

    static constexpr bool has_external_tombstone = std::is_same<optional_v2, ::afh::optional_v2<Contained, void>>::value;

    template <typename T> struct bare_type_impl                           { using type = T; };
    template <typename T> struct bare_type_impl<::afh::optional_v2<T>> { using type = T; };

    // Strips and extract the wrapped type or if not wrapped, just return the type.
    template <typename T> using  bare_t = typename bare_type_impl<strip_t<T>>::type;
    
    template <typename T> using  fwd_to_bare_type_t = fwd_type_t<T, bare_t<T>>;

    // Want to ensure that these are not called by accedent.
    optional_v2_impl           (optional_v2_impl const&) = delete;
    optional_v2_impl& operator=(optional_v2_impl const&) = delete;
    optional_v2_impl           (optional_v2_impl     &&) = delete;
    optional_v2_impl& operator=(optional_v2_impl     &&) = delete;

public:
    using contained = Contained;

    optional_v2_impl(tombstone_tag) noexcept {
        if constexpr (!is_trivially_destructible_without_tombstone) {
            is_tombstoned(true);
        }
        assert(is_trivially_destructible_without_tombstone || is_tombstoned());
    }

    template <typename...Ts>
    optional_v2_impl(Ts&&...args) noexcept(noexcept(
        emplace(std::forward<Ts>(args)...)
    ))
    {
        emplace(std::forward<Ts>(args)...);
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
    }

    // Does emplace construction of Contained, excluding "move/copy
    // construction".
    template <typename...Ts>
    optional_v2* emplace(Ts&&...args)
        noexcept(
            noexcept(
                emplace(::afh::emplace<Contained>(std::forward<Ts>(std::declval<Ts>())...))
            )
        )
    {
        emplace(::afh::emplace<Contained>(std::forward<Ts>(args)...));
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        return static_cast<optional_v2*>(this);
    }

    // Does emplace "move construction".
    //
    // NOTE: Neither move or copy constructors would actually be called because
    //       this class is never passed a optional_v2_impl const& or
    //       optional_v2_impl&& directly anyway, since the derived
    //       class explicitly passes a optional_v2 const& or
    //       optional_v2&& respectively.
    optional_v2* emplace(optional_v2&& to_be_moved)
        noexcept(
            noexcept(
                emplace(::afh::emplace<Contained>(std::move(to_be_moved).value()))
            )
        )
    {
        emplace(::afh::emplace<Contained>(std::move(to_be_moved).value()));
        if constexpr (to_be_moved.has_external_tombstone) {
            to_be_moved.is_tombstoned(true);
        }
        // Operation was a move on a optional_v2 object.
        assert(is_trivially_destructible_without_tombstone || to_be_moved.is_tombstoned());
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        return static_cast<optional_v2*>(this);
    }

    // Does emplace "copy construction".
    //
    // NOTE: Neither move or copy constructors would actually be called because
    //       this class is never passed a optional_v2_impl const& or
    //       optional_v2_impl&& directly anyway, since the derived
    //       class explicitly passes a optional_v2 const& or
    //       optional_v2&& respectively.
    optional_v2* emplace(optional_v2 const& to_be_copied)
        noexcept(
            noexcept(
                emplace(::afh::emplace<Contained>(to_be_copied.value()))
            )
        )
    {
        emplace(::afh::emplace<Contained>(to_be_copied.value()));
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        return static_cast<optional_v2*>(this);
    }

    // Does emplace construction of T
    // If going to accept derived types, then the size of values must be the same.
    //
    // NOTE: If going to accept derived types, then the size of values must be
    //       the same.
    // NOTE: Although not actually using const_tag, specifying it for
    //       completeness.
    template <typename T, typename const_tag, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    optional_v2* emplace(emplace_params<T, const_tag, Ts...>&& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        //AFH___OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (!is_trivially_destructible_without_tombstone && has_external_tombstone) {
            // = (has_tombstone && has_external_tombstone || !trivially_destructable && has_external_tombstone)
            // = (false || !trivially_destructable && has_external_tombstone)
            // = (!trivially_destructable && has_external_tombstone)
            is_tombstoned(false);
        }
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        return static_cast<optional_v2*>(this);
    }

    // Does emplace construction of T.
    //
    // A const emplace_params implies that all params passed to constructor
    // will be const lvalues, regardless of what they actually were.  This
    // allows for safe reuse of an emplace_params.
    //
    // NOTE: If going to accept derived types, then the size of values must be
    //       the same.
    // NOTE: Although not actually using const_tag, specifying it for
    //       completeness.
    template <typename T, typename const_tag, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    optional_v2* emplace(emplace_params<T, const_tag, Ts...> const& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        AFH___OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (!is_trivially_destructible_without_tombstone && has_external_tombstone) {
            is_tombstoned(false);
        }
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        return static_cast<optional_v2*>(this);
    }

private:
    void destruct_exempted_members()
    {
        if (!is_tombstoned()) {
            if constexpr (std::is_empty_v<Contained>)
                Destruct_exempt_members()(*std::launder(reinterpret_cast<Contained*>(this)));
            else
                Destruct_exempt_members()(storage<Contained>::value);
        }
        else {
            storage<Contained>::value.~Contained();
        }
    }

public:
    void reset()
    {
        assert(is_trivially_destructible_without_tombstone || !is_tombstoned());
        if constexpr (!is_trivially_destructible_without_tombstone) {
            destruct_exempted_members();
            if constexpr (has_external_tombstone) {
                is_tombstoned(true);
            }
        }
        assert(is_trivially_destructible_without_tombstone || is_tombstoned());
    }

    static constexpr bool has_nothing_to_destruct_after_move = Destruct_exempt_members::is_empty;

    // TODO: To be consistent, I should have volatile here.  That's 4
    //       overloads. :( Although, I could get away with two, since swapping
    //       a volatile with a non-volatile is prolly not a good idea?
    void swap(optional_v2& other) noexcept(
            std::is_nothrow_move_constructible_v<Contained>
            && std::is_nothrow_swappable_v<Contained>
        )
    {
        using std::swap;
        if (has_value())
            if (other.has_value())
                swap(value(), other.value());
            else
                assign(derived(), std::move(other));
        else if (other.has_value())
            assign(other, std::move(derived()));
    }

    void has_been_moved() volatile noexcept
    {
        assert(is_trivially_destructible_without_tombstone || 
            (   has_external_tombstone && !is_tombstoned()
            || !has_external_tombstone &&  is_tombstoned()));
        if constexpr (!is_trivially_destructible_without_tombstone && has_external_tombstone) {
            is_tombstoned(true);
        }
        assert(is_trivially_destructible_without_tombstone || is_tombstoned());
    }

    void has_been_moved()          noexcept
    {
        assert(is_trivially_destructible_without_tombstone ||
            (   has_external_tombstone && !is_tombstoned()
            || !has_external_tombstone &&  is_tombstoned()));
        if constexpr (!is_trivially_destructible_without_tombstone && has_external_tombstone) {
            is_tombstoned(true);
        }
        assert(is_trivially_destructible_without_tombstone || is_tombstoned());
    }

private:
    using moving_optional_v2 = std::true_type;
    using assigning_copy     = std::false_type;
    // moving optional_v2 rvalue referenced wrapped type.
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs, moving_optional_v2)
        noexcept(
            noexcept(std::forward<T>(lhs).value() = std::forward<U>(rhs).value())
        )
    {
        // Only assign something if there is something to assign, or if it's
        // trivially destructable without a tombstone marker.
        if (rhs.is_trivially_destructible_without_tombstone || !rhs.is_tombstoned()) {
            // = (rhs.has_tombstone && !rhs.is_tombstoned() || !rhs.trivially_destructable && !rhs.is_tombstoned())
            // Assign the underlying rhs value to the underlying lhs value.
            // Don't use value().operator=(...) because even if Contained is
            // an object that has an operator=() member function, the compiler
            // seems to get confused.  However, generally, it's limiting the
            // type as types can be assignable, but not have an operator=(...)
            // function, such as primitive types.
            std::forward<T>(lhs).value() = std::forward<U>(rhs).value();
            assert(lhs.is_trivially_destructible_without_tombstone || !lhs.is_tombstoned());
            if constexpr (!rhs.is_trivially_destructible_without_tombstone && rhs.has_external_tombstone) {
                rhs.is_tombstoned(true);
            }
            assert(rhs.is_trivially_destructible_without_tombstone ||  rhs.is_tombstoned());
        }
        else if (!lhs.is_tombstoned()) {
            lhs.reset();
        }
        return fwd_like<T>(static_cast<optional_v2&>(lhs));
    }

    // copying non-optional_v2 wrapped reference or lvalue reference type.
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs, assigning_copy)
        noexcept(
            noexcept(std::forward<T>(lhs).value() = std::forward<U>(rhs))
            && noexcept(lhs.emplace(std::forward<U>(rhs)))
        )
    {
        if (lhs.is_trivially_destructible_without_tombstone || !lhs.is_tombstoned()) {
            // Assign the underlying lhs value to the rhs value.
            std::forward<T>(lhs).value() = std::forward<U>(rhs);
        }
        else {
            lhs.emplace(std::forward<U>(rhs));
        }
        assert(lhs.is_trivially_destructible_without_tombstone || !is_optional_v2<U>::value || !lhs.is_tombstoned());
        return fwd_like<T>(static_cast<optional_v2&>(lhs));
    }

    // TTA: Allowing assigning any type.  This assumes that
    //      Contained::operator=() isn't overloaded on anything other than
    //      lvalue/rvalue Contained reference or a type that is implicitly
    //      convertable to such a type.  But what if it is overloaded on
    //      more?  May have to take a second look at above assign() functions.
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs)
        noexcept(noexcept(
            assign(std::forward<T>(lhs), std::forward<U>(rhs), is_optional_v2_t<U>{})
        ))
    {
        return assign(std::forward<T>(lhs), std::forward<U>(rhs), is_optional_v2_t<U>{});
    }

public:
    template <typename T> auto&& operator=(T&& rhs)          &  noexcept(noexcept(assign(         (*this), std::forward<T>(rhs)))) { return assign(         (*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile &  noexcept(noexcept(assign(         (*this), std::forward<T>(rhs)))) { return assign(         (*this), std::forward<T>(rhs)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    template <typename T> auto&& operator=(T&& rhs)          && noexcept(noexcept(assign(std::move(*this), std::forward<T>(rhs)))) { return assign(std::move(*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile && noexcept(noexcept(assign(std::move(*this), std::forward<T>(rhs)))) { return assign(std::move(*this), std::forward<T>(rhs)); }

private:
    void is_tombstoned(bool value)                noexcept {        derived()->is_tombstoned(value); }
    void is_tombstoned(bool value)       volatile noexcept {        derived()->is_tombstoned(value); }

public:
    bool is_tombstoned(          ) const          noexcept { return  derived()->is_tombstoned(); }
    bool is_tombstoned(          ) const volatile noexcept { return  derived()->is_tombstoned(); }

    // Mimic std::optional::has_value()
    bool has_value(              ) const          noexcept { return !derived()->is_tombstoned(); }
    bool has_value(              ) const volatile noexcept { return !derived()->is_tombstoned(); }

    // Mimic std::optional::operator bool()
    constexpr explicit operator bool() const noexcept { return has_value(); }

    // Mimic std::optional::value_or
    template<typename U> auto value_or(U&& default_value) const          &  noexcept { return bool(*this) ? value(          *this ) : static_cast<Contained>(std::forward<U>(default_value)); }
    template<typename U> auto value_or(U&& default_value) const volatile &  noexcept { return bool(*this) ? value(          *this ) : static_cast<Contained>(std::forward<U>(default_value)); }

    template<typename U> auto value_or(U&& default_value)                && noexcept { return bool(*this) ? value(std::move(*this)) : static_cast<Contained>(std::forward<U>(default_value)); }
    template<typename U> auto value_or(U&& default_value)       volatile && noexcept { return bool(*this) ? value(std::move(*this)) : static_cast<Contained>(std::forward<U>(default_value)); }

    // Access value as lvalue reference
    auto&& value()                &  noexcept { return value(          *this ); }
    auto&& value()       volatile &  noexcept { return value(          *this ); }
    auto&& value() const          &  noexcept { return value(          *this ); }
    auto&& value() const volatile &  noexcept { return value(          *this ); }

    // Access value as rvalue reference
    auto&& value()                && noexcept { return value(std::move(*this)); }
    auto&& value()       volatile && noexcept { return value(std::move(*this)); }
    auto&& value() const          && noexcept { return value(std::move(*this)); }
    auto&& value() const volatile && noexcept { return value(std::move(*this)); }

    // Member of operators (TTA: should I use std::addressof instead of &?)
    auto* operator->()                noexcept { return &value(); }
    auto* operator->()       volatile noexcept { return &value(); }
    auto* operator->() const          noexcept { return &value(); }
    auto* operator->() const volatile noexcept { return &value(); }

    // Address of operators (TTA: should I use std::addressof instead of &?)
    auto* operator& ()                noexcept { return &value(); }
    auto* operator& ()       volatile noexcept { return &value(); }
    auto* operator& () const          noexcept { return &value(); }
    auto* operator& () const volatile noexcept { return &value(); }

    // Conversion operators
    //
    // NOTE: Only implicit conversion casts to weaker types and base
    //       classes with the same reference type are allowed.  However,
    //       becasue keyword "this" cannot be interrogated external to the
    //       body of the function, each function has to be manually writen.
    //
    //                     | downcast   | is_stronger_or_same  ||           
    //    lrvalue transfer | or upcast  | <T, decltype(*this)> || Cast is   
    //   ==================+============+======================++===========
    //     T&& from &&     | downcast   | true                 ||  implicit 
    //     T&  from &      | downcast   | true                 ||  implicit 
    //     T&& from &&     | upcast     | true                 ||  explicit  // This will not throw. Need a special
    //     T&  from &      | upcast     | true                 ||  explicit  // X_dynamic_cast for that.
    //   ------------------+------------+----------------------++-----------
    //     T&& from &&     | don't care | false                ||  explicit 
    //     T&  from &      | don't care | false                ||  explicit 
    //   ------------------+------------+----------------------++-----------
    //     T&& from &      | don't care | don't care           ||  explicit 
    //     T&  from &&     | don't care | don't care           ||  explicit 
    //
    // There is no exception when downcasting, hence noexcept(IS_DOWNCAST).
    // Although I state upcasting in the table, since this is a container with
    // a specific size, upcasting is only valid if the size of the type casted
    // to is the same.
    //
    // There will be 4 functions (no cv, c, v, and cv) for each line in the
    // chart.  That's 4*8 = 32 functions.  That's a lot of code, with a lot of
    // potential for error.  Because the main body does exactly the same thing
    // (and also happens to be very simple as they just hand off to other
    // functions), using a macro is the safest and clearest way to do this.
    //
    // If what type of "this" could be deduced, with all its qualifiers,
    // then this would colapse to two functions, or a single function if
    // template <typename U> operator U&&() was a universal reference
    // conversion function. 

#define IS_DOWNCAST (std::is_base_of<strip_t<U>, Contained>::value)
#define IS_UPCAST   (std::is_base_of<Contained, strip_t<U>>::value && !std::is_same<Contained, strip_t<U>>::value)
// NOTE: Using ... instead of a named parameter because when
//       enabler(const_volatile) is called with const_volatile being blank (no
//       cv), VC++ complains.
// NOTE: Disabling binding to std::nullptr_t to allow AFH__INTEROGATE_TYPE() macro to work properly.
#define ENABLE_IF_NOT_NULLPTR_T(...) (!std::is_same_v<afh::strip_t<U>, std::nullptr_t>)
#define IS_CV_STRONGER_OR_SAME(...)  (is_stronger_or_same_cv_v<U, int __VA_ARGS__> && ENABLE_IF_NOT_NULLPTR_T())

#define CONVERSION(const_volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        template <typename U, std::enable_if_t<enabler(const_volatile), int> = 0>               \
        explicit operator U convert_to_lrvalue () const_volatile convert_from_lrvalue noexcept  \
        {                                                                                       \
            static_assert(sizeof(U) <= sizeof(Contained), "Cannot upcast to a larger type.");   \
            return fwd_like<U convert_to_lrvalue>(const_cast<Contained&>(value()));            \
        }

#define CONVERSION_ALL_CVS(        convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(              , convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(      volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(const         , convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(const volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit)

    CONVERSION_ALL_CVS(&&, &&,  IS_DOWNCAST && IS_CV_STRONGER_OR_SAME,         );
    CONVERSION_ALL_CVS(& , & ,  IS_DOWNCAST && IS_CV_STRONGER_OR_SAME,         );
    CONVERSION_ALL_CVS(&&, &&,  IS_UPCAST   && IS_CV_STRONGER_OR_SAME, explicit);
    CONVERSION_ALL_CVS(& , & ,  IS_UPCAST   && IS_CV_STRONGER_OR_SAME, explicit);

    CONVERSION_ALL_CVS(&&, &&,                !IS_CV_STRONGER_OR_SAME, explicit);
    CONVERSION_ALL_CVS(& , & ,                !IS_CV_STRONGER_OR_SAME, explicit);

    CONVERSION_ALL_CVS(&&, & ,                ENABLE_IF_NOT_NULLPTR_T, explicit);
    CONVERSION_ALL_CVS(& , &&,                ENABLE_IF_NOT_NULLPTR_T, explicit);

#undef CONVERSION_ALL_CVS
#undef CONVERSION
#undef ENABLE_IF_NOT_NULLPTR_T
#undef IS_CV_STRONGER_OR_SAME
#undef IS_UPCAST
#undef IS_DOWNCAST
};

template <typename Contained, typename X>
destruct_class<Contained, X>::~destruct_class()
{
    static_cast<optional_v2_impl<Contained>*>(this)->destruct_exempted_members();
}

} // namespace detail
} // namespace afh
#endif // #ifndef AFH_DESTRUCTIVE_MOVE_HPP__
