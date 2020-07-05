/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
using mmmfloat = double;

namespace mimium {
namespace newast {

// forward declaration
struct Base;
struct Op;
struct Number;
struct String;

struct Symbol;
struct Lvar;
struct Rvar;
struct Self;

struct Lambda;
struct Fcall;

struct Struct;  // currently internally used for closure conversion;
struct StructAccess;

struct LambdaArgs;
struct FcallArgs;
struct ArrayInit;
struct Tuple;

struct ArrayAccess;

struct Time;

using Expr =
    std::variant<Op, Number, String, Rec_Wrap<Rvar>, Self, Rec_Wrap<Lambda>,
                 Rec_Wrap<Fcall>, Rec_Wrap<Time>, Rec_Wrap<StructAccess>,
                 Rec_Wrap<ArrayInit>, Rec_Wrap<ArrayAccess>, Rec_Wrap<Tuple>>;

struct Fdef;  // internally equivalent to lambda
struct Assign;
struct Return;
struct Declaration;
struct For;
struct If;

using Statement = std::variant<Assign, Return, Declaration, Rec_Wrap<Fdef>,
                               Rec_Wrap<For>, Rec_Wrap<If>, Rec_Wrap<Expr>>;
using Statements = std::deque<Statement>;

using ExprPtr = std::unique_ptr<Expr>;

enum class OpId {
  Add,
  Sub,
  Mul,
  Div,
  Equal,
  NotEq,
  LessEq,
  GreaterEq,
  LessThan,
  GreaterThan,
  And,
  BitAnd,
  Or,
  BitOr,
  Xor,
  Exponent,
  Not,
  LShift,
  RShift
};

struct DebugInfo {
  struct SourceLoc {
    int line;
    int col;
  } source_loc;
  std::string symbol;
};
struct Base {
  DebugInfo debuginfo;
};

// Operator ast. lhs might be nullopt in case of Sub and Not operator.

struct Op : public Base {
  std::optional<ExprPtr> lhs;
  ExprPtr rhs;
};
struct Number : public Base {
  mmmfloat value{};
};
struct Symbol : public Base {
  std::string value{};
};
struct String : public Symbol {};
struct Lvar : public Symbol {};
struct Rvar : public Symbol {};
struct Self : public Base {};

struct LambdaArgs : public Base {
  std::deque<std::unique_ptr<Lvar>> args;
};
struct Lambda : public Base {
  std::unique_ptr<LambdaArgs> args;
  std::unique_ptr<Statements> body;
};
struct Fdef : public Lambda {};

struct FcallArgs : public Base {
  std::deque<ExprPtr> args;
};

// Fcall ast, callee may be not simply function name but lambda definition or
// high order function

struct Fcall : public Base {
  ExprPtr fn;
  FcallArgs args;
};
// Time ast, only a function call can be tied with time.
struct Time : public Base {
  std::unique_ptr<Fcall> fcall;
  mmmfloat time;
};

struct Assign : public Base{
    std::unique_ptr<Lvar> lvar;
    ExprPtr expr;
};


}  // namespace newast
}  // namespace mimium