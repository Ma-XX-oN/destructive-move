#pragma once
#ifndef AFH_DESTRUCTIVE_MOVE_HPP__
#define AFH_DESTRUCTIVE_MOVE_HPP__

#include "utility.hpp"
#include <new> // for launder
#include <tuple>
#include <assert.h>
namespace afh {

template<typename T, typename const_tag, typename...Ts>
struct emplace_params;

template <size_t I, typename...Ts>
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
    template <size_t I, typename...Us>
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
// template <size_t I, typename...Ts>
// auto&& get(emplace_params<Ts...>& params);
//
//  Gets the Ith stored reference in the emplace_params object.  As this
//  function doesn't care what the types are, it just puts them all into a
//  parameter pack.
template <size_t I, typename...Ts>
auto&& get(emplace_params<Ts...>& params)
{
    return std::get<I>(params.ref_storage);
}

//=============================================================================
namespace detail
{
    template <typename T>
    class destructively_movable_impl;
}

//-----------------------------------------------------------------------------
// struct tombstone_tag {};
//
//  Tag to tell destructively_movable to create Contained type in a tombstoned
//  state.  Also used in the Is_valid function object to tell Contained object
//  to mark itself in a tombstoned state.
struct tombstone_tag {};

//-----------------------------------------------------------------------------
// template <typename T>
// struct destructively_movable_traits;
//
//  Specifies a default function overload object to mark or get valid state of
//  type T.  If set to tombstone_tag, then this explictly states that the type
//  can not be used with a destructively_movable object.
//
////
// Traits to specialise on
////
//  Is_valid (required overloaded function object type)
//
//   This is an function object, which contains the following two overloads
//   (and this doubles if using in a volatile context):
//
//     bool operator()(Contained const         &) noexcept;
//     bool operator()(Contained const volatile&) noexcept;
//     void operator()(Contained               &, tombstone_tag) noexcept;
//     void operator()(Contained       volatile&, tombstone_tag) noexcept;
//
//   The first two will query the Contained object if it is valid. The second
//   two explicitly tell the object to put itself into an invalid state. The
//   second is only called if the destructively_movable object was constructed
//   using the tombstone_tag, or was destructed using the destruct()
//   operation.  A moved object should cause that object to be implictly marked
//   as invalid.
//
//   It is required that the object, when its contents have been moved to
//   another object, if queried if it's valid, that it will respond false.
//   This state doesn't invalidate the "valid but otherwise indeterminate
//   state" mantra.  This tombstoned state is part of that state.  If,
//   however, the class is never used without the destructively_movable
//   template wrapper, then it is concievable that it could programmed such
//   that it might be put in a truely invalid state, where if destructed
//   without the wrapper, the behaviour would be undefined.
//
////
//  Destruct (optional overloaded function object type)
//
//   This is a function object, which contains the following function:
//
//     void operator()(Contained         &) noexcept
//
//   This really shouldn't be necessary, but some types just don't play that
//   nice when their guts get ripped out.  Some actually even allocate new
//   objects (cou-*MicroSoft*-gh! ;)).  Because of this, this workaround is a
//   necessary evil, and is called when the moved object is supposed to be
//   destructed.  This allows explicit destruction of those sub-objects in
//   Contained that don't play nice.
//
//   NOTE: Best to destruct the required objects in reverse order that they
//         execute in the same order like a destructor does.  Not really
//         necessary, but may prevent possible weirdness.
//
//   If Destruct is not provided, then it will be replaced with empty_destruct.

template <typename T>
struct destructively_movable_traits
{
    using Is_valid = void;
};

//-----------------------------------------------------------------------------
// Empty destructor function object.
template <typename T>
struct empty_destruct
{
    void operator()(T*) noexcept {}
};

//-----------------------------------------------------------------------------
namespace detail {
    template <typename T, typename = void>
    struct destructively_movable_destruct_impl {
        using type = empty_destruct<T>;
    };

