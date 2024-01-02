#pragma once

#include <cxxabi.h>
#include <string>

#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE
#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC

template<typename T>
std::string demangled_type_name() {
    int status;
    char* demangled = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string name(demangled);
    free(demangled);
    return name;
}