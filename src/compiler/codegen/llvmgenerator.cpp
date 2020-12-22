/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/codegen/llvmgenerator.hpp"
#include "compiler/codegen/codegen_visitor.hpp"
#include "compiler/codegen/typeconverter.hpp"

namespace mimium {

LLVMGenerator::LLVMGenerator(llvm::LLVMContext& ctx, TypeEnv& typeenv)
    : ctx(ctx),
      module(std::make_unique<llvm::Module>("no_file_name.mmm", ctx)),
      builder(std::make_unique<llvm::IRBuilder<>>(ctx)),
      mainentry(nullptr),
      currentblock(nullptr),
      curfunc(nullptr),
      typeenv(typeenv),
      typeconverter(std::make_unique<TypeConverter>(*builder, *module)),
      runtime_fun_names(
          {{"mimium_getnow", llvm::FunctionType::get(getDoubleTy(), {}, false)},
           {"access_array_lin_interp",
            llvm::FunctionType::get(
                getDoubleTy(), {llvm::PointerType::get(getDoubleTy(), 0), getDoubleTy()}, false)},
           {"mimium_malloc", llvm::FunctionType::get(geti8PtrTy(), {geti64Ty()}, false)}}) {}
void LLVMGenerator::init(std::string filename) {
  module->setSourceFileName(filename);
  module->setModuleIdentifier(filename);
  module->setTargetTriple(LLVMGetDefaultTargetTriple());
}

void LLVMGenerator::setDataLayout(const llvm::DataLayout& dl) { module->setDataLayout(dl); }

void LLVMGenerator::reset(std::string filename) {
  dropAllReferences();
  init(filename);
}

LLVMGenerator::~LLVMGenerator() { dropAllReferences(); }
void LLVMGenerator::dropAllReferences() {
  variable_map.clear();
  if (module != nullptr) { module->dropAllReferences(); }
}

llvm::Type* LLVMGenerator::getType(types::Value& type) { return std::visit(*typeconverter, type); }

llvm::Type* LLVMGenerator::getType(const std::string& name) { return getType(typeenv.find(name)); }
llvm::Type* LLVMGenerator::getClosureToFunType(types::Value& type) {
  auto aliasty = rv::get<types::Alias>(type);
  auto clsty = rv::get<types::Closure>(aliasty.target);

  auto fty = rv::get<types::Function>(clsty.fun.val);
  fty.arg_types.emplace_back(types::Ref{clsty.captures});
  return (*typeconverter)(fty);
}
void LLVMGenerator::switchToMainFun() {
  setBB(mainentry);
  currentblock = mainentry;
  codegenvisitor->recursivefn_ptr = nullptr;
  curfunc = mainentry->getParent();
}
llvm::Function* LLVMGenerator::getForeignFunction(const std::string& name) {
  auto& [type, targetname] = LLVMBuiltin::ftable.find(name)->second;
  if (name == "delay") {
    rv::get<types::Function>(type).arg_types.emplace_back(types::Ref{types::delaystruct});
  }
  return getFunction(targetname, getType(type));
}
llvm::Function* LLVMGenerator::getRuntimeFunction(const std::string& name) {
  const auto& type = runtime_fun_names.at(name);
  return getFunction(name, type);
}

llvm::Function* LLVMGenerator::getFunction(const std::string& name, llvm::Type* type) {
  auto* funtype = llvm::cast<llvm::FunctionType>(type);
  auto fnc = module->getOrInsertFunction(name, funtype);
  auto* fn = llvm::cast<llvm::Function>(fnc.getCallee());
  fn->setCallingConv(llvm::CallingConv::C);
  return fn;
}

void LLVMGenerator::setBB(llvm::BasicBlock* newblock) { builder->SetInsertPoint(newblock); }
void LLVMGenerator::createMiscDeclarations() {
  // create malloc
  auto* i8 = builder->getInt8Ty();
  auto* i8ptr = builder->getInt8PtrTy();
  auto* i64 = builder->getInt64Ty();
  auto* b = builder->getInt1Ty();
  // create llvm memset
  auto* memsettype = llvm::FunctionType::get(builder->getVoidTy(), {i8ptr, i8, i64, b}, false);
  module->getOrInsertFunction("llvm.memset.p0i8.i64", memsettype).getCallee();
  for (auto [name, type] : runtime_fun_names) {
    module->getOrInsertFunction(name, llvm::cast<llvm::FunctionType>(type));
  }
}

// Create mimium_main() function it returns address of closure object for dsp()
// function if it exists.

void LLVMGenerator::createMainFun() {
  auto* fntype = llvm::FunctionType::get(builder->getInt8PtrTy(), false);
  auto* mainfun =
      llvm::Function::Create(fntype, llvm::Function::ExternalLinkage, "mimium_main", *module);
  mainfun->setCallingConv(llvm::CallingConv::C);
  curfunc = mainfun;
  variable_map.emplace(curfunc, std::make_shared<namemaptype>());
  using Akind = llvm::Attribute;
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) { aset = aset.addAttribute(ctx, a); }

  mainfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  mainentry = llvm::BasicBlock::Create(ctx, "entry", mainfun);
}
void LLVMGenerator::createTaskRegister(bool isclosure = false) {
  std::vector<llvm::Type*> argtypes = {
      builder->getDoubleTy(),   // time
      builder->getInt8PtrTy(),  // address to function
      builder->getDoubleTy()    // argument(single)
  };
  std::string name = "addTask";
  if (isclosure) {
    argtypes.emplace_back(builder->getInt8PtrTy());  // address to closure args(instead of void* type)
    name = "addTask_cls";
  }
  auto* fntype = llvm::FunctionType::get(builder->getVoidTy(), argtypes, false);
  auto addtask = module->getOrInsertFunction(name, fntype);
  auto* addtaskfun = llvm::cast<llvm::Function>(addtask.getCallee());

  addtaskfun->setCallingConv(llvm::CallingConv::C);
  using Akind = llvm::Attribute;
  std::vector<Akind::AttrKind> attrs = {Akind::NoUnwind, Akind::NoInline, Akind::OptimizeNone};
  llvm::AttributeSet aset;
  for (auto& a : attrs) { aset = aset.addAttribute(ctx, a); }
  addtaskfun->addAttributes(llvm::AttributeList::FunctionIndex, aset);
  setValuetoMap(name, addtask.getCallee());
}

void LLVMGenerator::createNewBasicBlock(std::string name, llvm::Function* f) {
  auto* bb = llvm::BasicBlock::Create(ctx, name, f);
  builder->SetInsertPoint(bb);
  currentblock = bb;
}
void LLVMGenerator::createRuntimeSetDspFn() {
  auto* voidptrtype = builder->getInt8PtrTy();
  auto* dspfnaddress = builder->CreateBitCast(module->getFunction("dsp"), voidptrtype);
  llvm::Value* dspclsaddress = nullptr;
  auto dspcls_cap = variable_map[curfunc]->find("ptr_dsp.cap");
  if (dspcls_cap != variable_map[curfunc]->end()) {
    dspclsaddress = builder->CreateBitCast(dspcls_cap->second, voidptrtype);
  } else {
    dspclsaddress = llvm::ConstantPointerNull::get(voidptrtype);
  }
  llvm::Value* dspmemobjaddress = nullptr;
  auto dspmemobj = variable_map[curfunc]->find("ptr_dsp.mem");
  if (dspmemobj != variable_map[curfunc]->end()) {
    dspmemobjaddress = builder->CreateBitCast(dspmemobj->second, voidptrtype);
    // insert 0 initialization of memobjs
    auto* memsetfn = module->getFunction("llvm.memset.p0i8.i64");
    auto* t = llvm::cast<llvm::PointerType>(dspmemobj->second->getType())->getElementType();
    auto size = module->getDataLayout().getTypeAllocSize(t);

    auto* sizeinst = getConstInt(size);
    constexpr int bitsize = 8;
    auto* zero = getConstInt(0, bitsize);
    auto* falsev = getConstInt(0, 1);
    builder->CreateCall(memsetfn, {dspmemobjaddress, zero, sizeinst, falsev});

  } else {
    dspmemobjaddress = llvm::ConstantPointerNull::get(voidptrtype);
  }
  auto setdsp = module->getOrInsertFunction(
      "setDspParams", llvm::FunctionType::get(builder->getVoidTy(),
                                              {voidptrtype, voidptrtype, voidptrtype}, false));
  builder->CreateCall(setdsp, {dspfnaddress, dspclsaddress, dspmemobjaddress});
}

