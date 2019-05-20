# destructively_movable


## Introduction

This library is in responce to the question in [CppCon Grill the Committee](https://youtu.be/cH0nJPbMFAY?t=1263) about having destructive moves added to C++.  I thought that it was an interesting problem, and wondered if a library solution could be made.  So I made this "toy" container to flesh out the possibility and, with the advent of *C++17*, it worked out quite well.

`destructively_movable` is a template container that allows the "dropping on the floor" of an object husk after its contents has been moved to another location.  This means that the destructor for that object is never called which, if it contains many objects, can result in significant performance improvements.  This is still in some development and testing stage, but is in a point where it does work and where I feel can be shown to the community for feedback.

## Description

A `destructively_movable` template is a type that hides its Contained type from the compiler, but will call that Contained type's destructor if it is determined that the object isn't constructed (tricky part).

It has the same size[<sup>[1]</sup>](#cavet-same-size) and alignment of the Contained type and is supposed to be an (almost) drop-in replacement[<sup>[2]</sup>](#cavet-drop-in-replacement) for that type.

The syntax is straight forward:

```c++
struct X { /*...*/ };

afh::destructively_movable<X> x;
```

This will define a variable `x` that is of type X which is destructively movable.  Note that this will work for the trivial case where X is trivially destructible and will not add any overhead in this case.

However, without any additional code, this will result in the size to be a little larger than `X` would have been on its own.  This is because there is nothing describing an internal tombstone state or how to set it, so it must adds an external Boolean to flag that state.  To specify the internal tombstone state, a `destructively_movable_traits` specialisation is needed to be added.

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

What is a tombstoned state?  It is a state that indicates the the object holds no resources and can be destroyed without having to call it's destructor.

As this is ment to be a drop-in replacement, the constructor of the Contined type is called via perfect forwarding.  So this:

```c++
X x{a, b, c};
```

becomes:
```c++
afh::destructively_movable<X> x{a, b, c};
```



## Cavets

1. <a name="cavet-same-size"></a>The size is only the same if the type has some intenal tombstone marker.
2. <a name="cavet-drop-in-replacement"></a>The structure reference (*`operator.`*) isn't overloadable.  So we must be satisfied with either using the structure dereference (*`operator->`*) to access the Contained object's members or getting the object via cast or function call and performing operations on that.
