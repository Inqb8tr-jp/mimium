#include "basic/ast_to_string.hpp"
#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/ast_loader_new.hpp"
#include "compiler/knormalize_visitor.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#define PREP(FILENAME)                                           \
  Driver driver{};                                               \
  newast::Statements& ast = *driver.parseFile(#FILENAME ".mmm"); \
  SymbolRenamer renamer;                                         \
  auto newast = renamer.rename(ast);                             \
  TypeInferer inferer;                                           \
  auto& env = inferer.infer(*newast);                            \
  MirGenerator mirgenerator(env);

TEST(mirgen, basic) {
  PREP(test_localvar)
  auto mir = mirgenerator.generate(*newast);
  std::string target =
      "root:\n"
      "  hoge0 = fun x1 , y2\n"
      "    hoge0:\n"
      "      alloca: localvar3_ptr (float*)\n"
      "      localvar3 = 2.000000\n"
      "      k2 = Mul x1 y2\n"
      "      k3 = Add k2 localvar3\n"
      "      return k3\n"
      "\n"
      "  k6 = 5.000000\n"
      "  k7 = 7.000000\n"
      "  alloca: main4_ptr (float*)\n"
      "  main4 = appcls hoge0 k6 , k7\n";
  EXPECT_EQ(mir->toString(), target);
}