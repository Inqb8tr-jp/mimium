#pragma once 
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
 #include <iostream>

enum AST_ID{
    BASE,
    NUMBER,
    FCALL,
    ASSIGN,
    FDEF,
    ARRAYINIT,
    LAMBDA,
    OP,
    LIST
};


namespace mimium{
    const std::map<std::string,AST_ID> ast_id = {
        {"fcall",FCALL},
        {"define",ASSIGN},
        {"fdef",FDEF},
        {"array",ARRAYINIT},
        {"lambda",LAMBDA}
    };
    static AST_ID str_to_id(std::string str){
        auto id = mimium::ast_id.find(str);
                if(id!=std::end(mimium::ast_id)){
                        return id->second;
                }else{
                        return NUMBER;
                }
    };
};

class AST{
    public:
    AST_ID id;
    AST()=default;
    AST(std::string& OPERATOR,std::shared_ptr<AST> LHS, std::shared_ptr<AST> RHS){};
    virtual std::string to_string() = 0;
    AST_ID getid(){return id;}
    void set_time(int t){time = t;}
    int get_time(){return time;}
    bool istimeset(){return (time>=0);}
    int time = -1;
    private:
    
};
using AST_Ptr = std::shared_ptr<AST>;

class OpAST : public AST,public std::enable_shared_from_this<OpAST>{
    public:
    AST_Ptr lhs,rhs;
    std::string op;
    OpAST(std::string& OPERATOR,AST_Ptr LHS, AST_Ptr RHS){
        id=OP;
        lhs = LHS;
        rhs = RHS;
        op = OPERATOR;
    }
    std::string to_string();
};
class ListAST : AST{
    public:
    std::vector<AST_Ptr> asts;
    ListAST(){
        id = LIST;
    }
    void addAST(AST_Ptr ast){
        asts.push_back(ast);
    }
    std::string to_string();
};
class NumberAST :  public AST,public std::enable_shared_from_this<NumberAST>{
    public:
    int val;
    NumberAST(int input){
        id=NUMBER;
        val = input;
    }
    std::string to_string();
};