#include <ctime>
#include <sstream>
#include "cpplox/Bytecode/NativeFunctions.h"


namespace cpplox {

  Value clock(int arg_count, std::span<Value> args) {
    return static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
  }

  /*
  TODO: This is currently broken as string is not a Value.
  Probably pass in a heap.
  Value concat(int arg_count, std::span<Value> args) {
    assert(arg_count < args.size());

    size_t offset {args.size() - arg_count};
    assert (std::holds_alternative<std::string>(args[offset]));
    std::string sep {std::get<std::string>(args[offset])};
    std::ostringstream result;
    for (size_t i = offset+1; i < args.size(); ++i) {
      result << value::to_string(args[i]);
      if (i < args.size() - 1) {
        result << sep;
      }
    }
    return result.str();
  }
  */
} // namespace cpplox