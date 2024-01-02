#pragma once
#include "cassert"
#include <span>
#include "cpplox/Bytecode/Value.h"


namespace cpplox {

Value clock(int arg_count, std::span<Value> args);
// Value concat(int arg_count, std::span<Value> args);
}