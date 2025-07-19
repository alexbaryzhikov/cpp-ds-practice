# C++ Data Structures Practice

This repository is dedicated to practicing the implementation of standard data structures in C++.

## Implemented Data Structures:

### Dynamic Array

A dynamic array is a data structure that can grow or shrink in size during runtime. Unlike static arrays, which have a fixed size defined at compile time, dynamic arrays can allocate more memory as needed to accommodate new elements. This implementation involves:

- **Underlying Array:** Using a raw C++ array to store elements.
- **Capacity:** Tracking the current maximum number of elements the underlying array can hold.
- **Size:** Tracking the current number of elements actually stored in the array.
- **Resizing:** When the array reaches its capacity, a new, larger array is allocated, elements are copied over, and the old array is deallocated.

### Type List

A type list (also known as a typelist or metaprogramming list) is a compile-time data structure that holds a sequence of types. It's primarily used in C++ template metaprogramming to perform computations on types at compile time. This implementation involves:

- **Recursive Structure:** Defined using templates, where each "node" in the list holds a type and a reference to the rest of the list (or a special "end" type).
- **Compile-time Operations:** Operations like `push_back`, `push_front`, `pop_back`, `pop_front`, `get` (by index), `length`, and `contains` are implemented using template metaprogramming techniques, meaning they are resolved during compilation, not runtime.
- **No Runtime Overhead:** Since all operations are performed at compile time, type lists incur no runtime performance overhead.