/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
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
  std::vector<mir::valueptr> known_functions;
  std::unordered_map<std::string, std::vector<std::string>> fvinfo;
  // fname: types::Tuple(...)
  std::unordered_map<std::string, types::Value> clstypeenv;

  void moveFunToTop(mir::blockptr mir);
  bool isKnownFunction(mir::valueptr fn);
  std::string makeCaptureName() { return "Capture." + std::to_string(capturecount++); }
  std::string makeClosureTypeName() { return "Closure." + std::to_string(closurecount++); }

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::list<mir::valueptr>::iterator& position)
        : cc(cc), position(position) {}

    ClosureConverter& cc;
    std::list<mir::valueptr> fvlist;
    mir::blockptr block_ctx;

    std::list<mir::valueptr>::iterator position;

    void updatepos() { ++position; }
    void checkFreeVar(mir::valueptr val);
    void checkFreeVar(mir::blockptr block);
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

   private:
    static void visitinsts(minst::Function& i, CCVisitor& ccvis);
    minst::MakeClosure createClosureInst(types::Function ftype, types::Alias fvtype,
                                         std::string& lv_name);
    void dump();

  };
  friend struct CCVisitor;
};
}  // namespace mimium