#include "cpplox/Treewalk/AstGenerator.h"

#include <algorithm>
#include <fstream>
#include <iostream>

void generate_asts(std::string dir_path, std::string fname,
                   std::vector<std::vector<std::string>> all_types) {
  generate_visitor(dir_path + "/" + fname + "Visitor.h", fname, all_types);
  generate_header(dir_path + "/" + fname + ".h", fname, all_types);
  generate_cpp(dir_path + "/" + fname + ".cpp", fname, all_types);
}

std::vector<std::string> split_str(std::string delimiter, std::string str) {
  std::vector<std::string> output{};
  size_t pos = 0;
  std::string token;
  while ((pos = str.find(delimiter)) != std::string::npos) {
    token = str.substr(0, pos);
    output.push_back(token);
    str.erase(0, pos + delimiter.length());
  }

  token = str.substr(0, str.size());
  output.push_back(token);
  return output;
}
std::string to_lowercase(const std::string& input) {
  std::string out{input};
  std::transform(
      out.begin(), out.end(), out.begin(),
      [](unsigned char c) -> unsigned char { return std::tolower(c); });
  return out;
}

void generate_header(std::string fpath, std::string fname,
                     std::vector<std::vector<std::string>> all_types) {
  std::ofstream f;
  f.open(fpath);
  f << "#pragma once" << std::endl;
  f << "#include <vector>" << std::endl;

  f << "#include \"cpplox/src/Types/Token.h\"" << std::endl;
  if (fname == "Stmt") {
    // Stms depends on Expr
    f << "#include \"cpplox/src/Expr/Expr.h\"" << std::endl;
  }
  f << "#include \"cpplox/src/" + fname + "/" + fname + "VisitorFwd.h\""
    << std::endl;
  f << std::endl;
  f << "namespace clox::" << fname << std::endl;
  f << "{" << std::endl;
  int indent{4};
  f << std::string(indent, ' ') << "using clox::Types::Token;" << std::endl;

  // Imports depend on type
  if (fname == "Expr") {
    f << std::string(indent, ' ') << "using clox::Types::Literal;" << std::endl;
    f << std::endl;
    f << std::string(indent, ' ') << "class Expr; //fwd declare" << std::endl;
    f << std::string(indent, ' ') << "using ExprPtr = std::unique_ptr<Expr>;"
      << std::endl;
    f << std::string(indent, ' ') << "using TokenPtr = std::unique_ptr<Token>;"
      << std::endl;
    f << std::string(indent, ' ')
      << "using LiteralPtr = std::unique_ptr<Literal>;" << std::endl;
  } else if (fname == "Stmt") {
    f << std::string(indent, ' ') << "using clox::Expr::ExprPtr;" << std::endl;
    f << std::endl;
    f << std::string(indent, ' ') << "class Stmt; //fwd declare" << std::endl;
    f << std::string(indent, ' ') << "using StmtPtr = std::unique_ptr<Stmt>;"
      << std::endl;
  }

  f << std::endl;

  f << std::string(indent, ' ') << "class " << fname << std::endl;
  f << std::string(indent, ' ') << "{" << std::endl;
  indent = 8;
  f << std::string(indent, ' ') << "public:" << std::endl;
  indent = 12;
  f << std::string(indent, ' ')
    << " virtual void accept(" + fname + "Visitor& visitor) const = 0;"
    << std::endl;
  f << std::string(indent, ' ')
    << "//TODO: figure out why specifiying other special functions breaks "
       "(copy constructor etc)."
    << std::endl;
  f << std::string(indent, ' ') << "explicit " + fname + "() = default;"
    << std::endl;
  f << std::string(indent, ' ') << "virtual ~" + fname + "() = default;"
    << std::endl;
  indent = 4;
  f << std::string(indent, ' ') << "};" << std::endl;

  for (auto& type : all_types) {
    std::string type_name{type[0] + fname};
    f << std::string(indent, ' ') << "class " << type_name << " : "
      << "public " << fname << std::endl;
    f << std::string(indent, ' ') << "{" << std::endl;
    indent = 8;
    f << std::string(indent, ' ') << "public:" << std::endl;

    indent = 12;
    // constructor declaration
    f << std::string(indent, ' ') << "explicit " << type_name << "(";
    for (size_t i = 1; i < type.size(); ++i) {
      f << type[i];
      if (i != type.size() - 1) {
        f << ", ";
      }
    }
    f << ")";

    // constructor body
    f << " : ";
    for (size_t i = 1; i < type.size(); ++i) {
      std::string var_name{split_str(" ", type[i]).back()};
      f << var_name << "(std::move(" << var_name << "))";
      if (i != type.size() - 1) {
        f << ", ";
      }
    }
    f << "{}" << std::endl;

    if (type_name == "LiteralExpr") {
      f << std::string(indent, ' ')
        << "// TODO: clox::Ast::LiteralExpr lit_expr{ \"foo\" }; works"
        << std::endl;
      f << std::string(indent, ' ')
        << "// but LiteralExpr takes Literal argument -> what conversion "
           "happens here? "
        << std::endl;
      f << std::string(indent, ' ')
        << "// especially since replacing const Literal& ctor with string "
           "breaks the above"
        << std::endl;
    }

    // functions
    f << std::string(indent, ' ')
      << "void accept(" + fname + "Visitor& visitor) const override;"
      << std::endl;
    f << std::endl;

    // fields
    for (size_t i = 1; i < type.size(); ++i) {
      f << std::string(indent, ' ') << type[i] << ";" << std::endl;
    }

    indent = 4;
    // class end
    f << std::string(indent, ' ') << "};" << std::endl;
    f << std::endl;
  }

  f << "} //clox::" << fname << std::endl;
  f.close();
}

