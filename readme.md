# lang
## depends
Flex and Bison are used for tokenising and parsing the language. Many distros will provide flex and bison by default, in Arch they are in package group base-devel.

```# pacman -S flex bison```

LLVM (for LLVM IR backend) and glslang (for SPIR-V backend) for code generation.

```# pacman -S llvm-libs glslang```

## usage
```
$ make
$ ./out/compiler
> u32 a = 100;
> u32 b = 0b100_100_100;
> a * b;
109200
```

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
  - should support arbitrary mixing and matching of execution modes/backends: REPL, compiletime, LLVM IR, and SPIR-V
    - would be _incredible_ for debugging experience to REPL code on the GPU
  - allow the user to specify where/when each bit of code should execute, and handle the data transfer in a default sensible way
    - cross machine: TCP
    - cross thread: shared memory
    - to/from GPU: various??
  - LLVM IR and SPIR-V backends cover all platforms
    - all kinds of SIMD, SSE, AVX, NEON, etc
    - all important GPUs
- better operators
  - first class support for
    - vectors and matrices
    - bitwise and bytewise shifts, rotates, shuffles, etc
    - all the kinds of atomics and synchronisation primitives in LLVM IR and SPIRV
  - clearer modulo and remainder differentiation
    - and allow "always positive modulo" with builtin
