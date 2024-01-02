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

Done: closures; in progress: GC (does not work yet & most of the tests fail - drop me a message if you want latest working revision).

TODOs: 
* Finish book
* Book's challenges
* Benchmark Bytecode codebase - are Values passed around efficiently? OPP offers better structure std::variant?
    * Define lifecycle interface
        - Tokens in Scanner should make copies, not point to raw string (could be freed up after Scanning)
        - Bytecode constants do/do not point to values stored in Tokens?
* Refactor & document Treewalk codebase - OPP offers better structure than std::variant?
* Register-based VM
* Codegen optimisations
* Decompiling
* Compile to LLVM
* Compile to M2?