/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <unistd.h>

#include <csignal>
#include <fstream>
#include <string>

#include "basic/helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
using Logger = mimium::Logger;
#include "compiler/compiler.hpp"
#include "runtime/runtime.hpp"

extern "C" {

mimium::Scheduler* global_sch;
void addTask(double time, void* addresstofn,  double arg) {
  global_sch->addTask(time, addresstofn, arg, nullptr);
}
void addTask_cls(double time, void* addresstofn,  double arg,
             void* addresstocls) {
  global_sch->addTask(time, addresstofn, arg, addresstocls);
}
int myprint(double d) {
  std::cerr << d << std::endl;
  return 0;
}
// double asyncTaskCaller(double d,double(*fnptr)(double)){
//  auto future = std::async(std::launch::deferred,fnptr,d);
//  global_sch->addTask(time,future);//executeTask()でfuture.getする
//  future.wait();
//   return future.get();
// }

}

std::function<void(int)> shutdown_handler;
void signalHandler(int signo) { shutdown_handler(signo); }

auto main(int argc, char** argv) -> int {
  enum class CompileStage : int {
    AST = 0,
    AST_UNIQUENAME,
    TYPEINFOS,
    MIR,
    MIR_CC,
    LLVMIR,
    EXECUTE
  };
  int returncode=0;
  cl::OptionCategory general_category("General Options", "");
  cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"),
                                      cl::init("-"), cl::cat(general_category));

  cl::opt<CompileStage> compile_stage(
      cl::desc("Printing Debug Infomations"),
      cl::values(
          clEnumValN(CompileStage::AST, "emit-ast",
                     "Emit raw abstarct syntax tree to stdout"),
          clEnumValN(CompileStage::AST_UNIQUENAME, "emit-ast-u",
                     "Emit AST with unique symbol names to stdout"),
          clEnumValN(CompileStage::TYPEINFOS, "emit-types",
                     "emit type information for all variables to stdout"),
          clEnumValN(CompileStage::MIR, "emit-mir", "emit MIR to stdout"),
          clEnumValN(CompileStage::MIR_CC, "emit-mir-cc",
                     "emit MIR after closure convertsion to stdout"),

          clEnumValN(CompileStage::LLVMIR, "emit-llvm",
                     "emit LLVM IR to stdout")),
      cl::cat(general_category));
  compile_stage.setInitialValue(CompileStage::EXECUTE);
  cl::opt<bool> is_snd_file("sndfile",
                         cl::desc("write out a sound file as an output"));
  is_snd_file.setInitialValue(false);
  cl::ResetAllOptionOccurrences();

  cl::HideUnrelatedOptions(general_category);
  cl::ParseCommandLineOptions(argc, argv, "Mimium\n");  // launch cli helper

  std::ifstream input(input_filename.c_str());
  signal(SIGINT, signalHandler);
  Logger::current_report_level = Logger::INFO;
    auto runtime = std::make_shared<mimium::Runtime_LLVM>();
  auto compiler = std::make_unique<mimium::Compiler>(runtime->getLLVMContext());
  shutdown_handler = [&runtime](int /*signal*/) {
    if (runtime->isrunning()) {
      runtime->stop();
    }
    std::cerr << "Interuppted by key" << std::endl;
    exit(0);
  };

  runtime->addScheduler(is_snd_file);
  global_sch = runtime->getScheduler().get();
  if (!input.good()) {  
    Logger::debug_log("Specify file name, repl mode is not implemented yet",Logger::ERROR);
// filename is empty:enter repl mode
  } else {  // try to parse and exec input file
    try {
      std::string filename = input_filename.c_str();
      Logger::debug_log("Opening " + filename, Logger::INFO);
      compiler->setFilePath(filename);
      compiler->setDataLayout(runtime->getJitEngine().getDataLayout());

      auto stage = compile_stage.getValue();
      do {
        auto ast = compiler->loadSourceFile(filename);
        if (stage == CompileStage::AST) {
          std::cout << ast->toString() << std::endl;
          break;
        }
        auto ast_u = compiler->alphaConvert(ast);
        if (stage == CompileStage::AST_UNIQUENAME) {
          std::cout << ast_u->toString() << std::endl;
          break;
        }
        auto& typeinfos = compiler->typeInfer(ast_u);
        if (stage == CompileStage::TYPEINFOS) {
          std::cout << typeinfos.dump() << std::endl;
          break;
        }
        auto mir = compiler->generateMir(ast_u);
        if (stage == CompileStage::MIR) {
          std::cout << mir->toString() << std::endl;
          break;
        }
        auto mir_cc = compiler->closureConvert(mir);
        if (stage == CompileStage::MIR_CC) {
          std::cout << mir_cc->toString() << std::endl;
          break;
        }
        auto& llvm_module = compiler->generateLLVMIr(mir_cc);
        if (stage == CompileStage::LLVMIR) {
          llvm_module.print(llvm::outs(),nullptr, false, true);
          break;
        }
        runtime->executeModule(compiler->moveLLVMModule());
        runtime->start();//start() blocks thread until scheduler stops
        returncode = 0;
        break;
      } while (false);

    } catch (std::exception& e) {
      mimium::Logger::debug_log(e.what(), mimium::Logger::ERROR);
      runtime->stop();
      returncode=1;
    }
  } 
  std::cerr << "return code: " << returncode << std::endl;
  return returncode;
}
