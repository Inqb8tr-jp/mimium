/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "cli.hpp"

namespace {

using ak = mimium::app::cli::ArgKind;

const std::unordered_map<std::string_view, ak> str_to_argkind = {
    {"--emit-ast", ak::EmitAst},
    {"--emit-ast-u", ak::EmitAstUniqueSymbol},
    {"--emit-mir", ak::EmitMir},
    {"--emit-mir-cc", ak::EmitMirClosureCoverted},
    {"--emit-llvm", ak::EmitLLVMIR},
    {"--verbose", ak::Verbose},
    {"--version", ak::ShowVersion},
    {"--help", ak::ShowHelp},
    {"-h", ak::ShowHelp},
    {"-o", ak::Output},
    {"--output", ak::Output},
    {"--optimize", ak::OptimizeLevel},
    {"--backend", ak::BackEnd},
    {"--engine", ak::ExecutionEngine},
};

}  // namespace

namespace mimium::app::cli {

ArgKind getArgKind(std::string_view str) { return getEnumByStr(str_to_argkind, str); }

bool isArgPaired(ArgKind arg) {
  switch (arg) {
    case ak::ShowHelp:
    case ak::ShowVersion:
    case ak::Verbose: return false;
    default: return true;
  }
  return true;
}

std::ostream& CliApp::printHelp(std::ostream& out) const {
  app->printAbout(out);
  out <<
      R"(
Usage: mimium [options] <input file>

Options: 

  -o|--output [*.mmmast,*.mmmmir,*.ll] - Specify output filename.
  --optimize  [0,1(default)]           - Set Optimization Level.
  --engine    [llvm(default)]          - Set execution engine.
  --backend   [rtaudio(default)]       - Set Audio Backend.
  --version                            - Print a version number to stdout.
  -h|--help                            - Show this help.
)";
  if (app->getOption().is_verbose) {
    out <<
        R"(
Debug Informations (if no output file specified, emit to stdout.)
  --emit-ast    - Emit raw abstarct syntax tree
  --emit-ast-u  - Emit AST with unique symbol names
  --emit-types  - emit type information for all variables
  --emit-mir    - emit MIR
  --emit-mir-cc - emit MIR after closure convertsion
  --emit-llvm   - emit LLVM IR
)";
  }
  out.flush();
  return out;
}

std::vector<std::string_view> CliApp::OptionParser::initRawArgs(int argc, char** argv) {
  std::vector<std::string_view> args;
  args.resize(argc);
  for (int idx = 0; idx < argc; idx++) { args[idx] = argv[idx]; }
  return args;
}

void CliApp::OptionParser::processArgs(ArgKind arg, std::string_view val) {
  switch (arg) {
    case ak::Output: result.output_path = val; break;
    case ak::BackEnd: result.runtime_option.backend = getBackEnd(val); break;
    case ak::ExecutionEngine: result.runtime_option.engine = getExecutionEngine(val); break;
    case ak::EmitAst: result.compile_option.stage = CompileStage::Parse; break;
    case ak::EmitAstUniqueSymbol: result.compile_option.stage = CompileStage::SymbolRename; break;
    case ak::EmitMir: result.compile_option.stage = CompileStage::MirEmit; break;
    case ak::EmitMirClosureCoverted:
      result.compile_option.stage = CompileStage::ClosureConvert;
      break;
    case ak::EmitLLVMIR: result.compile_option.stage = CompileStage::Codegen; break;
    case ak::ShowVersion: res_mode = CliAppMode::ShowVersion; return;
    case ak::ShowHelp: res_mode = CliAppMode::ShowHelp; return;

    case ak::Verbose: result.is_verbose = true; return;
    case ak::Invalid:
    default: throw std::runtime_error("Unknown Option Type: " + std::string(val));
  }
}

bool CliApp::OptionParser::isArgOption(std::string_view str) { return str.substr(0, 2) == "--"; }

std::pair<AppOption, CliAppMode> CliApp::OptionParser::operator()(int argc, char** argv) {
  auto args = initRawArgs(argc, argv);
  auto iter = args.cbegin();

  std::advance(iter, 1);  // skip application name;
  while (iter != args.cend()) {
    const auto& a = *iter;
    if (isArgOption(a)) {
      auto kind = getArgKind(a);
      if (kind == ArgKind::Invalid) {
        Logger::debug_log("Unknown Argument Type: " + std::string(a), Logger::WARNING);
      }
      if (isArgPaired(kind)) { iter++; }
      processArgs(kind, *iter);
    } else {
      auto [path, type] = getFilePath(a);
      this->result.input_path = path;
      this->result.input_type = type;
    }
    iter++;
  }
  return std::pair(this->result, res_mode);
}

CliApp::CliApp(int argc, char** argv) : app(nullptr) {
  auto [option, mode] = OptionParser()(argc, argv);
  this->app = std::make_unique<GenericApp>(option);
  this->mode = mode;
}

int CliApp::run() {
  switch (this->mode) {
    case CliAppMode::Run: return this->app->run();
    case CliAppMode::ShowHelp: printHelp(); return 0;
    case CliAppMode::ShowVersion: this->app->printVersion(); return 0;
  }
}

}  // namespace mimium::app::cli