llvm::Value* LLVMGenerator::getOrCreateFunctionPointer(llvm::Function* f) {
  auto name = std::string(f->getName()) + "_ptr";
  llvm::Value* funptr = module->getNamedGlobal(name);
  if (funptr == nullptr) {
    funptr = module->getOrInsertGlobal(name, llvm::PointerType::get(f->getType(), 0));
    builder->CreateStore(f, funptr);
  }
  return funptr;
}

void LLVMGenerator::preprocess() {
  createMainFun();
  createMiscDeclarations();
  createTaskRegister(true);   // for non-closure function
  createTaskRegister(false);  // for closure
  setBB(mainentry);
}
void LLVMGenerator::visitInstructions(mir::valueptr inst, bool isglobal) {
  codegenvisitor->isglobal = isglobal;
  if (auto* i = std::get_if<mir::Instructions>(inst.get())) {
    codegenvisitor->instance_holder = inst;
    llvm::Value* res = std::visit(*codegenvisitor, *i);
    codegenvisitor->registerLlvmVal(inst, res);
  }
}

void LLVMGenerator::generateCode(mir::blockptr mir, const funobjmap* funobjs) {
  codegenvisitor = std::make_shared<CodeGenVisitor>(*this, funobjs);
  preprocess();

  for (auto& inst : mir->instructions) { visitInstructions(inst, true); }
  if (module->getFunction("dsp") != nullptr) { createRuntimeSetDspFn(); }
  // main always return null for now;
  builder->CreateRet(llvm::ConstantPointerNull::get(builder->getInt8PtrTy()));
}

llvm::Value* LLVMGenerator::tryfindValue(std::string name) {
  auto map = variable_map[curfunc];
  llvm::Value* res = nullptr;
  auto iter = map->find(name);
  if (iter != map->end()) { res = iter->second; }
  if (isVarOverWritten(name)) {
    auto* ptr = findValue("ptr_" + name);
    res = builder->CreateLoad(ptr, name);
  }
  return res;
}
llvm::Value* LLVMGenerator::findValue(std::string name) {
  auto* res = tryfindValue(name);
  if (res == nullptr) {
    throw std::runtime_error("variable " + name + " cannot be found in llvm conversion");
  }

  return res;
}

void LLVMGenerator::setValuetoMap(std::string name, llvm::Value* val) {
  auto map = variable_map[curfunc];
  map->try_emplace(name, val);
}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

void LLVMGenerator::dumpvar(llvm::Value* v) {
  v->print(llvm::errs(), true);
  llvm::errs() << "\n";
}
void LLVMGenerator::dumpvar(llvm::Type* v) {
  v->print(llvm::errs(), true);
  llvm::errs() << "\n";
}
void LLVMGenerator::dumpvars() {
  for (auto& [f, map] : variable_map) {
    llvm::errs() << f->getName() << ":\n";
    for (auto& [key, val] : *map) {
      llvm::errs() << "   " << key << " :  " << val->getName() << "\n";
    }
  }
}
}  // namespace mimium