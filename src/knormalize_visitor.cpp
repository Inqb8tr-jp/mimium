#include "knormalize_visitor.hpp"

namespace mimium {

KNormalizeVisitor::KNormalizeVisitor(){
  init();
}

// KNormalizeVisitor::~KNormalizeVisitor(){

// }
void KNormalizeVisitor::init(){
  var_counter=1;
  current_context=nullptr;
  while (!res_stack.empty()) {
    res_stack.pop(); // make stack empty
  }
}
AST_Ptr KNormalizeVisitor::insertAssign(AST_Ptr ast) {
  if (ast->getid() == OP) {
    auto tmpsymbol =
        std::make_shared<SymbolAST>("tmp" + std::to_string(var_counter));
    auto assign = std::make_unique<AssignAST>(tmpsymbol, ast);  // recursive!!
    var_counter++;
    current_context->appendAST(std::move(assign));
    return std::move(tmpsymbol);
  } else {
    return ast;
  }
}

void KNormalizeVisitor::visit(ListAST& ast) {
    auto tempctx = current_context;
  current_context = std::make_shared<ListAST>();
  for (auto& elem : ast.getlist()) {
    elem->accept(*this);
  }
  // //res_stack.push(current_context);
  // if(tempctx!=nullptr){//if top level, keep currentcontext to return
  //   current_context = tempctx;//switch back ctx
  // }
}

void KNormalizeVisitor::visit(OpAST& ast) {
  auto newrhs = insertAssign(ast.rhs);
  auto newlhs = insertAssign(ast.lhs);
  newrhs->accept(*this);
  newlhs->accept(*this);  // recursively visit
  auto res = std::make_unique<OpAST>(ast.op, stack_pop(), stack_pop()); //getting op string in dirty way
  res_stack.push(std::move(res));
}

void KNormalizeVisitor::visit(AssignAST& ast) {
  ast.getBody()->accept(*this);  // result is stored into res_stack
  auto newast = std::make_unique<AssignAST>(ast.getName(), stack_pop());  // copy
  current_context->appendAST(std::move(newast));
}


void KNormalizeVisitor::visit(NumberAST& ast){
    exprVisit(ast);
}
void KNormalizeVisitor::visit(SymbolAST& ast){
    exprVisit(ast);
}
void KNormalizeVisitor::visit(LambdaAST& ast){
    ast.getBody()->accept(*this);  // result is stored into tmp_ptr;
    auto newbody = stack_pop();
    auto newast = std::make_unique<LambdaAST>( ast.getArgs(),std::move(newbody));  res_stack.push(std::move(newast));
}
void KNormalizeVisitor::visit(FcallAST& ast){
    exprVisit(ast);
};
void KNormalizeVisitor::visit(ArgumentsAST& ast){
//this won't be used?
}
void KNormalizeVisitor::visit(ArrayAST& ast){
    exprVisit(ast);
}
void KNormalizeVisitor::visit(ArrayAccessAST& ast){//access index may be expr
    ast.getIndex()->accept(*this);
    auto newast = std::make_unique<ArrayAccessAST>(ast.getName(),stack_pop());
    res_stack.push(std::move(newast));
}
void KNormalizeVisitor::visit(IfAST& ast){
    ast.getElse()->accept(*this);
    ast.getThen()->accept(*this);
    auto newast = std::make_unique<IfAST>(ast.getCond(),stack_pop(),stack_pop());
    current_context->appendAST(std::move(newast));
}
void KNormalizeVisitor::visit(ReturnAST& ast){
    ast.getExpr()->accept(*this);
    auto newast  = std::make_unique<ReturnAST>(stack_pop());
    current_context->appendAST(std::move(newast));
}
void KNormalizeVisitor::visit(ForAST& ast){
    ast.getExpression()->accept(*this);
    auto newast = std::make_unique<ForAST>(ast.getVar(),ast.getIterator(),stack_pop());
    current_context->appendAST(std::move(newast));
}
void KNormalizeVisitor::visit(DeclarationAST& ast){
    current_context->appendAST(std::make_unique<DeclarationAST>(ast));
}
void KNormalizeVisitor::visit(TimeAST& ast){
    ast.getTime()->accept(*this);
    ast.getExpr()->accept(*this);
    auto newast = std::make_unique<TimeAST>(stack_pop(),stack_pop());
    res_stack.push(std::move(newast));
}

std::shared_ptr<ListAST> KNormalizeVisitor::getResult(){
  return current_context;
}

AST_Ptr KNormalizeVisitor::stack_pop() {//helper
  auto res = res_stack.top();
  res_stack.pop();
  return res;
}

}  // namespace mimium