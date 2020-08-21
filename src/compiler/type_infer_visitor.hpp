/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/ast_new.hpp"
#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
#include "compiler/ffi.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each
// variable has unique name regardless its scope)

namespace mimium {
struct TypevarReplaceVisitor {
  std::unordered_map<int, types::Value> tvmap;
  void replace();
  types::Value getTypeVarFromMap(types::TypeVar& i) {
    types::Value res;
    if (auto tvi = std::get_if<Rec_Wrap<types::TypeVar>>(&tvmap[i.index])) {
      if (tvi->getraw().index == i.index) {
        Logger::debug_log("typevar loop detected, deducted to Float",
                          Logger::WARNING);
        res = types::Float();
      } else {
        res = std::visit(*this, tvmap[i.index]);
      }
    } else {
      res = std::visit(*this, tvmap[i.index]);
    }
    return res;
  }
  // default behaviour for primitive
  types::Value operator()(types::Float& i) { return i; }
  types::Value operator()(types::String& i) { return i; }
  types::Value operator()(types::None& i) { return i; }
  types::Value operator()(types::Void& i) { return i; }

  types::Value operator()(types::Ref& i) {
    return types::Ref(std::visit(*this, i.val));
  }
  types::Value operator()(types::Pointer& i) {
    return types::Pointer(std::visit(*this, i.val));
  }
  types::Value operator()(types::TypeVar& i) {
    types::Value res;
    if (tvmap.count(i.index) > 0) {
      res = getTypeVarFromMap(i);
    } else {
      // case of fail to infer
      Logger::debug_log("failed to infer type", Logger::WARNING);
      res = types::Float();
    }
    return res;
  }
  types::Value operator()(types::Function& i) {
    std::vector<types::Value> newarg;
    for (auto& a : i.arg_types) {
      newarg.emplace_back(std::visit(*this, a));
    }
    auto newret = std::visit(*this, i.ret_type);
    return types::Function(std::move(newret), std::move(newarg));
  }
  types::Value operator()(types::Closure& i) {
    auto newcap = std::visit(*this, i.captures);
    return types::Closure((*this)(i.fun), std::move(newcap));
  };
  types::Value operator()(types::Array& i) {
    auto newelem = std::visit(*this, i.elem_type);
    return types::Array(std::move(newelem), i.size);
  }
  types::Value operator()(types::Struct& i) {
    std::vector<types::Struct::Keytype> newarg;
    for (auto& a : i.arg_types) {
      types::Struct::Keytype v = {a.field, std::visit(*this, a.val)};
      newarg.emplace_back(std::move(v));
    }
    return types::Struct(std::move(newarg));
  }
  types::Value operator()(types::Tuple& i) {
    std::vector<types::Value> newarg;
    for (auto& a : i.arg_types) {
      newarg.emplace_back(std::visit(*this, a));
    }
    return types::Tuple(std::move(newarg));
  }
  // types::Value operator()(types::Time& i) {
  //   return types::Time(std::visit(*this, i.val));
  // }
  types::Value operator()(types::Alias& i) {
    return types::Alias(i.name, std::visit(*this, i.target));
  };
};
class TypeInferVisitor : public ASTVisitor {
  friend TypevarReplaceVisitor;

 public:
  TypeInferVisitor();
  ~TypeInferVisitor() override = default;
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

  bool typeCheck(types::Value& lt, types::Value& rt);
  bool unify(types::Value& lt, types::Value& rt);

  bool unify(std::string lname, std::string rname);
  bool unify(std::string lname, types::Value& rt);
  // bool unifyArg(types::Value& target, types::Value& realarg);

  TypeEnv& getEnv() { return typeenv; };
  types::Value getLastType() { return res_stack.top(); }
  types::Value stackPop() {
    auto res = res_stack.top();
    res_stack.pop();
    return res;
  }
  std::string tmpfname;

  TypeEnv& infer(AST_Ptr toplevel);
  void replaceTypeVars();

