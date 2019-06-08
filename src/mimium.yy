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
  #include "ast.hpp"
  #define YYDEBUG 1

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

   AT "@"
   
   END    0     "end of file"
   NEWLINE "newline"
;
%token <int> NUM "number"
%type  <AST_Ptr> expr "expression"
%type <AST_Ptr> term_time "term @ something"
%type <AST_Ptr> term "primary"
%type <AST_Ptr> top "top"
%type <AST_Ptr> statements "statements"


%locations


%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD

%left  AT

%right NOT 

%start top

%%

top :statements END {$$=std::move($1);}
    ;

statements : expr {driver.add_line(std::move($1));}
      | expr NEWLINE statements {driver.add_line(std::move($1));}
      ;

expr : expr ADD    expr  {$$ = driver.add_op("+" , std::move($1),std::move($3));}
     | expr SUB    expr  {$$ = driver.add_op("-" , std::move($1),std::move($3));}
     | expr MUL    expr  {$$ = driver.add_op("*" , std::move($1),std::move($3));}
     | expr DIV    expr  {$$ = driver.add_op("/" , std::move($1),std::move($3));}
     | expr MOD    expr  {$$ = driver.add_op("%" , std::move($1),std::move($3));}
     | expr EXPONENT expr{$$ = driver.add_op("^" , std::move($1),std::move($3));}
     | expr OR     expr  {$$ = driver.add_op("|" , std::move($1),std::move($3));}
     | expr AND    expr  {$$ = driver.add_op("&" , std::move($1),std::move($3));}
     | expr BITOR  expr  {$$ = driver.add_op("||", std::move($1),std::move($3));}
     | expr BITAND expr  {$$ = driver.add_op("&&", std::move($1),std::move($3));}
     | term_time {$$ = std::move($1);};

term_time : term AT NUM {$$ = driver.set_time(std::move($1),std::move($3));}
         | term {$$ = std::move($1);}
         ;
term : NUM {$$ = driver.add_number($1);}
        | '(' expr ')' {$$ =std::move($2);};

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
   std::cerr << "Error: " << err_message << " at " << l << "\n";
}