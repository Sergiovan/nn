# Advanced nn features

This document loosely explains all more advanced nn features.

It is unlikely that I'll ever get here at the current rate of rewriting and rethinking the project, but at least the ideas are documented somewhere.

## Prelude
Instead of hardcoding type names and functions in the compiler, put everything into the library with in-language facilities and import it automagically via the prelude.

## Methods on primitives
Add in-language way to describe primitives, basically. Kinda like what Rust does.

## Traits and Concepts
Adopt something similar to traits in Rust and concepts in C++ and typeclasses in Haskall. Basically, let types "subscribe" (explicit) or "model" (implicit) functionality, that can be used to disambiguate generics.

## Tagged unions, or algebraic data types
Variants, basically.

## Pattern matching
aka advanced destructuring, add pattern matching for declarations and for `match-case`

## Generators and fibers and async and threads
These are all interesting constructs that merit their own interesting syntax and decisions, but that's for later.

## Named returns
It would be interesting to be able to name return values, for clarity and to reference them from inside the function (for example, assign to all and return instead of returning all values at once)

## Overloading
### Static
Static overloading lets many functions share a second name, which is disambiguated from parameter and/or return type. Importantly, this shared name is not the actual name of the function, and all functions require unique names anyway.

### Dynamic
Dynamic overloading allows overloading on a dynamic paramter, inversely to static overloading. Many dynamic overloaded functions coalesce into a single function at the end, and the overloading happens at runtime based on the type of the arguments (which uses type information)

### Advanced pointer types
Shared, Weak and Unique pointers are made as painless to use as possible, hopefully masquerading as normal pointers.

## Moving and copying
Nihil emoji. Copy-by-default for most types, opt-in move-by-default? Special operators? Methods? Oh boy.

## import/export for libraries
For creating/using libraries, syntax niceties. 

## Magic functions
### Constructors
A way to define what happens when you construct a type. Or a default function for it. Or many default functions. Or an operator?

### Destructors
A way to defined what happens at the end of the lifetime of a type. Useful for RAII. 

### Operator overloading
A way to define operators for types, instead of needing to call functions. Alternatively, a way to define your own operators, and operator precedence.