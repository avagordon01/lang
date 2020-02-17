# kl lang
kl is a language for easy and high performance compute kernel and graphics shader programming. It targets CPUs and GPUs with both an LLVM-IR backend and a SPIR-V backend. The key idea is (taken from Halide) to separate the algorithm from the execution location/schedule/pattern. It does this by letting the programmer write the algorithm and then independently specify when and where bits of code are executed in a fine-grained way.

## depends
Meson and Ninja are used as the build system. Flex is used for tokenising. LLVM and glslang are required for the LLVM-IR and SPIR-V backend/codegen.

```# pacman -S meson ninja flex llvm llvm-libs glslang```

Alternatively, a [nix](https://nixos.org/nix/) derivation is provided. This can be used either by doing `nix-build`, or entering a `nix-shell` and entering the following commands.

## building
```
$ meson setup build
$ cd build
$ ninja
```

## usage
```
$ build/compiler input.kl output.ir
```

## testing
```
$ ninja test
```

## status
- frontend
  - [x] basic imperative language
  - [x] control flow (if/for/while/switch/functions)
  - [x] primitive types (bool, {u,i,f}{8,16,32,64})
  - [x] linkable with C
  - [ ] user defined types
    - [ ] structs
    - [ ] arrays
  - [ ] builtin functions
    - [ ] casts
    - [ ] maths
    - [ ] advanced bitwise
  - [ ] memory
    - [ ] heap backed variables
    - [ ] compiler knows all aliasing (or no aliasing)
    - [ ] pointers like C++ references and unique_ptrs
    - [ ] initialisation?
    - [ ] custom allocators
  - [ ] modules
    - [ ] import/export definitions from/to other kl files
    - [ ] import/export definitions from/to C files
    - [ ] top level file definition order unimportant
  - [ ] generic programming
    - [ ] compiletime type things
    - [ ] builtins type functions for "is a", "has a"
- backend
  - [x] LLVM backend
  - [ ] SPIR-V backend
  - [ ] evaluator/interpreter
    - [ ] arbitrary compiletime execution
  - [ ] switching between backends at arbitrary code locations
    - [ ] transparent transfer between processors (CPU, SIMD, GPU, ...)

## ideas
- halide and futhark inspiration
  - futhark is a language compiler written in haskell generating opencl
  - halide is an eDSL in C++ using LLVM as a backend, then generating x86, ARM, CUDA
  - futhark has a much nicer language/workflow versus halide
  - futhark and halide are both functional
  - halide algorithms are completely pure, taking pixel coordinates and saying what input and output pixel coordinates to operate on
  - "not turing complete"
  - because theres no recursion in halide
  - seperate out the algorithm from the schedule
  - give fine grained control over which and what shape cores to use
  - use an explicit parser and code generator, don't piggyback on C++ like halide
- sorting for locality 
  - halide doesnt (I think) sort the input for locality
  - could morton sort multidimensional input
  - greatest common subset of functionality between GPU, CPU, CPU SIMD
    - no recursion or virtual functions because GPUs dont support it
    - no strings
- interfacing
  - just generate blobs of object code that take pointers or short arrays of ints/bytes and return the same
  - maybe generate or parse C headers... probably hard
  - not sure how to manage the blobs, uploading to the GPU, spawning the threads etc
  - just operate on blobs in memory, no need for input/output operations, and especially no string operations
- execution
  - allow complete control of when code is compiled and where code is executed 
  - compilation modes: offline, JIT, REPL
  - _where_ the code executes
    - backends LLVM IR and SPIR-V give us basically all platforms
    - SIMD (SSE, AVX, NEON, etc)
    - FPU
    - GPU
    - multiple cores, sockets, machines...
    - handle the data transfer in a default sensible way
  - would be _incredible_ for debugging experience to REPL code on the GPU
- better operators
  - first class support for
    - vectors and matrices
    - bitwise and bytewise shifts, rotates, shuffles, etc
    - all the kinds of atomics and synchronisation primitives in LLVM IR and SPIRV
  - clearer modulo and remainder differentiation
    - and allow "always positive modulo" with builtin
