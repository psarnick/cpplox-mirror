# cpplox

Based on http://craftinginterpreters.com

How to build and test:
* `mkdir build && cd build`
* `cmake ..`
* `cmake --build . -- -j 8`
* `ctest -R VM`

How to run one file:
* `./src/cpplox ../test/for/syntax.lox` while in `build` directory
* `cat ./compiler.log` to see bytecode and execution trace 

Almost done: control flow (conditions and loops; one test still fails).
Next: functions.

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
