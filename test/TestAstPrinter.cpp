#include "cpplox/Treewalk/AstPrinter.h"
#include "cpplox/Treewalk/ExprFactory.h"
#include "gtest/gtest.h"

namespace clox {
using namespace clox::Expr;
using namespace clox::ExprFactory;

TEST(AstPrinterTests, test_binary_with_both_literal) {
  ExprPtr bin{make_binary_expr(make_literal_expr("left"),
                               {TokenType::PLUS, "+", 0},
                               make_literal_expr("right"))};

  AstPrinter printer{};
  bin->accept(printer);

  std::string result{printer.result()};
  ASSERT_EQ(result, "(+ left right)");
};

TEST(AstPrinterTests, test_nested_grouping) {
  ExprPtr second_grouping{
      make_grouping_expr(make_grouping_expr(make_literal_expr("1")))};
  AstPrinter printer{};
  second_grouping->accept(printer);

  std::string result{printer.result()};
  ASSERT_EQ(result, "(group (group 1))");
};

TEST(AstPrinterTests, test_nested_binary) {
  ExprPtr bin{make_binary_expr(
      make_literal_expr("1"), {TokenType::PLUS, "+", 0},
      make_binary_expr(make_literal_expr("1"), {TokenType::STAR, "*", 0},
                       make_literal_expr("2")))};

  AstPrinter printer{};
  bin->accept(printer);

  std::string result{printer.result()};

  // ASSERT_EQ(result, "1+1*2");
  ASSERT_EQ(result, "(+ 1 (* 1 2))");
};

TEST(AstPrinterTests, test_negative_unary_grouping) {
  ExprPtr unary_expr{make_unary_expr(
      {TokenType::MINUS, "-", 0},
      make_grouping_expr(make_grouping_expr(make_literal_expr("1"))))};

  AstPrinter printer{};
  unary_expr->accept(printer);

  std::string result{printer.result()};
  // ASSERT_EQ(result, "-((1))");
  ASSERT_EQ(result, "(- (group (group 1)))");
};

TEST(AstPrinterTests, test_compound_expression) {
  ExprPtr bin{make_binary_expr(
      make_grouping_expr(make_grouping_expr(
          make_unary_expr({TokenType::MINUS, "-", 0}, make_literal_expr("1")))),
      {TokenType::MINUS, "-", 0},
      make_grouping_expr(make_binary_expr(make_literal_expr("1"),
                                          {TokenType::STAR, "*", 0},
                                          make_literal_expr("2"))))};

  AstPrinter printer{};
  bin->accept(printer);

  std::string result{printer.result()};
  // ASSERT_EQ(result, "((-1))-(1*2)");
  ASSERT_EQ(result, "(- (group (group (- 1))) (group (* 1 2)))");
};
}  // namespace clox