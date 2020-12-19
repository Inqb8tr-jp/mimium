/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "collect_memoryobjs.hpp"
namespace mimium {

std::unordered_set<mir::valueptr> MemoryObjsCollector::collectToplevelFuns(mir::blockptr toplevel) {
  std::unordered_set<mir::valueptr> res;
  for (const auto& inst : toplevel->instructions) {
    if (mir::isInstA<minst::Function>(inst)) { res.emplace(inst); }
  }
  return res;
}

std::optional<mir::valueptr> MemoryObjsCollector::tryFindFunByName(
    std::unordered_set<mir::valueptr> fnset, std::string const& name) {
  auto res = std::find_if(fnset.cbegin(), fnset.cend(), [&](mir::valueptr v) {
    auto& f = mir::getInstRef<minst::Function>(v);
    return f.name == name;
  });
  return (res == fnset.end()) ? std::nullopt : std::optional(*res);
}

std::shared_ptr<FunObjTree> MemoryObjsCollector::traverseFunTree(mir::valueptr fun) {
  assert(mir::isInstA<minst::Function>(fun));
  const auto& f = mir::getInstRef<minst::Function>(fun);
  CollectMemVisitor visitor(*this);
  auto res = visitor.visitInsts(f.body);
  if (res.hasself) {
    auto rettype = rv::get<types::Function>(f.type).ret_type;
    auto& resulttype = CollectMemVisitor::getTupleFromAlias(res.objtype);
    resulttype.arg_types.emplace_back(rettype);
  }
  auto objptr = std::make_shared<FunObjTree>(FunObjTree{fun, res.hasself, res.objs, res.objtype});
  result_map.emplace(fun, objptr);
  return objptr;
}

funobjmap MemoryObjsCollector::process(mir::blockptr toplevel) {
  auto toplevelfuns = collectToplevelFuns(toplevel);
  auto dsp_fun = tryFindFunByName(toplevelfuns, "dsp");
  if (dsp_fun.has_value()) {
    auto res = traverseFunTree(dsp_fun.value());
#ifdef MIMIUM_DEBUG_BUILD
    dump_res = *res;
#endif

    auto& insts = toplevel->instructions;
    auto dspmemtype = res->objtype;
    auto alloca = minst::Allocate{{"dsp.mem", dspmemtype}, dspmemtype};
    insts.insert(std::begin(insts), std::make_shared<mir::Value>(std::move(alloca)));
  }
  return result_map;
}

std::string MemoryObjsCollector::indentHelper(int indent) {
  std::string indent_s;
  for (int i = 0; i < indent; i++) { indent_s += " "; }
  return indent_s;
}

#ifdef MIMIUM_DEBUG_BUILD

void MemoryObjsCollector::dumpFunObjTree(FunObjTree const& tree, int indent) {
  std::string fname;
  if (auto* ext = std::get_if<mir::ExternalSymbol>(tree.fname.get())) {
    fname = ext->name;
  } else {
    auto& f = mir::getInstRef<minst::Function>(tree.fname);
    fname = f.name;
  }
  std::cerr << indentHelper(indent)<< fname << "->\n";
  int nextindent = indent + 4;
  if (tree.hasself) { std::cerr << indentHelper(nextindent) << "self\n"; }
  for (const auto& o : tree.memobjs) {
    dumpFunObjTree(*o, nextindent);
  }
  if (tree.memobjs.empty() && !tree.hasself) { std::cerr << "\n"; }
}

void MemoryObjsCollector::dump() const {
  std::cerr << "-------------Memory Object Functions: -----\n";
  dumpFunObjTree(dump_res);
  std::cerr << "------------" << std::endl;
}
#endif
// visitor
using ResultT = MemoryObjsCollector::CollectMemVisitor::ResultT;

types::Tuple& MemoryObjsCollector::CollectMemVisitor::getTupleFromAlias(types::Value& t) {
  assert(std::holds_alternative<types::rAlias>(t));
  auto& atype = rv::get<types::Alias>(t);
  assert(std::holds_alternative<types::rTuple>(atype.target));
  return rv::get<types::Tuple>(atype.target);
}

ResultT MemoryObjsCollector::CollectMemVisitor::makeResfromHasSelf(bool hasself) {
  ResultT res;
  res.hasself = hasself;
  return res;
}

void MemoryObjsCollector::CollectMemVisitor::mergeResultTs(ResultT& dest, ResultT& src) {
  dest.hasself |= src.hasself;
  // combine result objects
  dest.objs.splice(dest.objs.end(), std::move(src.objs));
  auto& t1 = getTupleFromAlias(dest.objtype);
  auto& t2 = getTupleFromAlias(src.objtype);
  std::copy(t2.arg_types.begin(), t2.arg_types.end(), std::back_inserter(t1.arg_types));
}

ResultT MemoryObjsCollector::CollectMemVisitor::visitInsts(mir::blockptr block) {
  const auto& insts = block->instructions;
  const auto& name = block->label;
  auto res = ResultT{{}, types::Alias{name, types::Tuple{}}, false};

  return std::accumulate(insts.begin(), insts.end(), res, [&](ResultT acc, mir::valueptr i) {
    auto r = this->visit(i);
    mergeResultTs(acc, r);
    return acc;
  });
}

ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Ref& i) {
  return makeResfromHasSelf(isSelf(i.target));
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Load& i) {
  return makeResfromHasSelf(isSelf(i.target));
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Store& i) {
  return makeResfromHasSelf(isSelf(i.target) && isSelf(i.value));
}

ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Op& i) {
  bool hasself = isSelf(i.rhs);
  if (i.lhs.has_value()) { hasself |= isSelf(i.lhs.value()); }
  return makeResfromHasSelf(hasself);
}

ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Fcall& i) {
  using opt_objtreeptr = std::optional<std::shared_ptr<FunObjTree>>;
  auto opt_tree =
      std::visit(overloaded{[&](const mir::Instructions& inst) -> opt_objtreeptr {
                              assert(std::holds_alternative<minst::Function>(inst));
                              auto ret = M.traverseFunTree(i.fname);
                              if (!ret->memobjs.empty() || ret->hasself) { return ret; }
                              return std::nullopt;
                            },
                            [&](const mir::ExternalSymbol& e) -> opt_objtreeptr {
                              if (e.name == "delay") {
                                return std::make_shared<FunObjTree>(
                                    FunObjTree{i.fname, false, {}, types::delaystruct});
                              }
                              return std::nullopt;
                            },
                            [&](const auto& i) -> opt_objtreeptr {
                              assert(false);  // cannot call self or constant as function
                              return std::nullopt;
                            }},
                 *i.fname);

  bool hasself = std::any_of(i.args.begin(), i.args.end(), [](auto& a) { return isSelf(a); });
  if (i.time) { hasself |= isSelf(i.time.value()); }
  auto res = makeResfromHasSelf(hasself);
  if (opt_tree.has_value()) {
    auto& tree = opt_tree.value();
    res.objs.emplace_back(tree);
    res.objtype = types::Alias{"", types::Tuple{{tree->objtype}}};
  }
  return res;
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::MakeClosure& i) {
  return makeResfromHasSelf(false);
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Array& i) {
  return makeResfromHasSelf(
      std::any_of(i.args.begin(), i.args.end(), [](const auto& a) { return isSelf(a); }));
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::ArrayAccess& i) {
  return makeResfromHasSelf(isSelf(i.target) && isSelf(i.index));
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Field& i) {
  return makeResfromHasSelf(isSelf(i.target) && isSelf(i.index));
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::If& i) {
  bool hasself = isSelf(i.cond);
  auto res = visitInsts(i.thenblock);
  if (i.elseblock.has_value()) {
    auto res2 = visitInsts(i.elseblock.value());
    mergeResultTs(res, res2);
  }
  return res;
}
ResultT MemoryObjsCollector::CollectMemVisitor::operator()(minst::Return& i) {
  bool isself = isSelf(i.val);
  assert(!isself && "self should not be shown in return statement");
  return makeResfromHasSelf(isself);
}
}  // namespace mimium