    template <typename T>
    struct destructively_movable_destruct_impl<T, typename destructively_movable_traits<T>::Destruct> {
        using type = typename destructively_movable_traits<T>::Destruct;
    };
}
//-----------------------------------------------------------------------------
template <typename T>
using destructively_movable_destruct = typename detail::destructively_movable_destruct_impl<T>::type;

//-----------------------------------------------------------------------------
template <typename T>
using destructively_movable_is_valid = typename destructively_movable_traits<T>::Is_valid;

//-----------------------------------------------------------------------------
// template <typename Contained>
// class destructively_movable;
//
//  The will create a destructively movable container for an object.  This
//  means if this object is moved, the container's contents is also moved. Then
//  the container that was moved from will not call the destructor of its
//  contained object.  This can have performance benifits if the Contained
//  destructor has to call other destructors for other objects that it
//  contains, either iteratively or recersively.
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
//  destructively_movable::destructively_movable(Contained      && object_to_move_from);
//  destructively_movable::destructively_movable(Contained const & object_to_copy_from);
//
//   Forwards to destructively_movable_impl::destructively_movable_impl(...).
////
//  template<typename...Ts>
//  destructively_movable_impl::destructively_movable_impl(Ts&&...args);
//
//   Forwards to destructively_movable_impl::construct(...).
////
//  destructively_movable_impl::destructively_movable_impl(tombstone_tag);
//
//   Marks the contained object space or external marker as invalid.
////
//  template <typename...Ts>
//  Derived* destructively_movable_impl::construct(Ts&&...args)
//
//   Forwards to destructively_movable_impl::construct(emplace_params<Contained, Ts>(...).
////
//  template <typename T, typename const_tag, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  Derived* destructively_movable_impl::construct(emplace_params<T, const_tag, Ts...>&& emplace)
////
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
//  template <typename T, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  Derived* destructively_movable_impl::construct(emplace_params<T, Ts...> const& emplace)
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
////
// Destructors
////
//  destructively_movable_impl::~destructively_movable_impl()
//
//   Calls destructor on Contained only if is_valid() is true.
//
//  void destructively_movable_impl::destruct()
//
//   Calls destructor on Contained object. is_valid() must be true before
//   calling this function.
//
////
// Assignment
////
//  template <typename T> auto&& destructively_movable_impl::operator=(T&& rhs)          & ;
//  template <typename T> auto&& destructively_movable_impl::operator=(T&& rhs) volatile & ;
//
//   Assigns lrvalue reference object of type T to the lvalue Contained object.
//
//  template <typename T> auto&& destructively_movable_impl::operator=(T&& rhs)          &&;
//  template <typename T> auto&& destructively_movable_impl::operator=(T&& rhs) volatile &&;
//
//   Assigns lrvalue reference object of type T to the rvalue Contained object.
//
////
// Validity (is object constructed)
////
//  void destructively_movable_impl::is_valid()           const          noexcept;
//  void destructively_movable_impl::is_valid()           const volitile noexcept;
//
//   Returns if the Contained object is constructed.
//
//  bool destructively_movable_impl::has_been_moved()          noexcept;
//  bool destructively_movable_impl::has_been_moved() volatile noexcept;
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
//     To get around this issue, one can define a Destruct type trait in the
//     destructively_movable_traits specialisation for Contained.  This would
//     have to be implemented for any Contained type that contains types that
//     have this issue, which is definitely not nice.  This is really a
//     workaround.  Objects that have been moved, should not create or hang on
//     to *ANY* resources.  Doing so make a mess on more than one level.
template <typename Contained, typename Enabled = void>
class destructively_movable
    : public detail::destructively_movable_impl<Contained>
{
    using base = detail::destructively_movable_impl<Contained>;
public:
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (destructively_movable const& over destructively_movable_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    destructively_movable(destructively_movable     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    destructively_movable(destructively_movable const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    destructively_movable&  operator=(destructively_movable     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    destructively_movable&& operator=(destructively_movable     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }

    void is_valid(bool value)                noexcept { assert(!value);        Is_valid(*this, tombstone_tag()); }
    void is_valid(bool value)       volatile noexcept { assert(!value);        Is_valid(*this, tombstone_tag()); }
    bool is_valid(          ) const          noexcept {                 return Is_valid(*this); }
    bool is_valid(          ) const volatile noexcept {                 return Is_valid(*this); }

    using base::base;
};

template <typename Contained>
class destructively_movable<Contained, destructively_movable_is_valid<Contained>>
    : public detail::destructively_movable_impl<Contained>
{
    using base = detail::destructively_movable_impl<Contained>;
    bool m_isValid;
public:
    // This copy/move constructor is needed because on copying/moving,
    // m_isValid will be updated after is was already set by the
    // base copy/move constructor.  This either invalidates the value
    // or results in an extra copy.
    //
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (destructively_movable const& over destructively_movable_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    destructively_movable(destructively_movable     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    destructively_movable(destructively_movable const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    destructively_movable&  operator=(destructively_movable const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    destructively_movable&& operator=(destructively_movable const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }

    void is_valid(bool value)                noexcept {        m_isValid = value; }
    void is_valid(bool value)       volatile noexcept {        m_isValid = value; }
    bool is_valid(          ) const          noexcept { return m_isValid; }
    bool is_valid(          ) const volatile noexcept { return m_isValid; }

    using base::base;
};

template <typename T>
struct is_destructively_movable : std::false_type {};

template <typename T, typename I>
struct is_destructively_movable<destructively_movable<T, I>> : std::true_type {};

template <typename T>
using is_destructively_movable_t = is_destructively_movable<T>;

namespace detail {
template <size_t Size>
class storage
{
    std::byte m_storage[Size];
};

template <>
class storage<0>
{
};

template <typename Contained>
class alignas(Contained) destructively_movable_impl
    : private storage<std::is_empty<Contained>::value ? 0 : sizeof(Contained)>
{
    using Derived  = destructively_movable<Contained>;
    using Is_valid = destructively_movable_is_valid<Contained>;
    using Destruct = destructively_movable_destruct<Contained>;

    Derived                * derived()                noexcept { return static_cast<Derived                *>(this); }
    Derived       volatile * derived()       volatile noexcept { return static_cast<Derived       volatile *>(this); }
    Derived const          * derived() const          noexcept { return static_cast<Derived const          *>(this); }
    Derived const volatile * derived() const volatile noexcept { return static_cast<Derived const volatile *>(this); }

    template <typename U>
    static auto&& object(U&& this_ref) noexcept
    {
        assert(this_ref.is_valid());
        return
            fwd_like<U>(
                *std::launder(
                    reinterpret_cast<copy_cv_t<U, Contained*>>(
                        std::addressof(this_ref)
                    )
                )
            );
    }

    static constexpr bool has_external_tombstone = std::is_same<Derived, destructively_movable<Contained, void>>::value;

    template <typename T> struct bare_type_impl                           { using type = T; };
    template <typename T> struct bare_type_impl<destructively_movable<T>> { using type = T; };

    // Strips and extract the wrapped type or if not wrapped, just return the type.
    template <typename T> using  bare_t = typename bare_type_impl<strip_t<T>>::type;
    
    template <typename T> using  fwd_to_bare_type_t = fwd_type_t<T, bare_t<T>>;

    template<typename...Ts>
    static constexpr bool is_destructively_movable_same_contained() noexcept
    {
        if constexpr (sizeof...(Ts) == 1) {
            using T = strip_t<std::tuple_element_t<0, std::tuple<Ts...>>>;
            if constexpr (is_destructively_movable<T>::value)
            {
                if constexpr (std::is_same<Contained, typename T::contained>::value)
                {
                    return true;
                }
            }
        }
        return false;
    }

    destructively_movable_impl(destructively_movable_impl const&) = delete;
    destructively_movable_impl& operator=(destructively_movable_impl const&) = delete;
public:
    using contained = Contained;

    destructively_movable_impl(tombstone_tag) noexcept {
        is_valid(false);
        assert(!is_valid());
    }

    template <typename...Ts>
    destructively_movable_impl(Ts&&...args) noexcept(noexcept(
        construct(std::forward<Ts>(args)...)
    ))
    {
        construct(std::forward<Ts>(args)...);
        assert(is_valid());
    }

    // Didn't want to have a separate "copy" and "move" constructor.  Couldn't
    // use trinary operator without both being evaluated sides being evaluated
    // and causing problems.  Is this better or worse than seperate construct
    // functions?
    template <typename...Ts>
    static constexpr bool construct_noexcept()
    {
        if constexpr (is_destructively_movable_same_contained<Ts...>()) {
            return noexcept(
                std::declval<Derived>()
                .construct(emplace<Contained>(std::forward<Ts>(std::declval<Ts>()).object()...)));
        }
        else {
            return noexcept(
                std::declval<Derived>()
                .construct(emplace<Contained>(std::forward<Ts>(std::declval<Ts>())...)));
        }
    }

    // Does emplace construction of Contained, including "move/copy
    // construction".
    //
    // NOTE: Neither move or copy constructors would actually be called because
    //       this class is never passed a destructively_movable_impl const& or
    //       destructively_movable_impl&& directly anyway, since the derived
    //       class explicitly passes a destructively_movable const& or
    //       destructively_movable&& respectively.
    template <typename...Ts>
    Derived* construct(Ts&&...args)
        noexcept(
            construct_noexcept<Ts...>()
        )
    {
        if constexpr (is_destructively_movable_same_contained<Ts...>()) {
            construct(emplace<Contained>(std::forward<Ts>(args).object()...));
        }
        else {
            construct(emplace<Contained>(std::forward<Ts>(args)...));
        }
        assert(is_valid());
        return static_cast<Derived*>(this);
    }

    // Does emplace construction of T
    // If going to accept derived types, then the size of objects must be the same.
    template <typename T, typename const_tag, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    Derived* construct(emplace_params<T, const_tag, Ts...>&& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        //OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (has_external_tombstone) {
            is_valid(true);
        }
        assert(is_valid());
        if constexpr (is_destructively_movable_same_contained<Ts...>())
        {
            auto&& value = get<0>(emplace);
            if constexpr (!std::is_lvalue_reference_v<decltype(value)> && value.has_external_tombstone) {
                // Moved and has external tombstone, so exlicitly clear.
                value.is_valid(false);
            }
            // Operation was a move on a destructively_movable object.
            assert(!value.is_valid());
        }
        return static_cast<Derived*>(this);
    }

    // Does emplace construction of T.
    //
    // A const emplace_params implies that all params passed to constructor
    // will be const lvalues, regardless of what they actually were.  This
    // allows for safe reuse of an emplace_params.
    //
    // If going to accept derived types, then the size of objects must be the same.
    template <typename T, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    Derived* construct(emplace_params<T, Ts...> const& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (has_external_tombstone) {
            is_valid(true);
        }
        assert(is_valid());
        return static_cast<Derived*>(this);
    }

    ~destructively_movable_impl()
    {
        if constexpr (!std::is_trivially_destructible<Contained>::value) {
            if (is_valid()) {
                object().~Contained();
            }
            else {
                Destruct()(std::launder(reinterpret_cast<Contained*>(this)));
            }
        }
    }

    void destruct()
    {
        assert(is_valid());
        this->~destructively_movable_impl();
        if constexpr (has_external_tombstone) {
            is_valid(false);
        }
        assert(!is_valid());
    }

    void has_been_moved() volatile noexcept
    {
        assert( has_external_tombstone &&  is_valid()
            || !has_external_tombstone && !is_valid());
        if constexpr (has_external_tombstone) {
            is_valid(false);
        }
        assert(!is_valid());
    }

    void has_been_moved()          noexcept
    {
        assert( has_external_tombstone &&  is_valid()
            || !has_external_tombstone && !is_valid());
        if constexpr (has_external_tombstone) {
            is_valid(false);
        }
        assert(!is_valid());
    }

private:
    // Used tag dispatch rather than function exclusion because heard that it's
    // slightly better compile time performance.  This does make it for a
    // slightly easier noexcept spec, though need to refer to caller to see
    // what tag means.
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs, std::true_type)
        noexcept(
            noexcept(std::forward<T>(lhs).object() = std::forward<U>(rhs).object())
        )
    {
        // Only assign something if there is something to assign.
        if (rhs.is_valid()) {
            // Assign the underlying lhs object to the underlying rhs object.
            // Don't use object().operator=(...) because even if Contained is
            // an object that has an operator=() member function, the compiler
            // seems to get confused.  However, generally, it's limiting the
            // type as types can be assignable, but not have an operator=(...)
            // function, such as primitive types.
            std::forward<T>(lhs).object() = std::forward<U>(rhs).object();
            assert(lhs.is_valid());
            if constexpr (rhs.has_external_tombstone) {
                rhs.is_valid(false);
            }
            assert(!rhs.is_valid());
        }
        else if (lhs.is_valid()) {
            lhs.destruct();
        }
        return fwd_like<T>(static_cast<Derived&>(lhs));
    }

    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs, std::false_type)
        noexcept(
            noexcept(std::forward<T>(lhs).object() = std::forward<U>(rhs))
            && noexcept(lhs.construct(std::forward<U>(rhs)))
        )
    {
        if (lhs.is_valid()) {
            // Assign the underlying lhs object to the rhs object.
            std::forward<T>(lhs).object() = std::forward<U>(rhs);
        }
        else {
            lhs.construct(std::forward<U>(rhs));
        }
        assert(!is_destructively_movable<U>::value || lhs.is_valid());
        return fwd_like<T>(static_cast<Derived&>(lhs));
    }

    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs)
        noexcept(noexcept(
            assign(std::forward<T>(lhs), std::forward<U>(rhs), is_destructively_movable_t<std::remove_reference_t<U>>{})
        ))
    {
        return assign(std::forward<T>(lhs), std::forward<U>(rhs), is_destructively_movable_t<std::remove_reference_t<U>>{});
    }

public:
    template <typename T> auto&& operator=(T&& rhs)          &  { return assign(         (*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile &  { return assign(         (*this), std::forward<T>(rhs)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    template <typename T> auto&& operator=(T&& rhs)          && { return assign(std::move(*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile && { return assign(std::move(*this), std::forward<T>(rhs)); }

private:
    void is_valid(bool value)                noexcept {        derived()->is_valid(value); }
    void is_valid(bool value)       volatile noexcept {        derived()->is_valid(value); }

public:
    bool is_valid(          ) const          noexcept { return derived()->is_valid(); }
    bool is_valid(          ) const volatile noexcept { return derived()->is_valid(); }

    // Access object as lvalue reference
    auto&& object()                &  noexcept { return object(          *this ); }
    auto&& object()       volatile &  noexcept { return object(          *this ); }
    auto&& object() const          &  noexcept { return object(          *this ); }
    auto&& object() const volatile &  noexcept { return object(          *this ); }

    // Access object as rvalue reference
    auto&& object()                && noexcept { return object(std::move(*this)); }
    auto&& object()       volatile && noexcept { return object(std::move(*this)); }
    auto&& object() const          && noexcept { return object(std::move(*this)); }
    auto&& object() const volatile && noexcept { return object(std::move(*this)); }

    // Member of operators (TTA: should I use std::addressof instead of &?)
    auto* operator->()                noexcept { return &object(); }
    auto* operator->()       volatile noexcept { return &object(); }
    auto* operator->() const          noexcept { return &object(); }
    auto* operator->() const volatile noexcept { return &object(); }

    // Address of operators (TTA: should I use std::addressof instead of &?)
    auto* operator& ()                noexcept { return &object(); }
    auto* operator& ()       volatile noexcept { return &object(); }
    auto* operator& () const          noexcept { return &object(); }
    auto* operator& () const volatile noexcept { return &object(); }

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
// Using ... instead of a named parameter because when enabler(const_volatile) is called with
// const_volatile being blank (no cv), VC++ complains.
#define IS_CV_STRONGER_OR_SAME(...) (is_stronger_or_same_cv_v<U, int __VA_ARGS__>)
#define IS_ALWAYS_ENABLED(...)      (!always_false<U>)

#define CONVERSION(const_volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        template <typename U, std::enable_if_t< enabler(const_volatile) , int> = 0>             \
        explicit operator U convert_to_lrvalue () const_volatile convert_from_lrvalue noexcept  \
        {                                                                                       \
            return fwd_like<U convert_to_lrvalue>(const_cast<Contained&>(object()));            \
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

    CONVERSION_ALL_CVS(&&, & ,                      IS_ALWAYS_ENABLED, explicit);
    CONVERSION_ALL_CVS(& , &&,                      IS_ALWAYS_ENABLED, explicit);

#undef CONVERSION_ALL_CVS
#undef CONVERSION
#undef IS_ALWAYS_ENABLED
#undef IS_CV_STRONGER_OR_SAME
#undef IS_UPCAST
#undef IS_DOWNCAST
};

} // namespace detail
} // namespace afh
#endif // #ifndef AFH_DESTRUCTIVE_MOVE_HPP__
