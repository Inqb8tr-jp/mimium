#include "interpreter_visitor.hpp"

#include <memory>

#include "environment.hpp"

namespace mimium {
void InterpreterVisitor::init() {
  runtime = std::make_unique<Runtime>();
  runtime->init(this->shared_from_this());
}
double InterpreterVisitor::get_as_double(mValue v) {
  return std::visit(
      overloaded{[](double d) { return d; },
                 [this](std::string s) {
                   return get_as_double(this->getRuntime()->findVariable(s));
                 },  // recursive!!
                 [](auto /*a*/) {
                   throw std::runtime_error("refered variable is not number");
                   return 0.;
                 }},
      v);
}
std::shared_ptr<Runtime> InterpreterVisitor::getRuntime() { return runtime; }
mValue InterpreterVisitor::findVariable(std::string str) {
  return runtime->findVariable(str);
};

void InterpreterVisitor::visit(ListAST& ast) {
  for (auto& line : ast.getElements()) {
    line->accept(*this);
  }
}
void InterpreterVisitor::visit(NumberAST& ast) {
  res_stack.push(ast.getVal());
};
void InterpreterVisitor::visit(LvarAST& ast) { res_stack.push(ast.getVal()); };
void InterpreterVisitor::visit(RvarAST& ast) {
  res_stack.push(findVariable(ast.getVal()));
}

void InterpreterVisitor::visit(OpAST& ast) {
  ast.lhs->accept(*this);
  double lv = get_as_double(stack_pop());
  ast.rhs->accept(*this);
  double rv = get_as_double(stack_pop());
  double res;
  switch (ast.getOpId()) {
    case ADD:
      res = lv + rv;
      break;
    case SUB:
      res = lv - rv;
      break;
    case MUL:
      res = lv * rv;
      break;
    case DIV:
      res = lv / rv;
      break;
    case EXP:
      res = std::pow(lv, rv);
      break;
    case MOD:
      res = std::fmod(lv, rv);
      break;
    case AND:
    case BITAND:
      res = static_cast<double>((bool)lv & (bool)rv);
      break;
    case OR:
    case BITOR:
      res = static_cast<double>((bool)lv | (bool)rv);
      break;
    case LT:
      res = static_cast<double>(lv < rv);
      break;
    case GT:
      res = static_cast<double>(lv > rv);
      break;
    case LE:
      res = static_cast<double>(lv <= rv);
      break;
    case GE:
      res = static_cast<double>(lv >= rv);
      break;
    case LSHIFT:
      res = static_cast<double>((int)lv << (int)rv);
      break;
    case RSHIFT:
      res = static_cast<double>((int)lv >> (int)rv);
      break;
    default:
      throw std::runtime_error("invalid binary operator");
      res = 0.0;
  }
  res_stack.push(res);
}
void InterpreterVisitor::visit(AssignAST& ast) {
  std::string varname = ast.getName()->getVal();  // assuming name as symbolast
  auto body = ast.getBody();

  if (body) {
    body->accept(*this);
    runtime->getCurrentEnv()->setVariable(varname, res_stack.top());
    Logger::debug_log(
        "Variable " + varname + " : " + Runtime::to_string(stack_pop()),
        Logger::DEBUG);

  } else {
    throw std::runtime_error("expression not resolved");
  }
}

void InterpreterVisitor::visit(ArgumentsAST& ast) {
  // args = ast;//this visitor is not needed?
}
void InterpreterVisitor::visit(ArrayAST& ast) {
  std::vector<double> v;
  for (auto& elem : ast.getElements()) {
    elem->accept(*this);
    v.push_back(get_as_double(stack_pop()));
  }
  res_stack.push(std::move(v));
};
void InterpreterVisitor::visit(ArrayAccessAST& ast) {
  ast.getName()->accept(*this);
  auto array = stack_pop();
  ast.getIndex()->accept(*this);
  auto index = static_cast<int>(get_as_double(stack_pop()));
  auto res = std::visit(
      overloaded{[&index](std::vector<double> a) -> double { return a[index]; },
                 [](auto /* e */) -> double {
                   throw std::runtime_error(
                       "accessed variable is not an array");
                   return 0;
                 }},
      array);
  res_stack.push(res);
};
overloaded fcall_visitor{
    [](auto /* v */) -> mClosure_ptr {
      throw std::runtime_error("reffered variable is not a function");
      return nullptr;
    },
    [](std::shared_ptr<Closure> v) -> mClosure_ptr { return v; }};
void InterpreterVisitor::visit(FcallAST& ast) {
  std::string name = ast.getFname()->getVal();
  auto args = ast.getArgs();
  if (Builtin::isBuiltin(name)) {
    auto fn = Builtin::builtin_fntable.at(name);
    mValue res = fn(args, this);
    res_stack.push(res);
  } else {
    auto argsv = args->getElements();
    mClosure_ptr closure =
        std::visit(fcall_visitor, runtime->findVariable(name));
    auto& lambda = closure->fun;
    auto originalenv = runtime->getCurrentEnv();
    auto lambdaargs = lambda.getArgs()->getElements();
    std::shared_ptr<Environment<mValue>> tmpenv = closure->env->createNewChild(
        "arg" + name);  // create arguments(upcasting,,,)
    auto argscond = lambdaargs.size() - argsv.size();
    if (argscond < 0) {
      throw std::runtime_error("too many arguments");
    } else {
      int count = 0;
      for (auto& larg : lambdaargs) {
        std::static_pointer_cast<LvarAST>(larg)->accept(*this);
        auto key = std::get<std::string>(stack_pop());
        argsv[count]->accept(*this);
        tmpenv->setVariableRaw(key, stack_pop());
        count++;
      }
      if (argscond == 0) {
        runtime->setCurrentEnv(tmpenv);  // switch env
        lambda.getBody()->accept(*this);
        runtime->getCurrentEnv()->getParent()->deleteLastChild();
        runtime->setCurrentEnv(originalenv);
      } else {
        throw std::runtime_error(
            "too few arguments");  // ideally we want to return new function
                                   // (partial application)
      }
    }
  }
};
void InterpreterVisitor::visit(LambdaAST& ast) {
  res_stack.push(std::make_shared<Closure>(runtime->getCurrentEnv(), ast));
};
void InterpreterVisitor::visit(IfAST& ast) {
  ast.getCond()->accept(*this);
  mValue cond = stack_pop();
  auto cond_d = get_as_double(cond);
  if (cond_d > 0) {
    ast.getThen()->accept(*this);
  } else {
    ast.getElse()->accept(*this);
  }
};
void InterpreterVisitor::visit(ReturnAST& ast) {
  ast.getExpr()->accept(*this);
};
void InterpreterVisitor::doForVisitor(mValue v, std::string iname,
                                      AST_Ptr expression) {
  std::visit(overloaded{[&](std::vector<double> v) {
                          auto it = v.begin();
                          while (it != v.end()) {
                            runtime->getCurrentEnv()->setVariable(iname, *it);
                            expression->accept(*this);
                            it++;
                          }
                        },
                        [&](double v) {
                          runtime->getCurrentEnv()->setVariable(iname, v);
                          expression->accept(*this);
                        },
                        [&](std::string s) {
                          doForVisitor(runtime->findVariable(s), iname,
                                       expression);
                        },
                        [](auto /* v */) {
                          throw std::runtime_error("iterator is invalid");
                        }},
             v);
}

void InterpreterVisitor::visit(ForAST& ast) {
  std::string loopname = "for" + std::to_string(rand());  // temporary,,
  runtime->getCurrentEnv() = runtime->getCurrentEnv()->createNewChild(loopname);
  ast.getVar()->accept(*this);
  auto varname = std::get<std::string>(stack_pop());
  auto expression = ast.getExpression();
  ast.getIterator()->accept(*this);
  mValue iterator = stack_pop();
  doForVisitor(iterator, varname, expression);
  runtime->getCurrentEnv() = runtime->getCurrentEnv()->getParent();
};
void InterpreterVisitor::visit(DeclarationAST& ast) {
  std::string name = ast.getFname()->getVal();
  auto argsast = ast.getArgs();
  auto args = argsast->getElements();
  if (name == "include") {
    assertArgumentsLength(args, 1);
    if (args[0]->getid() == SYMBOL) {  // this is not smart
      args[0]->accept(*this);
      std::string filename = std::get<std::string>(stack_pop());
      auto temporary_driver = std::make_unique<mmmpsr::MimiumDriver>(
          runtime->current_working_directory);
      temporary_driver->parsefile(filename);
      runtime->loadAst(temporary_driver->getMainAst());
    } else {
      throw std::runtime_error("given argument is not a string");
    }
  } else {
    throw std::runtime_error("specified declaration is not defined: " + name);
  }
};
void InterpreterVisitor::visit(TimeAST& ast) {
  ast.getTime()->accept(*this);
  runtime->getScheduler()->addTask(static_cast<int>(get_as_double(stack_pop())),
                                   ast.getExpr());
  res_stack.push(ast.getExpr());  // for print??
};

void InterpreterVisitor::visit(StructAST& ast) {
  // todo implement
}
void InterpreterVisitor::visit(StructAccessAST& ast) {}

bool InterpreterVisitor::assertArgumentsLength(const std::deque<AST_Ptr>& args,
                                               int length) {
  bool res;
  int size = args.size();
  if (size == length) {
    res = true;
  } else {
    throw std::runtime_error(
        "Argument length is invalid. Expected: " + std::to_string(length) +
        " given: " + std::to_string(size));
    res = false;
  };
  return res;
}
}  // namespace mimium