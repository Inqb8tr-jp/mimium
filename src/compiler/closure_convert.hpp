/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <set>

#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
#include "compiler/ffi.hpp"
namespace mimium {

namespace minst = mir::instruction;

class ClosureConverter {
 public:
  explicit ClosureConverter(TypeEnv& typeenv);
  ~ClosureConverter();
  mir::blockptr convert(mir::blockptr toplevel);
  void reset();
  bool hasCapture(const std::string& fname) { return fvinfo.count(fname) > 0; }

  auto& getCaptureNames(const std::string& fname) { return fvinfo[fname]; }
  auto& getCaptureType(const std::string& fname) { return clstypeenv[fname]; }
  // void dump();
 private:
  TypeEnv& typeenv;
  mir::blockptr toplevel;
  int capturecount;
  int closurecount;
  std::set<mir::valueptr> known_functions;
  std::unordered_map<std::string, std::vector<std::string>> fvinfo;
  // fname: types::Tuple(...)
  std::unordered_map<std::string, types::Value> clstypeenv;
  std::unordered_map<mir::valueptr, mir::valueptr> fn_to_cls;

  void moveFunToTop(mir::blockptr mir);
  bool isKnownFunction(mir::valueptr fn);
  std::string makeCaptureName() { return "Capture." + std::to_string(capturecount++); }
  std::string makeClosureTypeName() { return "Closure." + std::to_string(closurecount++); }

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::list<mir::valueptr>::iterator& position)
        : cc(cc) {}

    ClosureConverter& cc;
    std::set<mir::valueptr> fvset;
    mir::blockptr block_ctx;

    std::list<mir::valueptr>::iterator position;

    void checkFreeVar(mir::valueptr val);
    void checkFreeVar(mir::blockptr block);
    void checkFreeVarArg(mir::valueptr val);
    void checkVariable(mir::valueptr& val, bool ismemoryvalue = false);
    void tryReplaceFntoCls(mir::valueptr& val);

    // void registerFv(std::string& name);
    void operator()(minst::Ref& i);
    void operator()(minst::Load& i);
    void operator()(minst::Store& i);

    void operator()(minst::Op& i);
    void operator()(minst::Function& i);
    void operator()(minst::Fcall& i);
    void operator()(minst::MakeClosure& i);
    void operator()(minst::Array& i);
    void operator()(minst::Field& i);
    void operator()(minst::If& i);
    void operator()(minst::Return& i);

    // for primitive operations
    template <typename T>
    void operator()(T& i) {
      constexpr bool isprimitive = std::is_base_of_v<mir::instruction::Base, std::decay_t<T>>;
      static_assert(isprimitive, "mir instruction unreachable");
    }
    bool isFreeVar(const std::string& name);
    void visit(mir::Value& i) {
      if (auto* instptr = std::get_if<mir::Instructions>(&i)) { std::visit(*this, *instptr); }
    }
    void visit(mir::Instructions& i) { std::visit(*this, i); }

    mir::valueptr instance_holder = nullptr;

   private:
    void visitinsts(minst::Function& i, CCVisitor& ccvis);
    minst::MakeClosure createClosureInst(mir::valueptr fnptr,
                                         std::vector<mir::valueptr> const& fvs, types::Alias fvtype,
                                         std::string& lv_name);
    void dump();

    // helper function to get pointer of actual instance in visitor function.
    // it validates raw pointer is really same before evaluation.
    template <class T>
    mir::valueptr getValPtr(T* i) {
      assert(i == &mir::getInstRef<T>(instance_holder));
      return instance_holder;
    }
  };
  friend struct CCVisitor;
};
}  // namespace mimium