# ROADMAP

This document describes how the project will proceed, roughly, mostly, probably, at least if I have something to say about it. 

The DONE section shows milestones already reached.

The TODO section shows semi-psuedo-vaguely-immediate issues that should be tackled. 

The IMMEDIATE FUTURE section shows what's upcoming next.
The FUTURE section shows what's to happen in a non-chronological, semi-accurate, per-release basis.

The WHISHLIST section shows things that I would like to get done at some point because I think they're neat.

## DONE
- There doesn't seem to be anything here yet...

## TODO
For v0.0.1.
- **Translation**:
    - Lexer:
        - Number parsing
    - Parser:
        - `$builtin()`
    - IR Generator:
        - "Destructors"
        - Variable lifetimes
    - IR-to-nnASM step
    - nnASM:
        - Proper alignment for stuff
- **Execution**:
    - Allocator
    - Test all opcodes
- **Optimization**:
    - AST optimizer
    - IR optimizer
    - nnASM optimizer


## IMMEDIATE FUTURE
- **v0.0**: Project start. ✓
- **v0.0.1**: mastermind2.nn program compiles from start to finish. 
- **v0.0.2**: Optimizations.
- **v0.0.3**: Store and run programs without compiling everything every time.
- **v0.1**: Syntax and semantics works as originally envisioned, if not perfectly.

## FUTURE
- **v0.1.1**: Calling C library functions.
- **v0.1.2**: Convert C headers to nn "headers".
- **v0.1.3**: Standard library beginnings.
- **v0.2**: Can write C bindings and call them.
- **v0.2.1**: `$if`
- **v0.2.2**: `$define`
- **v0.2.3**: `$defined`
- **v0.2.4**: `$$var`
- **v0.3**: Useful compiler flags (See .spec/compiler_notes.txt)
- **v0.3.1**: Proper linking.
- **v0.3.2**: Incremental compilation.
- **v0.3.3**: Compile to ELF format.
- **v0.3.4**: Compile to PE format.
- **v0.3.5**: Compile to arm64.
- **v0.3.6**: Compile to x86_64.
- **v0.4**: Compile to run without vm.
- **v0.4.1**: Compiler options.
- **v0.4.2**: Environment variables.
- **v0.4.3**: Disk locations.
- **v0.5**: Proper command line interface.
- **v0.5.1**: Compiler warnings.
- **v0.5.2**: Compiler errors show info.
- **v0.5.3**: More specific errors.
- **v0.5.4**: Debug information.
- **v0.5.5**: Debugger.
- **v0.6**: Better debugging.
- **v0.6.1**: Pure functions.
- **v0.6.2**: Compile time math.
- **v0.6.3**: Inlining.
- **v0.7**: Serious optimizations.
- **v0.7.1**: typeof and sizeof.
- **v0.7.2**: Coroutines.
- **v0.7.3**: any type.
- **v0.7.4**: Generic structs and functions.
- **v0.7.5**: Combination type casting.
- **v0.7.6**: "Moving"
- **v0.8**: Syntax extended (See .spec/future_stuff.txt)
- **v0.8.1**: `$macro`
- **v0.8.2**: `$operator`
- **v0.8.3**: `$constructor`
- **v0.8.4**: `$destructor`
- **v0.8.5**: `$this`
- **v0.8.6**: `$main`
- **v0.9**: More compiler flags (See .spec/compiler_notes.txt)
- **v0.9.1**: Bootstrapping.
- **v0.9.2**: More optimizations.
- **v0.9.3**: General bug fixing and will-do-laters.
- **v1.0**: Compiler.

## WISHLIST
- Syntax information interface.
- Language server interface.
- Minimal IDE.