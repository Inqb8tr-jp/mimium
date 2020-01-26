#include "basic/type.hpp"

namespace mimium {
namespace types {

Kind kindOf(const Value& v) { return std::visit(kindvisitor, v); }

bool isPrimitive(const Value& v) { return kindOf(v) == Kind::PRIMITIVE; }

bool isTypeVar(types::Value t) {
  return std::holds_alternative<Rec_Wrap<types::TypeVar>>(t);
}
void unifyTypeVars(TypeVar &tv, Value &v){
  auto& tmp = tv.getFirstLink();
  tmp.contained = v;
  while(tmp.next){
    tmp.next.value()->contained = v;
    tmp = *tmp.next.value();
  }
}


std::string toString(const Value& v, bool verbose) {
  tostrvisitor.verbose = verbose;
  return std::visit(tostrvisitor, v);
}
void dump(const Value& v, bool verbose) {
  std::cerr << toString(v, verbose) << "\n";
}

Value getFunRettype(types::Value& v){
  return rv::get<Function>(v).ret_type;
}
std::optional<Value> getNamedClosure(types::Value& v){
  // Closure* res;
  std::optional<Value> res=std::nullopt;
      if(auto ref = std::get_if<Rec_Wrap<Ref>>(&v)){
   if(auto alias = std::get_if<Rec_Wrap<Alias>>(&(ref->getraw().val))){
    Alias& a = *alias;
    if(auto cls = std::get_if<Rec_Wrap<Closure>>(&a.target)){
      res = *ref;
    }
   }
      }
  return res;
}


}  // namespace types


void TypeEnv::replaceTypeVars(){
  for(auto&[key,val]:env){
    if(rv::holds_alternative<types::TypeVar>(val)){
      auto& tv = rv::get<types::TypeVar>(val);
      if(std::holds_alternative<types::None>(tv.contained)){
        // throw std::runtime_error("type inference for " + key + " failed");
        tv.contained = types::Float();
      }
      env[key] = tv.contained;
    }
  }
}

std::string TypeEnv::toString(bool verbose) {
  std::stringstream ss;
  types::ToStringVisitor vis;
  vis.verbose = verbose;
  for (auto& [key, val] : env) {
    ss << key << " : " << std::visit(vis, val) << "\n";
  }
  return ss.str();
}
void TypeEnv::dump(bool verbose){
std::cerr <<"-------------------\n"<< toString(verbose)<<"-------------------\n";
}
}  // namespace mimium