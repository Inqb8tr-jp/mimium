/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {
class KNormalizeVisitor : public ASTVisitor {
 public:
  explicit KNormalizeVisitor(TypeInferVisitor& typeinfer);
  KNormalizeVisitor();
  ~KNormalizeVisitor() override = default;
  void init();

  void visit(OpAST& ast) override;
  void visit(ListAST& ast) override;
  void visit(NumberAST& ast) override;
  void visit(StringAST& ast) override;

  void visit(LvarAST& ast) override;
  void visit(RvarAST& ast) override;
  void visit(SelfAST& ast) override;

  void visit(AssignAST& ast) override;
  void visit(ArgumentsAST& ast) override;
  void visit(FcallArgsAST& ast) override;
  void visit(ArrayAST& ast) override;
  void visit(ArrayAccessAST& ast) override;
  void visit(FcallAST& ast) override;
  void visit(LambdaAST& ast) override;
  void visit(IfAST& ast) override;
  void visit(ReturnAST& ast) override;
  void visit(ForAST& ast) override;
  void visit(DeclarationAST& ast) override;
  // void visit(TimeAST& ast) override;
  void visit(StructAST& ast) override;
  void visit(StructAccessAST& ast) override;

  std::shared_ptr<MIRblock> getResult();

 private:
  TypeInferVisitor typeinfer;  // to resolve anonymous function type;

  std::shared_ptr<MIRblock> rootblock;
  std::shared_ptr<MIRblock> currentblock;
  int var_counter;
  std::string makeNewName();
  std::string getVarName();
  bool isArgTime(FcallArgsAST& args);
  std::string tmpname;
  std::shared_ptr<ListAST> current_context;
  AST_Ptr insertAssign(AST_Ptr ast);
  void insertOverWrite(AST_Ptr body, const std::string& name);
  void insertAlloca(AST_Ptr body, const std::string& name);
  void insertRef(AST_Ptr body, const std::string& name);

  std::stack<std::string> res_stack_str;
  std::stack<types::Value> type_stack;
  std::vector<std::string> lvar_list;
  std::string stackPopStr() {
    auto ret = res_stack_str.top();
    res_stack_str.pop();
    return ret;
  }
};

class MirGenerator {
 public:
  MIRblock& generate(newast::Statements& topast, TypeEnv& typeenv);
  struct ExprKnormVisitor : public VisitorBase<Instructions> {
    Instructions operator()(newast::Op& ast);
    Instructions operator()(newast::Number& ast);
    Instructions operator()(newast::String& ast);
    Instructions operator()(newast::Rvar& ast);
    Instructions operator()(newast::Self& ast);
    Instructions operator()(newast::Lambda& ast);
    Instructions operator()(newast::Fcall& ast);
    Instructions operator()(newast::Time& ast);
    Instructions operator()(newast::Struct& ast);
    Instructions operator()(newast::StructAccess& ast);
    Instructions operator()(newast::ArrayInit& ast);
    Instructions operator()(newast::ArrayAccess& ast);
    Instructions operator()(newast::Tuple& ast);
  };
  struct StatementKnormVisitor: public VisitorBase<Instructions>  {
    Instructions operator()(newast::Assign& ast);
    Instructions operator()(newast::Return& ast);
    // Instructions operator()(newast::Declaration& ast);
    Instructions operator()(newast::For& ast);
    Instructions operator()(newast::If& ast);
    Instructions operator()(newast::ExprPtr& ast);

  };

 private:
  MIRblock mir;
};

}  // namespace mimium