void generate_cpp(std::string fpath, std::string fname,
                  std::vector<std::vector<std::string>> all_types) {
  std::ofstream f;
  f.open(fpath);
  f << "#pragma once" << std::endl;
  f << "#include \"" + fname + "Visitor.h\"" << std::endl;
  f << std::endl;
  f << "namespace clox::" << fname << std::endl;
  f << "{" << std::endl;
  int indent{4};

  for (auto& type : all_types) {
    std::string type_name{type[0] + fname};
    std::string method_name{type[0]};
    f << std::string(indent, ' ') << "void " << type_name
      << "::accept(" + fname + "Visitor& visitor) const" << std::endl;
    f << std::string(indent, ' ') << "{" << std::endl;
    indent = 8;
    f << std::string(indent, ' ') << "visitor.visit" << method_name
      << "(*this);" << std::endl;
    indent = 4;
    f << std::string(indent, ' ') << "}" << std::endl;
  }

  f << "} //clox::" << fname;
  f.close();
}

void generate_visitor(std::string fpath, std::string fname,
                      std::vector<std::vector<std::string>> all_types) {
  int indent{4};
  std::ofstream f;
  f.open(fpath);
  f << "#pragma once" << std::endl;
  f << "#include \"" + fname + ".h\"" << std::endl;
  f << std::endl;
  f << "namespace clox::" << fname << std::endl;
  f << "{" << std::endl;

  f << std::string(indent, ' ') << "class " + fname + "Visitor" << std::endl;
  f << std::string(indent, ' ') << "{" << std::endl;
  indent = 8;
  f << std::string(indent, ' ') << "public:" << std::endl;
  indent = 12;
  for (auto& type : all_types) {
    std::string var_name{to_lowercase(type[0])};
    std::string type_name{type[0] + fname};
    if (var_name == "if" || var_name == "while" || var_name == "return") {
      var_name = var_name + "_";
    }
    f << std::string(indent, ' ') << "virtual void visit" << type[0]
      << "(const " << type_name << "& " << var_name << ") = 0;" << std::endl;
  }
  f << std::endl;
  f << std::string(indent, ' ') << "//base class boilderplate" << std::endl;
  f << std::string(indent, ' ') << "explicit " + fname + "Visitor() = default;"
    << std::endl;
  f << std::string(indent, ' ') << "virtual ~" + fname + "Visitor() = default;"
    << std::endl;
  f << std::string(indent, ' ')
    << fname + "Visitor(const " + fname + "Visitor&) = delete;" << std::endl;
  f << std::string(indent, ' ')
    << fname + "Visitor& operator=(const " + fname + "Visitor&) = delete;"
    << std::endl;
  f << std::string(indent, ' ')
    << fname + "Visitor(" + fname + "Visitor&&) = delete;" << std::endl;
  f << std::string(indent, ' ')
    << fname + "Visitor operator=(" + fname + "Visitor&&) = delete;"
    << std::endl;

  indent = 8;
  f << std::string(indent, ' ') << "};" << std::endl;
  indent = 0;
  f << "} //clox::" << fname << std::endl;
  f.close();
}