 private:
  std::stack<types::Value> res_stack;
  // static bool checkArg(types::Value& fnarg, types::Value& givenarg);
  void unifyTypeVar(types::TypeVar& tv, types::Value& v);
  // hold value for infer type of "self"
  std::optional<types::Value> current_return_type;
  TypeEnv typeenv;
  TypevarReplaceVisitor tvreplacevisitor;
  // std::unordered_map<int, types::Value> typevar_to_actual_map;
  bool has_return;
};

// new typeinferer

struct OccurChecker {
  types::TypeVar& tv;
  explicit OccurChecker(types::TypeVar& target) : tv(target) {}
  bool operator()(types::Function& t) {
    return std::visit(*this, t.ret_type) || checkArgs(t.arg_types);
  }
  bool operator()(types::Array& t) { return std::visit(*this, t.elem_type); }
  bool operator()(types::Struct& t) { return checkArgs(t.arg_types); }
  bool operator()(types::Tuple& t) { return checkArgs(t.arg_types); };
  bool operator()(types::TypeVar& t) { return (t.index == tv.index); }
  template <typename T>
  bool operator()(T& t) {
    bool res;
    if constexpr (T::kind == types::Kind::POINTER) {
      res = std::visit(*this, t.val);
    } else if constexpr (std::is_same_v<std::decay_t<T>, types::Alias>) {
      res = std::visit(*this, t.target);
    } else {
      res = false;
    }
    return res;
  }
  template <typename T>
  bool operator()(Rec_Wrap<T>& t) {
    return (*this)(t.getraw());
  }
  bool checkArgs(std::vector<types::Value>& args) {
    bool res = false;
    for (auto&& a : args) {
      res |= std::visit(*this, a);
    }
    return res;
  }
  bool checkArgs(std::vector<types::Struct::Keytype>& args) {
    bool res = false;
    for (auto&& a : args) {
      res |= std::visit(*this, a.val);
    }
    return res;
  }
};

struct TypeInferer {
  struct ExprTypeVisitor : public VisitorBase<types::Value> {
    explicit ExprTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Op& ast);
    types::Value operator()(ast::Number& ast);
    types::Value operator()(ast::String& ast);
    types::Value operator()(ast::Rvar& ast);
    types::Value operator()(ast::Self& ast);
    types::Value operator()(ast::Lambda& ast);
    types::Value operator()(ast::Fcall& ast);
    types::Value operator()(ast::Time& ast);
    types::Value operator()(ast::Struct& ast);
    types::Value operator()(ast::StructAccess& ast);
    types::Value operator()(ast::ArrayInit& ast);
    types::Value operator()(ast::ArrayAccess& ast);
    types::Value operator()(ast::Tuple& ast);

   private:
    TypeInferer& inferer;
  };
  // visitor for ast::Statements. its return value will be the return/expr of
  // last line in statements(used for inference of function return type).
  struct StatementTypeVisitor : public VisitorBase<types::Value> {
    explicit StatementTypeVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(ast::Assign& ast);
    types::Value operator()(ast::Return& ast);
    // types::Value operator()(ast::Declaration& ast);
    types::Value operator()(ast::For& ast);
    types::Value operator()(ast::If& ast);
    types::Value operator()(ast::ExprPtr& ast);

   private:
    TypeInferer& inferer;
  };
  struct TypeUnifyVisitor {
    explicit TypeUnifyVisitor(TypeInferer& parent) : inferer(parent) {}
    TypeInferer& inferer;

    // // typevar unifying
    template <typename T>
    types::Value unify(T t1, types::rTypeVar t2) {
      types::Value res = t1;
      auto& t2_real = inferer.typeenv.findTypeVar(t2.getraw().index);
      if (!std::holds_alternative<types::rTypeVar>(t2_real)) {
        res = inferer.unify(t2_real, types::Value(t1));
      } else {
        if (OccurChecker{rv::get<types::TypeVar>(t2_real)}(t1)) {
          throw std::runtime_error("type loop detected");
        }
        inferer.typeenv.tv_container[rv::get<types::TypeVar>(t2_real).index] = t1;
      }
      return res;
    }
    template <typename T>
    types::Value unify(types::rTypeVar t1, T t2) {
      return unify(t2, t1);  //
    }
    types::Value unify(types::rTypeVar t1, types::rTypeVar t2) {
      types::Value res = t1;
      auto i1 = t1.getraw().index;
      auto i2 = t2.getraw().index;
      auto& t1_point = inferer.typeenv.tv_container[i1];
      auto& t2_point = inferer.typeenv.tv_container[i2];
      bool t1contain = !std::holds_alternative<types::rTypeVar>(t1_point);
      bool t2contain = !std::holds_alternative<types::rTypeVar>(t2_point);
      if (i1 != i2) {
        if (!t1contain && !t2contain) {
          if (i1 > i2) {
            t1_point = t2_point;
            res = t2_point;
          } else if(i1<i2){
            t2_point = t1_point;
            res = t1_point;
          }

        } else {
          res = inferer.unify(t1_point, t2_point);
        }
      }
      return res;
    }
    types::Value unify(types::rPointer p1, types::rPointer p2);
    types::Value unify(types::rRef p1, types::rRef p2);
    types::Value unify(types::rAlias a1, types::rAlias a2);
    template <typename T>
    types::Value unify(T a1, T a2) {
      return a1;
    }

    template <typename T, typename std::enable_if<
                              std::is_same_v<std::decay_t<T>, types::TypeVar>>::type=nullptr>
    types::Value unify(types::rAlias a1, T a2) {
      return std::visit(*this, a1.getraw().target, types::Value(a2));
    }
    template <typename T, typename std::enable_if<
                              std::is_same_v<std::decay_t<T>, types::TypeVar>>::type=nullptr>
    types::Value unify(T a1, types::rAlias a2) {
      return std::visit(*this, types::Value(a1), a2.getraw().target);
    }
    types::Value unify(types::rFunction f1, types::rFunction f2);
    types::Value unify(types::rArray a1, types::rArray a2);
    types::Value unify(types::rStruct s1, types::rStruct s2);
    types::Value unify(types::rTuple f1, types::rTuple f2);
    // // template <typename T1, typename T2>
    // // types::Value operator()(Rec_Wrap<T1>& t1, Rec_Wrap<T2>& t2) {
    // //   return (*this)(static_cast<T1&>(t1),static_cast<T2&>(t2));
    // // }

