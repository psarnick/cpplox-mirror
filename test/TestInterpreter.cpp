#include <cassert>
#include <iostream>
#include <memory>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "cpplox/Treewalk/Interpreter.h"
#include "cpplox/Treewalk/Parser.h"
#include "cpplox/Treewalk/Runner.h"
#include "gtest/gtest.h"

namespace clox {
// TODO: in tests - should NUMBER tokentypes be passed as numbers not strings?
// TODO: how to better enforce this?
// TODO: Remove literals where scanner wouldn't produce those. Better still
//       use the same method to create tokens.
using namespace Expr;
using clox::Interpreter;
using clox::Runner;
using clox::RuntimeException;
using clox::Types::Token;
using clox::Types::TokenType;
using ErrorsAndDebug::ErrorReporter;

using test_param = std::string;
class TestInterpreterFixture : public ::testing::TestWithParam<test_param> {
 protected:
  std::ostringstream oss;
  Runner r{oss};
  const std::string tests_path_prefix = "/Users/psarnick/dev/cpplox/test/";
  // TODO: Take from env variable
  const std::unordered_set<std::string> tests_with_empty_output = {
      "comments/only_line_comment.lox",
      "comments/only_line_comment_and_line.lox"};
};

std::string get_expectation(std::string fpath) {
  std::string delimiter{"// expect: "};
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

TEST_P(TestInterpreterFixture, EndToEnd) {
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

/*
INSTANTIATE_TEST_SUITE_P(
    BenchmarkTests,
    TestInterpreterFixture,
    ::testing::Values(
      "benchmark/equality.lox",
      "benchmark/binary_trees.lox",
      "benchmark/properties.lox",
      "benchmark/invocation.lox",
      "benchmark/fib.lox",
      "benchmark/trees.lox",
      "benchmark/string_equality.lox",
      "benchmark/instantiation.lox",
      "benchmark/zoo_batch.lox",
      "benchmark/method_call.lox",
      "benchmark/zoo.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    ClosureTests, TestInterpreterFixture,
    ::testing::Values("closure/reuse_closure_slot.lox",
                      "closure/assign_to_shadowed_later.lox",
                      "closure/close_over_later_variable.lox",
                      "closure/closed_closure_in_function.lox",
                      "closure/unused_later_closure.lox",
                      "closure/shadow_closure_with_local.lox",
                      "closure/unused_closure.lox",
                      "closure/close_over_function_parameter.lox",
                      // "closure/close_over_method_parameter.lox",
                      // Needs class
                      "closure/open_closure_in_function.lox",
                      "closure/reference_closure_multiple_times.lox",
                      "closure/nested_closure.lox",
                      "closure/assign_to_closure.lox"));

INSTANTIATE_TEST_SUITE_P(
    CommentsTests, TestInterpreterFixture,
    ::testing::Values("comments/line_at_eof.lox",
                      "comments/only_line_comment.lox", "comments/unicode.lox",
                      "comments/only_line_comment_and_line.lox"));

INSTANTIATE_TEST_SUITE_P(LimitTests, TestInterpreterFixture,
                         ::testing::Values(
                             // "limit/too_many_constants.lox",
                             // Only VM
                             // "limit/no_reuse_constants.lox",
                             // Needs pooling constants
                             // "limit/too_many_upvalues.lox",
                             // Revisit when closure done
                             // "limit/stack_overflow.lox",
                             // Revisit when functions done
                             // "limit/too_many_locals.lox",
                             // Revisit when functions done
                             // "limit/loop_too_large.lox"
                             // Revisit when while done
                             ));

INSTANTIATE_TEST_SUITE_P(
    VariableTests, TestInterpreterFixture,
    ::testing::Values(
        "variable/in_nested_block.lox",
        "variable/scope_reuse_in_different_blocks.lox",
        // "variable/local_from_method.lox",
        // Needs class
        // "variable/use_global_in_initializer.lox",
        // Needs class
        // "variable/use_this_as_var.lox",
        // Revisit at end
        "variable/redeclare_global.lox",
        // "variable/use_nil_as_var.lox",
        // Revisit at end
        "variable/undefined_global.lox", "variable/shadow_and_local.lox",
        "variable/early_bound.lox",
        // "variable/duplicate_parameter.lox",
        // Revisit after functions done
        "variable/uninitialized.lox",
        // "variable/use_false_as_var.lox",
        // Revisit at end
        "variable/shadow_global.lox", "variable/duplicate_local.lox",
        "variable/in_middle_of_block.lox", "variable/shadow_local.lox",
        "variable/unreached_undefined.lox",
        // "variable/collide_with_parameter.lox",
        // Revisit when functions done
        "variable/use_local_in_initializer.lox", "variable/redefine_global.lox",
        "variable/undefined_local.lox"));

INSTANTIATE_TEST_SUITE_P(NilTests, TestInterpreterFixture,
                         ::testing::Values("nil/literal.lox"));

INSTANTIATE_TEST_SUITE_P(IfTests, TestInterpreterFixture,
                         ::testing::Values("if/var_in_then.lox",
                                           "if/dangling_else.lox",
                                           "if/truth.lox", "if/fun_in_else.lox",
                                           "if/class_in_else.lox",
                                           "if/else.lox", "if/fun_in_then.lox",
                                           "if/class_in_then.lox",
                                           "if/var_in_else.lox", "if/if.lox"));

INSTANTIATE_TEST_SUITE_P(
    AssignmentTests, TestInterpreterFixture,
    ::testing::Values("assignment/grouping.lox", "assignment/syntax.lox",
                      "assignment/global.lox", "assignment/prefix_operator.lox",
                      "assignment/associativity.lox", "assignment/to_this.lox",
                      "assignment/infix_operator.lox", "assignment/local.lox",
                      "assignment/undefined.lox"));

INSTANTIATE_TEST_SUITE_P(ReturnTests, TestInterpreterFixture,
                         ::testing::Values("return/after_if.lox",
                                           "return/after_else.lox",
                                           "return/at_top_level.lox",
                                           "return/return_nil_if_no_value.lox",
                                           "return/in_method.lox",
                                           "return/in_function.lox",
                                           "return/after_while.lox"));

INSTANTIATE_TEST_SUITE_P(
    FunctionTests, TestInterpreterFixture,
    ::testing::Values(
        "function/local_mutual_recursion.lox", "function/empty_body.lox",
        "function/too_many_arguments.lox", "function/add.lox",
        "function/missing_comma_in_parameters.lox",
        "function/nested_call_with_arguments.lox",
        "function/body_must_be_block.lox", "function/missing_arguments.lox",
        "function/parameters.lox", "function/local_recursion.lox",
        "function/recursion.lox", "function/print.lox",
        "function/too_many_parameters.lox", "function/mutual_recursion.lox",
        "function/extra_arguments.lox"));

INSTANTIATE_TEST_SUITE_P(
    ScanningTests, TestInterpreterFixture,
    ::testing::Values("scanning/numbers.lox", "scanning/keywords.lox",
                      "scanning/punctuators.lox", "scanning/whitespace.lox",
                      "scanning/identifiers.lox", "scanning/strings.lox"));

INSTANTIATE_TEST_SUITE_P(
    FieldTests,
    TestInterpreterFixture,
    ::testing::Values(
      "field/set_on_nil.lox",
      "field/get_on_string.lox",
      "field/many.lox",
      "field/set_on_function.lox",
      "field/set_on_bool.lox",
      "field/method.lox",
      "field/call_nonfunction_field.lox",
      "field/get_on_nil.lox",
      "field/set_on_class.lox",
      "field/set_on_string.lox",
      "field/on_instance.lox",
      "field/get_on_function.lox",
      "field/call_function_field.lox",
      "field/set_evaluation_order.lox",
      "field/method_binds_this.lox",
      "field/set_on_num.lox",
      "field/get_on_class.lox",
      "field/get_and_set_method.lox",
      "field/get_on_bool.lox",
      "field/get_on_num.lox",
      "field/undefined.lox"
    )
);


INSTANTIATE_TEST_SUITE_P(PrintTests, TestInterpreterFixture,
                         ::testing::Values("print/missing_argument.lox"));

INSTANTIATE_TEST_SUITE_P(NumberTests, TestInterpreterFixture,
                         ::testing::Values("number/decimal_point_at_eof.lox",
                                           "number/nan_equality.lox",
                                           "number/literals.lox",
                                           "number/leading_dot.lox",
                                           "number/trailing_dot.lox"));

INSTANTIATE_TEST_SUITE_P(CallTests, TestInterpreterFixture,
                         ::testing::Values("call/nil.lox", "call/bool.lox",
                                           "call/num.lox", "call/object.lox",
                                           "call/string.lox"));

INSTANTIATE_TEST_SUITE_P(Logical_operatorTests, TestInterpreterFixture,
                         ::testing::Values("logical_operator/and.lox",
                                           "logical_operator/or.lox",
                                           "logical_operator/and_truth.lox",
                                           "logical_operator/or_truth.lox"));


INSTANTIATE_TEST_SUITE_P(
    InheritanceTests,
    TestInterpreterFixture,
    ::testing::Values(
      "inheritance/inherit_from_nil.lox",
      "inheritance/inherit_from_function.lox",
      "inheritance/parenthesized_superclass.lox",
      "inheritance/set_fields_from_base_class.lox",
      "inheritance/inherit_from_number.lox",
      "inheritance/inherit_methods.lox",
      "inheritance/constructor.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    SuperTests,
    TestInterpreterFixture,
    ::testing::Values(
      "super/no_superclass_method.lox",
      "super/call_same_method.lox",
      "super/no_superclass_call.lox",
      "super/no_superclass_bind.lox",
      "super/parenthesized.lox",
      "super/this_in_superclass_method.lox",
      "super/closure.lox",
      "super/super_in_top_level_function.lox",
      "super/call_other_method.lox",
      "super/missing_arguments.lox",
      "super/super_in_closure_in_inherited_method.lox",
      "super/super_in_inherited_method.lox",
      "super/super_without_dot.lox",
      "super/indirectly_inherited.lox",
      "super/super_at_top_level.lox",
      "super/super_without_name.lox",
      "super/extra_arguments.lox",
      "super/bound_method.lox",
      "super/constructor.lox",
      "super/reassign_superclass.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(AdhocTests, TestInterpreterFixture,
                         ::testing::Values("adhoc/scope.lox", "adhoc/fib.lox",
                                           "adhoc/for-return-closure.lox",
                                           "adhoc/nested_block.lox",
                                           "adhoc/closure.lox",
                                           "adhoc/make-counter-closure.lox",
                                           "adhoc/fun-call.lox"));

INSTANTIATE_TEST_SUITE_P(BoolTests, TestInterpreterFixture,
                         ::testing::Values("bool/equality.lox",
                                           "bool/not.lox"));

INSTANTIATE_TEST_SUITE_P(ExpressionsTests, TestInterpreterFixture,
                         ::testing::Values("expressions/evaluate.lox",
                                           "expressions/parse.lox"));

INSTANTIATE_TEST_SUITE_P(
    ForTests, TestInterpreterFixture,
    ::testing::Values("for/return_closure.lox", "for/scope.lox",
                      "for/var_in_body.lox", "for/syntax.lox",
                      "for/return_inside.lox", "for/statement_initializer.lox",
                      "for/statement_increment.lox",
                      "for/statement_condition.lox", "for/closure_in_body.lox",
                      "for/class_in_body.lox", "for/fun_in_body.lox"));

INSTANTIATE_TEST_SUITE_P(
    ClassTests,
    TestInterpreterFixture,
    ::testing::Values(
      "class/empty.lox",
      "class/local_inherit_self.lox",
      "class/local_inherit_other.lox",
      "class/inherited_method.lox",
      "class/reference_self.lox",
      "class/inherit_self.lox",
      "class/local_reference_self.lox"
    )
);

INSTANTIATE_TEST_SUITE_P(
    ThisTests,
    TestInterpreterFixture,
    ::testing::Values(
      "this/this_in_method.lox",
      "this/this_at_top_level.lox",
      "this/closure.lox",
      "this/this_in_top_level_function.lox",
      "this/nested_closure.lox",
      "this/nested_class.lox"
    )
);


INSTANTIATE_TEST_SUITE_P(StringTests, TestInterpreterFixture,
                         ::testing::Values("string/error_after_multiline.lox",
                                           "string/literals.lox",
                                           "string/multiline.lox",
                                           "string/unterminated.lox"));

INSTANTIATE_TEST_SUITE_P(RegressionTests, TestInterpreterFixture,
                         ::testing::Values("regression/40.lox",
                                           "regression/394.lox"));

INSTANTIATE_TEST_SUITE_P(
    WhileTests, TestInterpreterFixture,
    ::testing::Values("while/return_closure.lox", "while/var_in_body.lox",
                      "while/syntax.lox", "while/return_inside.lox",
                      "while/closure_in_body.lox", "while/class_in_body.lox",
                      "while/fun_in_body.lox"));

INSTANTIATE_TEST_SUITE_P(
    MethodTests,
    TestInterpreterFixture,
    ::testing::Values(
      "method/empty_block.lox",
      "method/arity.lox",
      "method/refer_to_name.lox",
      "method/too_many_arguments.lox",
      "method/print_bound_method.lox",
      "method/missing_arguments.lox",
      "method/not_found.lox",
      "method/too_many_parameters.lox",
      "method/extra_arguments.lox"
    )
);


INSTANTIATE_TEST_SUITE_P(
    OperatorTests, TestInterpreterFixture,
    ::testing::Values(
        "operator/add_num_nil.lox", "operator/equals_method.lox",
        "operator/equals_class.lox", "operator/subtract_num_nonnum.lox",
        "operator/multiply.lox", "operator/negate.lox",
        "operator/divide_nonnum_num.lox", "operator/comparison.lox",
        "operator/greater_num_nonnum.lox",
        "operator/less_or_equal_nonnum_num.lox",
        "operator/multiply_nonnum_num.lox", "operator/not_equals.lox",
        "operator/add_bool_num.lox", "operator/negate_nonnum.lox",
        "operator/add.lox", "operator/greater_or_equal_nonnum_num.lox",
        "operator/equals.lox", "operator/less_nonnum_num.lox",
        "operator/add_bool_string.lox", "operator/divide.lox",
        "operator/add_string_nil.lox", "operator/add_bool_nil.lox",
        "operator/divide_num_nonnum.lox", "operator/multiply_num_nonnum.lox",
        "operator/less_or_equal_num_nonnum.lox",
        "operator/greater_nonnum_num.lox", "operator/not.lox",
        "operator/add_nil_nil.lox", "operator/subtract.lox",
        "operator/subtract_nonnum_num.lox", "operator/not_class.lox",
        "operator/greater_or_equal_num_nonnum.lox",
        "operator/less_num_nonnum.lox"));

INSTANTIATE_TEST_SUITE_P(
    ConstructorTests,
    TestInterpreterFixture,
    ::testing::Values(
      "constructor/call_init_explicitly.lox",
      "constructor/return_value.lox",
      "constructor/init_not_method.lox",
      "constructor/missing_arguments.lox",
      "constructor/default.lox",
      "constructor/arguments.lox",
      "constructor/default_arguments.lox",
      "constructor/call_init_early_return.lox",
      "constructor/extra_arguments.lox",
      "constructor/return_in_nested_function.lox",
      "constructor/early_return.lox"
    )
);
*/

INSTANTIATE_TEST_SUITE_P(BlockTests, TestInterpreterFixture,
                         ::testing::Values("block/empty.lox", "block/scope.lox",
                                           "block/double_nested_sope.lox"));

}  // namespace clox