#pragma once

#ifndef __MMMSCANNER_HPP__
#define __MMMSCANNER_HPP__ 1

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "location.hh"
#include "mimium_parser.hpp"

namespace mmmpsr {

class MimiumScanner : public yyFlexLexer {
 public:
  explicit MimiumScanner(std::istream &in)
      : yyFlexLexer(in, std::cout),
        loc(std::make_unique<mmmpsr::MimiumParser::location_type>()){

        };

   ~MimiumScanner()override = default;

  // get rid of override virtual function warning
  using FlexLexer::yylex;

  virtual int yylex(mmmpsr::MimiumParser::semantic_type * lval,
                    mmmpsr::MimiumParser::location_type *location);
  // YY_DECL defined in mc_lexer.l
  // Method body created by flex in mc_lexer.yy.cc

 private:
  /* yyval ptr */
  mmmpsr::MimiumParser::semantic_type *yylval = nullptr;
  std::shared_ptr<mmmpsr::MimiumParser::location_type> loc = nullptr;
};

}  // namespace mmmpsr

#endif /* END __MMMSCANNER_HPP__ */
