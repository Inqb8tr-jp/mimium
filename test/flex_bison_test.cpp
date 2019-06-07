#include "gtest/gtest.h"
#include "driver.hpp"

TEST(bison_parser_test, expr) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "1+2+3*2/2";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((+ (+ 1 2) (/ (* 3 2) 2)) )");
}

TEST(bison_parser_test, parensis) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "(2+3)/5/4";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((/ (/ (+ 2 3) 5) 4) )");
}

TEST(bison_parser_test, lines) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "(2+3)/5/4 \n 3+5";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((+ 3 5) (/ (/ (+ 2 3) 5) 4) )");
}

TEST(bison_parser_test, time) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "(3+2)@50";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((+ 3 2)@50 )");
}


TEST(bison_parser_test, fileread) {
     mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile1.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((+ 2 3)@128 )");
}