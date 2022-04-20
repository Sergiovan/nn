# Compiletime nn features

This document loosely explains all compiletime nn features. These are features that involve computations at compilation time instead of at runtime.

This is all very vague and might not even be a thing when I get here, but just a general idea.

## Compiletime variables
Declaring variables with `let` makes the compiletime variables, meaning their value _has to be_ known at compiletime, and is available anywhere where a value might be needed at such times. 

## Compiletime functions
Functions declared with `<>` instead of `()` are compiletime functions, and can return funky things like function definitions and types. This is how we get generics. I gotta think about why this isn't a good idea still, for now I like it.

## Static code generation
Since functions can run at compiletime, with access to the ast and the source code and all that, it should be possible to modify these in-place as the program is running, effectively generating code (but a little abstracted).

## Types as compiletime expressions
Types are just another sort of value, except it's always compiletime since all types must be known at runtime. However as compiletime functions can return types or make new types or do many other funky things, the types of declarations (i.e. the expressions after `:`) are just more compiletime expressions.

## Compiler notes
To access compiler intrinsics or annotate code compiler notes are used, which are identifiers that start with `$`.

## Conditional compilation
It should be possible to compile different programs depending on outside parameters, like compiler parameters or target architecture.

## Interpreter
Since there's compiletime functions that need to execute arbitrary expressions, an interpreter to run programs should also be made (to be used as a runtime too)

## Project management as code
Instead of relying on external tools and files, project management and compilation can simply be embedded as a compiletime function or functions or values in the program.