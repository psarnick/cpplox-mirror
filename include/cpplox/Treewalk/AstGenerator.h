#pragma once
#include <string>
#include <vector>

void generate_asts(std::string dir_path, std::string fname,
                   std::vector<std::vector<std::string>> all_types);
void generate_cpp(std::string fpath, std::string fname,
                  std::vector<std::vector<std::string>> all_types);
void generate_header(std::string fpath, std::string fname,
                     std::vector<std::vector<std::string>> all_types);
void generate_visitor(std::string fpath, std::string fname,
                      std::vector<std::vector<std::string>> all_types);