    // // template <typename T1, typename T2>
    // // types::Value operator()(Rec_Wrap<T1>& t1, T2& t2) {
    // //   return (*this)(static_cast<T1&>(t1), t2);
    // // }
    // // template <typename T1, typename T2>
    // // types::Value operator()(Rec_Wrap<T1> const& t1, T2& t2) {
    // //   return (*this)(static_cast<const T1&>(t1), t2);
    // // }
    // // template <typename T1, typename T2>
    // //  types::Value operator()(T1& t1, Rec_Wrap<T2>& t2) {
    // //   return (*this)(t1, static_cast<T2&>(t2));
    // // }
    // // template <typename T1, typename T2>
    // //  types::Value operator()(T1& t1, Rec_Wrap<T2> const& t2) {
    // //   return (*this)(t1, static_cast<const T2&>(t2));
    // // }
    // template <typename T>
    // types::Value operator()(T t1, T t2) {
    //   return t1;  // for primitives
    // }
    // type mismatch.

    // note(tomoya): t2 in debugger may be desplayed as "error: no value"
    // despite they are active(because of expansion of parameter pack in
    // std::visit)
    template <typename T1, typename T2>
    types::Value operator()(T1&& t1, T2&& t2) {
      constexpr bool issame =
          std::is_same_v<std::decay_t<T1>, std::decay_t<T2>>;
      constexpr bool istv_l = std::is_same_v<std::decay_t<T1>, types::rTypeVar>;
      constexpr bool istv_r = std::is_same_v<std::decay_t<T2>, types::rTypeVar>;
      if constexpr (issame || istv_l || istv_r) {
        return unify(t1, t2);
      } else {
        throw std::runtime_error("type mismatch");
        return t1;  // for primitives
      }
    }
    std::vector<types::Value> unifyArgs(std::vector<types::Value>& v1,
                                        std::vector<types::Value>& v2);
  };
  struct SubstituteVisitor {
    explicit SubstituteVisitor(TypeInferer& parent) : inferer(parent) {}
    types::Value operator()(types::TypeVar& t) {
      auto& target = inferer.typeenv.findTypeVar(t.index);
      if (std::visit(OccurChecker{t}, target)) {
        Logger::debug_log( "type loop detected. decuced into float type.",Logger::WARNING);
        return types::Float();
      }
      auto contained = std::visit(*this,target);
      if (std::holds_alternative<types::None>(contained) ||
          std::holds_alternative<types::rTypeVar>(contained)) {
        throw std::runtime_error(
            "failed to replace typevar. decuced into float type.");
        return types::Float();
      }
      return std::visit(*this, target);
    }
    types::Value operator()(types::Function& f) {
      return types::Function(std::visit(*this, f.ret_type),
                             replaceArgs(f.arg_types));
    }
    types::Value operator()(types::Alias& a) {
      return types::Alias(a.name, std::visit(*this, a.target));
    }
    // TODO(tomoya): other aggregate types...
    template <typename T>
    types::Value operator()(T& t) {
      if constexpr (T::kind == types::Kind::POINTER) {
        return T(std::visit(*this, t.val));
      }
      return t;
    }
    template <typename T>
    types::Value operator()(Rec_Wrap<T>& t) {
      return (*this)(t.getraw());
    }
    std::vector<types::Value> replaceArgs(std::vector<types::Value>& arg) {
      std::vector<types::Value> res;
      for (auto&& a : arg) {
        res.emplace_back(std::visit(*this, a));
      }
      return res;
    }
    TypeInferer& inferer;
  };

  friend struct ExprTypeVisitor;
  friend struct StatementTypeVisitor;
  friend struct TypeUnifyVisitor;
  TypeInferer()
      : exprvisitor(*this),
        statementvisitor(*this),
        unifyvisitor(*this),
        substitutevisitor(*this),
        typeenv() {
    for (const auto& [key, val] : LLVMBuiltin::ftable) {
      typeenv.emplace(key, val.mmmtype);
    }
    typeenv.emplace("mimium_getnow", types::Function(types::Float(), {}));
  }
  // entry point.
  TypeEnv& infer(ast::Statements& topast);
  types::Value inferStatements(ast::Statements& statements);
  TypeEnv& getTypeEnv() { return typeenv; }

 private:
  TypeEnv typeenv;
  std::unordered_map<int, types::Value> typevarmap;
  std::stack<types::Value> selftype_stack;
  ExprTypeVisitor exprvisitor;
  StatementTypeVisitor statementvisitor;
  TypeUnifyVisitor unifyvisitor;
  SubstituteVisitor substitutevisitor;
  types::Value addLvar(ast::Lvar& lvar);

  types::Value unify(types::Value lhs, types::Value rhs) {
    return std::visit(unifyvisitor, lhs, rhs);
  }

  void substituteTypeVars();
};

}  // namespace mimium