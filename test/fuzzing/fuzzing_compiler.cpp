#include "compiler/compiler.hpp"
#include "runtime/JIT/runtime_jit.hpp"

extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* data, std::size_t size) {
  std::string src((char*)data, size);
  std::ofstream dummy("/dev/null", std::ios::out | std::ios::app);
  mimium::Logger::setoutput(dummy);
  src += "\0";
  try {
    auto runtime = std::make_shared<mimium::Runtime_LLVM>();
    auto compiler = std::make_unique<mimium::Compiler>(runtime->getLLVMContext());

    compiler->setFilePath("dummy.mmm");
    compiler->setDataLayout(runtime->getJitEngine().getDataLayout());
    auto ast = compiler->loadSource(src);
    if (ast != nullptr && !ast->empty()) {
      auto ast_u = compiler->renameSymbols(std::move(ast));
      auto typeenv = compiler->typeInfer(ast_u);
      auto mir = compiler->generateMir(ast_u);
      auto mircc = compiler->closureConvert(mir);
      auto memobjs = compiler->collectMemoryObjs(mircc);
      auto& llvmir = compiler->generateLLVMIr(mircc,memobjs);
    }
  } catch (std::runtime_error& e) {
    // std::cerr << e.what() << std::endl;
  }
  return 0;
}