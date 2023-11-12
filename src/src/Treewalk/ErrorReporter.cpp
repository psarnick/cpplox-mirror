#include "cpplox/Treewalk/ErrorReporter.h"

#include <iterator>
#include <sstream>
#include <vector>

namespace clox::ErrorsAndDebug {

void ErrorReporter::set_error(std::string msg) {
  status = LoxStatus::ERROR;
  error_msgs.push_back(msg);
}

std::string ErrorReporter::to_string() {
  const char* const delim = "\n";
  std::ostringstream imploded;
  std::copy(error_msgs.begin(), error_msgs.end(),
            std::ostream_iterator<std::string>(imploded, delim));
  return imploded.str();
}

void ErrorReporter::clear() {
  status = LoxStatus::OK;
  error_msgs.clear();
}

bool ErrorReporter::has_error() { return status == LoxStatus::ERROR; }

LoxStatus ErrorReporter::get_status() { return status; }

}  // namespace clox::ErrorsAndDebug
