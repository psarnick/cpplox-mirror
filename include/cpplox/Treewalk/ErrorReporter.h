#pragma once
#include <string>
#include <vector>

namespace clox::ErrorsAndDebug {

enum class LoxStatus { OK, ERROR };

class ErrorReporter {
 public:
  bool has_error();
  LoxStatus get_status();
  void set_error(std::string message);
  std::string to_string();
  void clear();

 private:
  LoxStatus status{LoxStatus::OK};
  std::vector<std::string> error_msgs{};
};

}  // namespace clox::ErrorsAndDebug
