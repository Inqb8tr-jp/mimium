#include "compiler/compiler.hpp"
namespace mimium {
Compiler::Compiler() : Compiler(*std::make_shared<llvm::LLVMContext>()) {}
Compiler::Compiler(llvm::LLVMContext& ctx)
    : driver(),
      alphavisitor(),
      typevisitor(),
      recursivechecker(),
      knormvisitor(typevisitor),
      closureconverter(
          std::make_shared<ClosureConverter>(typevisitor.getEnv())),
      memobjcollector(typevisitor.getEnv()),
      llvmgenerator(ctx, typevisitor.getEnv(),memobjcollector) {}
Compiler::~Compiler() = default;
void Compiler::setFilePath(std::string path) {
  this->path = path;
  llvmgenerator.init(path);
}
void Compiler::setDataLayout(const llvm::DataLayout& dl) {
  llvmgenerator.setDataLayout(dl);
}
void Compiler::recursiveCheck(AST_Ptr ast) { ast->accept(recursivechecker); }
AST_Ptr Compiler::loadSource(std::string source) {
  driver.parsestring(source);
  auto ast = driver.getMainAst();
  recursiveCheck(ast);
  return ast;
}
AST_Ptr Compiler::loadSourceFile(std::string filename) {
  driver.parsefile(filename);
  auto ast = driver.getMainAst();
  recursiveCheck(ast);
  return ast;
}
AST_Ptr Compiler::alphaConvert(AST_Ptr ast) {
  ast->accept(alphavisitor);
  return alphavisitor.getResult();
}
TypeEnv& Compiler::typeInfer(AST_Ptr ast) { return typevisitor.infer(ast); }

std::shared_ptr<MIRblock> Compiler::generateMir(AST_Ptr ast) {
  ast->accept(knormvisitor);
  return knormvisitor.getResult();
}
std::shared_ptr<MIRblock> Compiler::closureConvert(
    std::shared_ptr<MIRblock> mir) {
  return closureconverter->convert(mir);
}

std::shared_ptr<MIRblock> Compiler::collectMemoryObjs(
    std::shared_ptr<MIRblock> mir) {
  return memobjcollector.process(mir);
}

llvm::Module& Compiler::generateLLVMIr(std::shared_ptr<MIRblock> mir) {
  llvmgenerator.generateCode(mir);
  return llvmgenerator.getModule();
}
}  // namespace mimium