# HSH Shader Pipeline and Graphics Library

**HSH** is a modular shader compilation pipeline and run-time graphics library. 
 
As an extended fork of LLVM's monorepo, HSH utilizes clang's parsing
capabilities to transform C++17 semantics into complete Vulkan pipelines
with HLSL as an intermediate shader language. The pipelines are encoded into
C++ headers along with necessary metadata to create pipelines at run-time.

Shader pipelines are defined as C++ source files with one or more 
classes inheriting `hsh::pipeline`. The constructors of these pipeline classes
contain statements using the builtin hsh types and intrinsic functions to
define shader code in a language-agnostic manner. The constructor parameters
may be declared with one or more resource handles; interfacing application
uniform data and textures with the generated shaders. Along with the shader
data in an API-ready representation, metadata is generated to establish vertex
formats, sampler parameters, blend modes and other information used by the
pipeline. Init-time declarations are generated forming an application-wide
linked list of all compiled pipelines. This list is iterated and nodes are
called to build all pipelines at run-time before actual rendering begins.

C++ templates may be used to define variations on a common shader model.
All specializations of the template supply constant parameters that the shader
template may use to vary specific aspects of its functionality (i.e. vertex
format array sizes, control-flow selection of post-processing statements,
blend modes, etc). For applications that cannot reasonably declare all possible
template specializations explicitly, a profile-guided mode is available where
non-constant values are bound to template parameters. Rendering is not
available in this profiling mode. Instead, applications are expected to
bind every possible pipeline specialization (based on all potential graphics
resources or other artistic information) and exit. A header file of these
unique specializations is emitted. Therefore, all pipelines can be compiled
offline as long as the application can operate as an enumerating pipeline
bind tool. Online pipeline compilation is currently outside the scope of hsh.

## hshgen Build Tool

*Found at `clang/tools/hshgen/hshgen.cpp` and `clang/lib/Hsh/HshGenerator.cpp`*

This compile-time tool is used to generate shader code and pipeline metadata
headers (`.hshhead`) from C++ files. When combined with the supplied CMake
package, applications can easily integrate hshgen on a per-source-file basis.

## libhsh Run-time Library

*Found at `libhsh`*

The offline preparation of hshgen is applied at run-time via the header-only
**libhsh**. This library serves as a lightweight wrapper around the Vulkan
graphics API. Types and constructs are provided to easily set up a rendering
loop and resource ownership model.

## Getting Started with HSH

In general, the build process of hsh is the same as vanilla LLVM. Build targets
of interest include:

| Target               | Description                                                        |
|----------------------|--------------------------------------------------------------------|
| hshgen               | Compile-time hshhead generator tool                                |
| check-hsh            | Regression tests for verifying translated source code              |
| install-distribution | Installs a minimal hsh distribution with hshgen and libhsh headers |

The original README contains detailed instructions on the build workflow as
follows:

# The LLVM Compiler Infrastructure

This directory and its sub-directories contain source code for LLVM,
a toolkit for the construction of highly optimized compilers,
optimizers, and run-time environments.

The README briefly describes how to get started with building LLVM.
For more information on how to contribute to the LLVM project, please
take a look at the
[Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting Started with the LLVM System

Taken from https://llvm.org/docs/GettingStarted.html.

### Overview

Welcome to the LLVM project!

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and converts it into
object files.  Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer.  It also contains basic regression tests.

C-like languages use the [Clang](http://clang.llvm.org/) front end.  This
component compiles C, C++, Objective C, and Objective C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

### Getting the Source Code and Building LLVM

The LLVM Getting Started documentation may be out of date.  The [Clang
Getting Started](http://clang.llvm.org/get_started.html) page might have more
accurate information.

This is an example work-flow and configuration to get and build the LLVM source:

1. Checkout LLVM (including related sub-projects like Clang):

     * ``git clone https://github.com/llvm/llvm-project.git``

     * Or, on windows, ``git clone --config core.autocrlf=false
    https://github.com/llvm/llvm-project.git``

2. Configure and build LLVM and Clang:

     * ``cd llvm-project``

     * ``mkdir build``

     * ``cd build``

     * ``cmake -G <generator> [options] ../llvm``

        Some common build system generators are:

        * ``Ninja`` --- for generating [Ninja](https://ninja-build.org)
          build files. Most llvm developers use Ninja.
        * ``Unix Makefiles`` --- for generating make-compatible parallel makefiles.
        * ``Visual Studio`` --- for generating Visual Studio projects and
          solutions.
        * ``Xcode`` --- for generating Xcode projects.

        Some Common options:

        * ``-DLLVM_ENABLE_PROJECTS='...'`` --- semicolon-separated list of the LLVM
          sub-projects you'd like to additionally build. Can include any of: clang,
          clang-tools-extra, libcxx, libcxxabi, libunwind, lldb, compiler-rt, lld,
          polly, or debuginfo-tests.

          For example, to build LLVM, Clang, libcxx, and libcxxabi, use
          ``-DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi"``.

        * ``-DCMAKE_INSTALL_PREFIX=directory`` --- Specify for *directory* the full
          path name of where you want the LLVM tools and libraries to be installed
          (default ``/usr/local``).

        * ``-DCMAKE_BUILD_TYPE=type`` --- Valid options for *type* are Debug,
          Release, RelWithDebInfo, and MinSizeRel. Default is Debug.

        * ``-DLLVM_ENABLE_ASSERTIONS=On`` --- Compile with assertion checks enabled
          (default is Yes for Debug builds, No for all other build types).

      * ``cmake --build . [-- [options] <target>]`` or your build system specified above
        directly.

        * The default target (i.e. ``ninja`` or ``make``) will build all of LLVM.

        * The ``check-all`` target (i.e. ``ninja check-all``) will run the
          regression tests to ensure everything is in working order.

        * CMake will generate targets for each tool and library, and most
          LLVM sub-projects generate their own ``check-<project>`` target.

        * Running a serial build will be **slow**.  To improve speed, try running a
          parallel build.  That's done by default in Ninja; for ``make``, use the option
          ``-j NNN``, where ``NNN`` is the number of parallel jobs, e.g. the number of
          CPUs you have.

      * For more information see [CMake](https://llvm.org/docs/CMake.html)

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-started-with-llvm)
page for detailed information on configuring and compiling LLVM. You can visit
[Directory Layout](https://llvm.org/docs/GettingStarted.html#directory-layout)
to learn about the layout of the source code tree.
