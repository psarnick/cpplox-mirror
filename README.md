# cpplox

Based on http://craftinginterpreters.com

How to run:
* `mkdir build && cd build`
* `cmake ..`
* `cmake --build . -- -j 8`
* `ctest -R VM`
* `cat compiler.log` to see bytecode and execution trace

Done: control flow (conditions and loops). Next: functions.

TODOs: 
* Finish book
* Book's challenges
* Benchmark Bytecode codebase - are Values passed around efficiently? OPP offers better structure std::variant?
* Refactor & document Treewalk codebase - OPP offers better structure than std::variant?
* Register-based VM
* Codegen optimisations
* Decompiling
* Compile to LLVM
* Compile to M2?
