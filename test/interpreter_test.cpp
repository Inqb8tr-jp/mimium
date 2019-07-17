#define MIMIUM_DEBUG

#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#include "driver.hpp"

#include "interpreter.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::Interpreter interpreter;
TEST(interpreter_test, assign) {
     interpreter.init();
     interpreter.loadSource("a = 1");
     EXPECT_EQ(mimium::Interpreter::to_string(interpreter.getMainAst()),"(assign a 1)") ;
}
TEST(interpreter_test, assign2) {
     interpreter.clear();
     interpreter.loadSource("main = 1.245");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,1.245);
}
TEST(interpreter_test, assignexpr) {
     interpreter.clear();
     interpreter.loadSource("main = 1+2.3/2.5*(2-1)+100");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,101.92);
     } 

TEST(interpreter_test, assignfunction) {
     interpreter.clear();
     interpreter.loadSource("fn hoge(a,b){return a*b+1} \n main = hoge(7,5)");   
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,36);
} 

TEST(interpreter_test, assignfunction_block) {
     interpreter.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
     interpreter.clear();
     interpreter.loadSourceFile("testfile_statements.mmm");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,8);
     } 
TEST(interpreter_test, localvar) {
     interpreter.clear();
     interpreter.loadSourceFile("test_localvar.mmm");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,37);
     } 
TEST(interpreter_test, localvar_nestfun) {
     interpreter.clear();
     interpreter.loadSourceFile("test_localvar2.mmm");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,3.5);
}
TEST(interpreter_test, closure_countup) {
     interpreter.clear();
     interpreter.loadSourceFile("test_closure.mmm");
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,3);
}
TEST(interpreter_test, builtin_print) {
     interpreter.clear();
     interpreter.loadSource("main=println(1)");   
     mValue main = interpreter.findVariable("main");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,0);
} 

TEST(interpreter_test, ifstatement) {
     interpreter.clear();
     interpreter.loadSourceFile("test_if.mmm");
     mValue main = interpreter.findVariable("true");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,5);
     mValue main2 = interpreter.findVariable("false");
     double resv2 = mimium::Interpreter::get_as_double(main2);
     EXPECT_EQ(resv2,100);
} 
TEST(interpreter_test, ifstatement2) {
     interpreter.clear();
     interpreter.loadSourceFile("test_if_nested.mmm");
     mValue main = interpreter.findVariable("zero");
     double resv = mimium::Interpreter::get_as_double(main);
     EXPECT_EQ(resv,0);
     mValue main2 = interpreter.findVariable("hand");
     double resv2 = mimium::Interpreter::get_as_double(main2);
     EXPECT_EQ(resv2,100);
     mValue main3 = interpreter.findVariable("fivehand");
     double resv3 = mimium::Interpreter::get_as_double(main3);
     EXPECT_EQ(resv3,500);
} 
TEST(interpreter_test, array) {
     interpreter.clear();
     interpreter.loadSource("main = [1,2,3,4,5]");   
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ("[1,2,3,4,5]", mimium::Interpreter::to_string(main) );
} 
TEST(interpreter_test, arrayaccess) {
     interpreter.clear();
     interpreter.loadSource("arr = [1,2,3,4,5]\n\
                             main = arr[2]");   
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ(3, mimium::Interpreter::get_as_double(main) );
} 
TEST(interpreter_test, forloop) {
     interpreter.clear();
     interpreter.loadSource("arr = [1,2,3,4,5]\n\
                         main=0\n\
                         for i in arr {\n\
                              main = main+i  \n\
                         }");
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ(15, mimium::Interpreter::get_as_double(main));
} 


TEST(interpreter_test, factorial) {
     interpreter.clear();
     interpreter.loadSourceFile("factorial.mmm");
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ(120, mimium::Interpreter::get_as_double(main));
}

TEST(interpreter_test, fibonacchi) {
     interpreter.clear();
     interpreter.loadSourceFile("fibonacchi.mmm");
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ(610, mimium::Interpreter::get_as_double(main));
}
TEST(interpreter_test, include) {
     interpreter.clear();
     interpreter.loadSourceFile("test_include.mmm");
     mValue main = interpreter.findVariable("main");
     EXPECT_EQ(25+128, mimium::Interpreter::get_as_double(main));
}
