%skeleton "lalr1.cc"
%require "3.0"
%debug 

%defines
%define api.parser.class {MimiumParser}
%define api.namespace{mmmpsr}


%{
#define YYDEBUG 1
#define YYERROR_VERBOSE 1


%}

%code requires{
   namespace mmmpsr {
      class MimiumDriver;
      class MimiumScanner;
   }
  #include <memory>
  #include "ast_definitions.hpp"
  #define YYDEBUG 1
  using AST_Ptr = std::shared_ptr<AST>;

}
%parse-param { MimiumScanner &scanner  }
%parse-param { MimiumDriver  &driver  }

%code {
    #include "driver.hpp"

  #undef yylex
  #define yylex scanner.yylex
}
%define api.value.type variant
%define parse.assert
%token
    ADD "+"
    SUB "-"
    MOD "%"
    MUL "*"
    DIV "/"
    EXPONENT "^"
    AND "&"
    OR "|"
    BITAND "&&"
    BITOR "||"
    NEQ "!="
    EQ "=="
    NOT "!"
    END    0     "end of file"
;
%token <int> NUM "number"
%type  <AST_Ptr> expr "expression"
%type <AST_Ptr> primary

%locations


%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD
%right NOT 

%%

expr : expr ADD expr  {$$ = driver.add_op("+",$1,$3);}
    /* | expr SUB expr  {$$ = std::make_shared<OpAST>("-", $1, $3);}
     | expr MUL expr  {$$ = std::make_shared<OpAST>("*", $1, $3);}
     | expr DIV expr  {$$ = std::make_shared<OpAST>("/", $1, $3);}
     | expr MOD expr  {$$ = std::make_shared<OpAST>("%", $1, $3);}
     | expr EXPONENT expr  {$$ = std::make_shared<OpAST>("^", $1, $3);}
     | expr OR expr  {$$ = std::make_shared<OpAST>("|", $1, $3);}
     | expr AND expr  {$$ = std::make_shared<OpAST>("&", $1, $3);}
     | expr BITOR expr  {$$ = std::make_shared<OpAST>("||", $1, $3);}
     | expr BITAND expr  {$$ = std::make_shared<OpAST>("&&", $1, $3);} */
     | primary;

primary : NUM {$$ = driver.add_number($1);}
        | '(' expr ')' {$$ =$2;};

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
   std::cerr << "Error: " << err_message << " at " << l << "\n";
}