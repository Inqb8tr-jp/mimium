#include "gtest/gtest.h"
#include "../parser.hpp"

namespace tch{
    using s  = std::string;
    s num = "123";
    s str = "hoge";
    s exprnumnum = "1+2";
    s exprnumnumnum = "1+2+3";

    s exprnumstr = "1+symn";

};

// TEST(parser_test, digit) {
//     auto testparser = pc::parser(digit);
//     auto res = testparser.parse(tch::num).success().value();
//     auto res2 = testparser.parse(tch::str);
//     EXPECT_EQ('1', res);
//     EXPECT_FALSE(res2.is_success());
// }

TEST(parser_test, num) {
    auto testparser = pc::parser(num);
    auto res = testparser.parse(tch::num).success().value();
    auto res2 = testparser.parse(tch::str);
    EXPECT_EQ("123", res->to_string());
    EXPECT_FALSE(res2.is_success());

}

TEST(parser_test, symbol) {
    auto testparser = pc::parser(symbol);
    auto res = testparser.parse(tch::str).success().value();
    auto res2 = testparser.parse(tch::num);

    EXPECT_EQ("hoge", res->to_string());
    EXPECT_FALSE(res2.is_success());
}


TEST(parser_test, expr) {
    auto testparser = pc::parser(expr);
    auto res = testparser.parse(tch::exprnumnum);
    auto res2 = testparser.parse(tch::exprnumnumnum);

    // EXPECT_TRUE(res.is_success());
    EXPECT_EQ("(fcall,+,(1,2))",res.success().value()->to_string());
    // EXPECT_TRUE(res2.is_success());
    EXPECT_EQ("(fcall,+,((fcall,+,(1,2)),3))",res2.success().value()->to_string());


    
    // auto res2 = testparser.parse(tch::exprnumstr).success().value();

    // EXPECT_EQ("+", res2.getOp());
    // EXPECT_EQ(NumberID, res2.getLHS()->getValueID());
    // EXPECT_EQ(VariableID, res2.getRHS()->getValueID());
}
