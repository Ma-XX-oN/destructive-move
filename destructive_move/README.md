# destructively_movable

## Introduction

This library is in responce to the question in [CppCon Grill the Committee](https://youtu.be/cH0nJPbMFAY?t=1263) about having destructive moves added to C++.  I thought that it was an interesting problem, and wondered if a library solution could be made.  So I made this "toy" container to flesh out the possibility and, with the advent of *C++17*, it worked out quite well.

`destructively_movable` is a template container that allows the "dropping on the floor" of an object husk after its contents has been moved to another location.  This means that the destructor for that object is never called which, if it contains many objects, can result in significant performance improvements.  This is still in some development and testing stage, but is in a point where it does work and where I feel can be shown to the community for feedback.

## Description

A `destructively_movable` template is a type that hides its Contained type from the compiler, but will call that Contained type's destructor if it is determined that the object isn't constructed (tricky part).

It has the same size[<sup>[1]</sup>](#caveat-same-size) and alignment of the Contained type and is supposed to be an (almost) drop-in replacement[<sup>[2]</sup>](#caveat-drop-in-replacement) for that type.

The syntax is straight forward:

```c++
struct X { /*...*/ };

afh::destructively_movable<X> x;
```

This will define a variable `x` that is of type X which is destructively movable.  Note that this will work for the trivial case where X is trivially destructible and will not add any overhead in that case.

## Tombstoned State

Without any additional code, the previous example will result in the size to be a little larger than `X` would have been on its own.  This is because there is nothing describing an internal tombstone state or how to set it, so it must add an external Boolean to the object to flag that state.  To specify what the internal tombstone state is and how to set it, a `destructively_movable_traits` specialisation is needed to be added.

```c++
struct X {
  /*...*/
  struct Is_tombstoned {
    bool operator()(X const& x) {
      // Check x for tombstone state
      return /*...*/;
    }
    void operator()(X& x, afh::tombstone_tag) {
      // Set x to tombstone state.  Assumed that x is uninitiaised
      // or that the contents have been moved.
    }
  };
};

template<>
::afh::destructively_movable_traits<X> {
  // TTA: Should I make this auto-check X for Is_tombstoned?
  using Is_tombstoned = typename X::Is_tombstoned;
};

destructively_movable<X> x;
```

What is a tombstoned state?  It is a state that indicates the the object holds no resources and can be destroyed without having to call it's destructor.  Having an external tombstone state has its drawbacks (see caveats[<sup>[3]</sup>](#caveat-external-tombstone)).

# Drop-in Replacment
As this is ment to be a drop-in replacement[<sup>[2]</sup>](#caveat-drop-in-replacement), the constructor of the Contined type is called via perfect forwarding.  So this:

```c++
X x{a, b, c};
```

becomes:
```c++
afh::destructively_movable<X> x{a, b, c};
```

Calls to functions that take `X` don't have to change at all.  So calling `fn(X&)` or `fn(base_of_X&)` will just work as well if you pass an `afh::destructively_movable<X>` object rather than a `X` object.  However, if passing to a function as an rvalue (e.g. `fn(X&& x)`, where `x` will have it's contents moved, then see caveats section[<sup>[3]</sup>](#caveat-external-tombstone).

Implicit casts all work as well, from changing to a stronger cv qualifier, to downcasting.  However, explict casts also work, from changing to a weaker/different cv qualifiers, to upcasting[<sup>[4]</sup>](#caveat-upcasting), to switching between lvalue/rvalue reference types.

The `operator&` is also overloaded to return a cv qualified `Contained*` and if for some reason, the object needs to be retrived in some other way that isn't usable via the other means, then there is an `object()` method that returns a fully ref and cv qualified reference based on the qualifiers of the `destructively_movable` container object.

## Destruction
When a `destructively_movable` object is destroyed, its destructor is still called, but the destuctor will only call the Contained object's destructor if the tombstone marker is set.  So, if this object contains more than one object, this should cause a slight performance boost.  The more sub-objects the Contained object consists of that have non-trivial destructors, the greater the performance gain.  A moved object that allocates/holds onto resources will not work in this scenario (see caveats[<sup>[5]</sup>](#caveat-hold-resource-after-move))

## Caveats

1. <a name="caveat-same-size"></a>
   The size is only the same if the type has some intenal tombstone marker.

2. <a name="caveat-drop-in-replacement"></a>
   The structure reference (*`operator.`*) isn't overloadable.  So we must be satisfied with either using the structure dereference (*`operator->`*) to access the Contained object's members or getting the object via cast or function call and performing operations on that.

3. <a name="caveat-external-tombstone"></a>
   It is always better to have the tombstone internal to the object.  Although an external tombstone is allowed, an issue can arise if the Contained object gets moved without the container knowing about it.  If this happens, then the Contained object will still get its destructor called.  So long as the object maintains the "valid but otherwise indeterminate state", this will result in still defined behaviour, although it will negate the performance improvement.\
\
   This performance loss can be regained by calling `has_been_moved()`, which will correct the state and ensure that the validity chaecks out correctly.  It can be called even if it has an internal tombstone without any performance penelty (with optimisations on). If called when the contained object has not been moved, it will most likely result in resource leaks, so don't do that.

4. <a name="caveat-upcasting"></a>
   Upcasting is not available unless the type being casted to doesn't increase in size.

5. <a name="caveat-hold-resource-after-move"></a>
   If, after a move, the Contained object allocates/retains any resources (cou-*MicroSoft*-gh! ;)), then using `destructively_movable` on that object will cause a leak in the moved from object.\
\
   To get around this issue, one can define a `Destruct` type trait in the `destructively_movable_traits` specialisation for Contained.  This would have to be implemented for any Contained type that contains types that have this issue, which is definitely not nice.  This is really a workaround.  IMO, the standard should be made more specific to say something to the effect that Objects that have been moved, should not retain (or create) *ANY* resources.  Doing so causes problems here and other places (such as move constructor to be not marked as noexcept because of allocating of memory *cough-cough*).