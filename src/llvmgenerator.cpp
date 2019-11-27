#include "llvmgenerator.hpp"
namespace mimium {
LLVMGenerator::LLVMGenerator(std::string _filename) {
  builder = std::make_unique<llvm::IRBuilder<>>(ctx);
  module = std::make_shared<llvm::Module>(_filename, ctx);
  builtinfn = std::make_shared<LLVMBuiltin>();
}
// LLVMGenerator::LLVMGenerator(llvm::LLVMContext& _ctx,std::string _filename){
//     // ctx.reset();
//     // ctx = std::move(&_ctx);
// }

LLVMGenerator::~LLVMGenerator() {
  namemap.clear();
  for (auto& f : module->getFunctionList()) {
    for (auto& bb : f.getBasicBlockList()) {
      bb.eraseFromParent();
    }
    delete &f;
  }
}

std::shared_ptr<llvm::Module> LLVMGenerator::getModule() {
  if (module) {
    return module;
  } else {
    return std::make_shared<llvm::Module>("null", ctx);
  }
}
llvm::Type* LLVMGenerator::getRawStructType(types::Value& type) {
  types::Struct s = std::get<recursive_wrapper<types::Struct>>(type);
  std::vector<llvm::Type*> field;
  for (auto& a : s.arg_types) {
    field.push_back(getType(a));
  }

  llvm::Type* structtype = llvm::StructType::create(ctx, field,"fvtype");
  return structtype;
}
llvm::Type* LLVMGenerator::getType(types::Value type) {
  return std::visit(
      overloaded{
          [this](types::Float f) { return llvm::Type::getDoubleTy(ctx); },
          [this](recursive_wrapper<types::Function> rf) {
            auto f = (types::Function)rf;
            std::vector<llvm::Type*> args;
            auto* rettype = getType(f.getReturnType());
            for (auto& a : f.getArgTypes()) {
              auto* atype = getType(a);
              args.push_back(atype);
            }
            // upcast
            llvm::Type* res = llvm::FunctionType::get(
                rettype, args, false);  // what is final parameter isVarArg??
            return res;
          },
          [this](recursive_wrapper<types::Struct> rs) {
            auto s = (types::Struct)rs;
            std::vector<llvm::Type*> field;
            for (auto& a : s.arg_types) {
              field.push_back(getType(a));
            }
            llvm::Type* structtype =
                llvm::PointerType::get(llvm::StructType::create(ctx, field,"fvtype"), 0);
            return structtype;
          },
          [this](auto t) {
            throw std::runtime_error("not a function");
            return llvm::Type::getDoubleTy(ctx);
            ;
          }},
      type);
}

void LLVMGenerator::setBB(std::shared_ptr<llvm::BasicBlock> newblock) {
  builder->SetInsertPoint(newblock.get());
}
void LLVMGenerator::preprocess() {
  auto* fntype = llvm::FunctionType::get(llvm::Type::getInt64Ty(ctx), false);
  auto* mainfun = llvm::Function::Create(
      fntype, llvm::Function::ExternalLinkage, "main", module.get());
  std::unique_ptr<llvm::BasicBlock> ptr(
      llvm::BasicBlock::Create(ctx, "entry", mainfun));
  mainentry = std::move(ptr);
  setBB(mainentry);
}

void LLVMGenerator::generateCode(std::shared_ptr<MIRblock> mir) {
  preprocess();
  for (auto& inst : mir->instructions) {
    visitInstructions(inst);
  }
}

void LLVMGenerator::visitInstructions(Instructions& inst) {
  std::visit(
      overloaded{
          [](auto i) {},
          [&, this](std::shared_ptr<NumberInst> i) {
            auto* ptr =
                builder->CreateAlloca(llvm::Type::getDoubleTy(ctx), nullptr);
            auto* finst =
                llvm::ConstantFP::get(this->ctx, llvm::APFloat(i->val));
            builder->CreateStore(std::move(finst), std::move(ptr));
            auto* load = builder->CreateLoad(ptr, i->lv_name);
            namemap.emplace(i->lv_name, load);
          },
          [&, this](std::shared_ptr<OpInst> i) {
            llvm::Value* ptr;
            switch (i->getOPid()) {
              case ADD:
                ptr = builder->CreateFAdd(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case SUB:
                ptr = builder->CreateFSub(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case MUL:
                ptr = builder->CreateFMul(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              case DIV:
                ptr = builder->CreateFDiv(namemap[i->lhs], namemap[i->rhs],
                                          i->lv_name);
                break;
              default:
                break;
            }
            namemap.emplace(i->lv_name, ptr);
          },
          [&, this](std::shared_ptr<FunInst> i) {
            bool hasfv = i->freevariables.size() > 0;
            auto* ft = static_cast<llvm::FunctionType*>(getType(i->type));
            llvm::Function* f = llvm::Function::Create(
                ft, llvm::Function::ExternalLinkage, i->lv_name, module.get());
            int idx = 0;
            for (auto& arg : f->args()) {
              if (idx < i->args.size()) {
                arg.setName(i->args[idx++]);
              }
            }
            if (i->freevariables.size() > 0) {
              auto it = f->args().end();
              (--it)->setName("clsarg_" + i->lv_name);
            }
            namemap.emplace(i->lv_name, f);

            auto* bb = llvm::BasicBlock::Create(ctx, "entry", f);
            builder->SetInsertPoint(bb);
            idx = 0;
            for (auto& arg : f->args()) {
              namemap.emplace(i->args[idx], &arg);
              idx++;
            }
            auto arg_end = f->arg_end();
            llvm::Value* lastarg = --arg_end;
            for (int id = 0; id < i->freevariables.size(); id++) {
              std::string newname = "fv_" + i->freevariables[id].name;
              llvm::Value* gep = builder->CreateStructGEP(lastarg, id, "fv");
              llvm::Value* load = builder->CreateLoad(gep, newname);
              auto [it, res] = namemap.try_emplace(newname, load);
            }
            for (auto& cinsts : i->body->instructions) {
              visitInstructions(cinsts);
            }
            setBB(mainentry);
          },
          [&, this](std::shared_ptr<MakeClosureInst> i) {
            auto it = static_cast<llvm::Function*>(namemap[i->fname])->arg_end();
            llvm::Type* strtype =  static_cast<llvm::PointerType*>((--it)->getType())->getElementType();
            llvm::Value* cap_size =
                builder->CreateAlloca(strtype, nullptr, i->lv_name);
            int idx = 0;
            for (auto& cap : i->captures) {
              llvm::Value* gep =
                  builder->CreateStructGEP(strtype, cap_size, idx, "");
              llvm::Value* store = builder->CreateStore(namemap[cap.name], gep);
              idx++;
            }
            namemap.emplace(i->lv_name, cap_size);
          },
          [&, this](std::shared_ptr<FcallInst> i) {
            llvm::Value* res;
            std::vector<llvm::Value*> args;
            for (auto& a : i->args) {
              args.push_back(namemap[a]);
              namemap[a]->dump();
            }
            if (i->ftype == CLOSURE) {
              llvm::Value* cls = namemap[i->fname+"$cls"];
              args.push_back(cls);
              cls->dump();
            }
            if (LLVMBuiltin::isBuiltin(i->fname)) {
              auto it = LLVMBuiltin::builtin_fntable.find(i->fname);
              builtintype fn = it->second;
              std::vector<llvm::Value*> arg;
              res = fn(args, i->fname, shared_from_this());
            } else {
              llvm::Function* fun = module->getFunction(i->fname);
              if (!fun)
                throw std::runtime_error("function could not be referenced");
              fun->dump();
              res = builder->CreateCall(fun, args, i->lv_name);
            }
            namemap.emplace(i->lv_name, std::move(res));
          },
          [&, this](std::shared_ptr<ReturnInst> i) {
            builder->CreateRet(namemap[i->val]);
          }},
      inst);
}

void LLVMGenerator::outputToStream(llvm::raw_ostream& stream) {
  module->print(stream, nullptr, false, true);
}

}  // namespace mimium