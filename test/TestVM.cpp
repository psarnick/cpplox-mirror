#include <iostream>
#include <memory>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "cpplox/Bytecode/ByteCodeRunner.h"
#include "cpplox/Bytecode/VM.h"
#include "cpplox/Treewalk/Parser.h"
#include "gtest/gtest.h"

namespace vm {
// TODO: in tests - should NUMBER tokentypes be passed as numbers not strings?
// TODO: how to better enforce this?
// TODO: Remove literals where scanner wouldn't produce those. Better still
//       use the same method to create tokens.
using clox::ByteCodeRunner;
using clox::Types::Token;
using clox::Types::TokenType;
using test_param = std::string;
class TestVMFixture : public ::testing::TestWithParam<test_param> {
 protected:
  std::ostringstream oss;
  ByteCodeRunner r{oss};
  const std::string tests_path_prefix = "/Users/psarnick/dev/cpplox/test/";
  // TODO: Take from env variable
  const std::unordered_set<std::string> tests_with_empty_output = {
      "comments/only_line_comment.lox",
      "comments/only_line_comment_and_line.lox"};
};

std::string get_expectation(std::string fpath) {
  std::string delimiter{"/ expect: "};
  std::ifstream ifs(fpath);
  std::string line{};
  if (!ifs.good()) {
    throw std::runtime_error("File not found: " + fpath);
  }

  std::vector<std::string> output{};
  while (std::getline(ifs, line)) {
    size_t pos = line.find(delimiter);
    if (pos != std::string::npos) {
      std::string s{line.substr(pos + delimiter.size(), line.size()) +
                    std::string{'\n'}};
      output.push_back(s);
    }
  }
  return std::accumulate(output.begin(), output.end(), std::string{});
}

TEST_P(TestVMFixture, EndToEnd) {
  std::string script_path{tests_path_prefix + GetParam()};
  std::string expect{get_expectation(script_path)};
  r.runFile(script_path);

  if (!tests_with_empty_output.contains(GetParam())) {
    if (expect.size() == 0 || oss.str().size() == 0) {
      throw std::runtime_error("Error running tests! Expect value: \"" +
                               expect + "\", oss value: " + oss.str() + "\"");
    }
  }
  ASSERT_EQ(oss.str(), expect);
}

INSTANTIATE_TEST_SUITE_P(
    E2eTestVM, TestVMFixture,
    ::testing::Values(
        "bool/equality.lox", "bool/not.lox", "comments/line_at_eof.lox",
        "comments/only_line_comment.lox",
        "comments/only_line_comment_and_line.lox", "comments/unicode.lox",
        "nil/literal.lox",
        /*"number/decimal_point_at_eof.lox", */  // add when methods supported
        "number/literals.lox",
        /*"number/leading_dot.lox", */   // add when classes
        /*"number/trailing_dot.lox", */  // add when classes
        "number/nan_equality.lox", "operator/add.lox",
        "operator/add_bool_nil.lox", "operator/add_bool_num.lox",
        "operator/add_bool_string.lox", "operator/add_nil_nil.lox",
        "operator/add_num_nil.lox", "operator/add_string_nil.lox",
        "operator/comparison.lox", "operator/divide.lox",
        "operator/divide_nonnum_num.lox", "operator/divide_num_nonnum.lox",
        "operator/equals.lox", "operator/greater_nonnum_num.lox",
        "operator/greater_num_nonnum.lox",
        "operator/greater_or_equal_nonnum_num.lox",
        "operator/greater_or_equal_num_nonnum.lox",
        "operator/less_nonnum_num.lox", "operator/less_num_nonnum.lox",
        "operator/less_or_equal_nonnum_num.lox",
        "operator/less_or_equal_num_nonnum.lox", "operator/multiply.lox",
        "operator/multiply_nonnum_num.lox", "operator/multiply_num_nonnum.lox",
        "operator/negate.lox", "operator/negate_nonnum.lox",
        "operator/not_equals.lox", "operator/subtract.lox",
        "operator/subtract_nonnum_num.lox", "operator/subtract_num_nonnum.lox",
        "precedence.lox", "print/missing_argument.lox", "string/literals.lox",
        "string/unterminated.lox", "unexpected_character.lox"
        /*"operator/equals_class.lox", */
        /*"operator/equals_method.lox",*/
        //"operator/not.lox"
        /*"operator/not_class.lox",*/
        ));

INSTANTIATE_TEST_SUITE_P(
    AssignmentTests, TestVMFixture,
    ::testing::Values("assignment/associativity.lox", "assignment/global.lox",
                      "assignment/grouping.lox",
                      "assignment/infix_operator.lox", "assignment/local.lox",
                      "assignment/prefix_operator.lox", "assignment/syntax.lox",
                      //"assignment/to_this.lox",
                      "assignment/undefined.lox"));

INSTANTIATE_TEST_SUITE_P(BlockTests, TestVMFixture,
                         ::testing::Values(
                             /*"block/empty.lox", */
                             "block/scope.lox",
                             "block/double_nested_sope.lox"));

/*
INSTANTIATE_TEST_SUITE_P(
    IfElseTests,
    TestVMFixture,
    ::testing::Values(
        "if/dangling_else.lox",
        "if/else.lox",
        "if/if.lox",
        "if/truth.lox",
        "if/var_in_else.lox",
        "if/var_in_then.lox"
    )
);


INSTANTIATE_TEST_SUITE_P(
    LogicalOperatorTests,
    TestVMFixture,
    ::testing::Values(
        "logical_operator/or.lox",
        "logical_operator/and_truth.lox",
        "logical_operator/and.lox",
        "logical_operator/or_truth.lox"
    )
);
*/
/*
INSTANTIATE_TEST_SUITE_P(
    WhileTests,
    TestVMFixture,
    ::testing::Values(
        /"while/class_in_body.lox",
        "while/fun_in_body.lox",
        "while/closure_in_body.lox",
        "while/syntax.lox",
        "while/return_inside.lox"
        "while/return_closure.lox",
        "while/var_in_body.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    ForLoopTests,
    TestVMFixture,
    ::testing::Values(
        /"for/class_in_body.lox",
        "for/fun_in_body.lox",
        "for/closure_in_body.lox",
        "for/scope.lox"
        "for/return_inside.lox",
        "for/return_closure.lox",
        /"for/statement_initializer.lox", in principle works, but tests do not
handle multiline output "for/statement_increment.lox",
        /"for/statement_condition.lox", / same as statement_initializer.lox
        "for/var_in_body.lox",
        "for/syntax.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    FunctionTests,
    TestVMFixture,
    ::testing::Values(
        "function/add.lox", / my test - may have to remove
        "function/extra_arguments.lox",
        "function/empty_body.lox",
        "function/body_must_be_block.lox",
        "function/missing_arguments.lox",
        "function/local_recursion.lox",
        "function/local_mutual_recursion.lox",
        "function/nested_call_with_arguments.lox",
        "function/mutual_recursion.lox",
        "function/missing_comma_in_parameters.lox",
        "function/print.lox"
        "function/parameters.lox",
        "function/too_many_parameters.lox",
        "function/too_many_arguments.lox",
        "function/recursion.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    ReturnnTests,
    TestVMFixture,
    ::testing::Values(
        "return/after_if.lox",
        "return/after_else.lox",
        "return/at_top_level.lox",
        "return/after_while.lox",
        "return/return_nil_if_no_value.lox",
        /"return/in_method.lox",
        "return/in_function.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    ClosureTests,
    TestVMFixture,
    ::testing::Values(
        "closure/assign_to_closure.lox",
        "closure/close_over_function_parameter.lox",
        "closure/assign_to_shadowed_later.lox",
        "closure/closed_closure_in_function.lox",
        /"closure/close_over_method_parameter.lox",
        "closure/close_over_later_variable.lox",
        "closure/reference_closure_multiple_times.lox",
        "closure/open_closure_in_function.lox",
        "closure/nested_closure.lox",
        "closure/shadow_closure_with_local.lox",
        "closure/reuse_closure_slot.lox",
        "closure/unused_later_closure.lox",
        "closure/unused_closure.lox"
    )
);*/
}  // namespace vm