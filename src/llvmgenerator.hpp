#pragma once
#include <memory>

#include "alphaconvert_visitor.hpp"
#include "ast.hpp"
#include "closure_convert.hpp"
#include "knormalize_visitor.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm_builtin_functions.hpp"

namespace mimium {
class LLVMBuiltin;
class LLVMGenerator : public std::enable_shared_from_this<LLVMGenerator> {
 private:
  // std::string filename;
  std::shared_ptr<LLVMBuiltin> builtinfn;

  auto getType(const types::Value& type) -> llvm::Type*;
  auto getRawStructType(const types::Value& type) -> llvm::Type*;
  void preprocess();
  void visitInstructions(const Instructions& inst);
  void dropAllReferences();
 public:
  llvm::LLVMContext ctx;
  std::unique_ptr<llvm::Function> curfunc;
  std::shared_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unordered_map<std::string, llvm::Value*> namemap;
  llvm::BasicBlock* mainentry;
  llvm::BasicBlock* currentblock;
  explicit LLVMGenerator(std::string filename);
  // explicit LLVMGenerator(llvm::LLVMContext& _cts,std::string filename);

  ~LLVMGenerator();
  void init(std::string filename);
  void reset(std::string filename);

  auto getModule() -> std::shared_ptr<llvm::Module>;
  void setBB(llvm::BasicBlock* newblock);

  void generateCode(std::shared_ptr<MIRblock> mir);
  void outputToStream(llvm::raw_ostream& ostream);
};
// struct InstructionVisitor{
//     void operator()(NumberInst)
// };
}  // namespace mimium