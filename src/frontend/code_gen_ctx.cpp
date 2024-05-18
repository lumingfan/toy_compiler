#include <iostream>
#include <cassert>

#include "frontend/code_gen_ctx.h"

namespace l24 {

CodeGenContext::CodeGenContext() {
    _context = std::make_unique<llvm::LLVMContext>();
    _module = std::make_unique<llvm::Module>("l24_module", *_context);
    _builder = std::make_unique<llvm::IRBuilder<>>(*_context);
}

void CodeGenContext::LogError(const std::string &str) {
    std::cerr << "Error: " << str << std::endl;
    exit(1);
}

void CodeGenContext::pushNamedValuesLayer() {
    _nested_named_values.emplace_back();
}

void CodeGenContext::popNamedValuesLayer() {
    _nested_named_values.pop_back();
}

void CodeGenContext::defineValue(const std::string &ident, L24Type::ValType ty, std::vector<llvm::Value*>vals, llvm::Value *array_size, bool is_ptr)  {
    assert(ty == L24Type::ValType::CONST || ty == L24Type::ValType::VAR);

    // this var/const is a global var/const
    if (_nested_named_values.empty()) {
        this->defineGlobalValue(ident, llvm::Type::getInt64Ty(*_context), vals, array_size);
        return ;
    }

    auto &named_values = getNamedValues(getCurrentLayer());

    if (named_values.count(ident) == 0) {
        switch (ty) {
        case L24Type::ValType::CONST: named_values[ident] = L24Type::ConstVal(this->createDefineValueInst(vals, ident, array_size, is_ptr)); break;
        case L24Type::ValType::VAR: named_values[ident] = L24Type::VarVal(this->createDefineValueInst(vals, ident, array_size, is_ptr)); break;
        default: LogError("you must specify var/const of this ident");
        }
    } else {
        LogError("this ident has been defined");
    }
}

void CodeGenContext::setValue(const std::string &ident, L24Type::ValType ty, llvm::Value *val, llvm::Value *sub_idx) {
    assert(ty == L24Type::ValType::CONST || ty == L24Type::ValType::VAR);

    int valid_layer = getCurrentLayer();
    for (; valid_layer >= 0; --valid_layer) {
        const auto &named_values = getNamedValues(valid_layer);
        if (named_values.count(ident) != 0) {
            break;
        }
    }
    if (valid_layer == -1) {
        setGlobalValue(ident, val, sub_idx);
        return ;
    }

    auto &named_values = getNamedValues(valid_layer);

    if (named_values.count(ident) == 0) {
        switch (ty) {
        case L24Type::ValType::CONST: named_values[ident] = L24Type::ConstVal(this->createDefineValueInst({val}, ident)); break;
        case L24Type::ValType::VAR: named_values[ident] = L24Type::VarVal(this->createDefineValueInst({val}, ident)); break;
        default: LogError("you must specify var/const of this ident");
        }
        return;
    }

    assert(ty == L24Type::ValType::VAR);
    try {
        this->createSetValueInst(std::get<L24Type::VarVal>(named_values[ident])._val, val, sub_idx);
    } catch (std::bad_variant_access const &ex) { LogError(ident + " is a const"); }
}

llvm::Value *CodeGenContext::getValue(const std::string &ident, L24Type::ValType ty, llvm::Value *sub_idx) {
    int valid_layer = getCurrentLayer();
    for (; valid_layer >= 0; --valid_layer) {
        const auto &named_values = getNamedValues(valid_layer);
        if (named_values.count(ident) != 0) {
            break;
        }
    }

    if (valid_layer == -1) {
        return getGlobalValue(ident, sub_idx);
    }

    auto &named_values = getNamedValues(valid_layer);
    const auto &var_val = named_values[ident];
    try {
        switch (ty) {
        case L24Type::ValType::VAR: return this->createGetValueInst(std::get<L24Type::VarVal>(var_val)._val, ident, sub_idx);
        default: return this->createGetValueInst(std::get<L24Type::ConstVal>(var_val)._val, ident, sub_idx);
        }
    } catch (std::bad_variant_access const &ex) {
        switch (ty) {
        case L24Type::ValType::CONST:
            LogError(std::string(ex.what()) + ": contained var , not const");
            break;
        case L24Type::ValType::VAR:
            LogError(std::string(ex.what()) + ": contained const, not var");
            break;
        case L24Type::ValType::ANY:
            return this->createGetValueInst(std::get<L24Type::VarVal>(var_val)._val, ident, sub_idx);
        }
    }
    return nullptr;
}

bool CodeGenContext::inCurrentLayer(const std::string &ident) {
    return getNamedValues(getCurrentLayer()).count(ident) != 0;
}

void CodeGenContext::codeGenStandardLibrary() const {
    // void getint();
    llvm::Type *int64_ty = llvm::Type::getInt64Ty(*_context);
    llvm::Type *void_ty = llvm::Type::getVoidTy(*_context);
    llvm::FunctionType *ft_getint =
        llvm::FunctionType::get(int64_ty, false);
    llvm::Function::Create(ft_getint, llvm::Function::ExternalLinkage, "getint", _module.get());

    // putint
    llvm::FunctionType *ft_putint =
        llvm::FunctionType::get(void_ty, {int64_ty}, false);
    llvm::Function::Create(ft_putint, llvm::Function::ExternalLinkage, "putint", _module.get());

    // getch
    llvm::FunctionType *ft_getch =
        llvm::FunctionType::get(int64_ty, false);
    llvm::Function::Create(ft_getch, llvm::Function::ExternalLinkage, "getch", _module.get());

    // putch
    llvm::FunctionType *ft_putch =
        llvm::FunctionType::get(void_ty, {int64_ty}, false);
    llvm::Function::Create(ft_putch, llvm::Function::ExternalLinkage, "putch", _module.get());

    llvm::PointerType *int64ptr_ty = llvm::PointerType::get(llvm::IntegerType::get(*(this->_context), 64), 0);

    // putarray
    llvm::FunctionType *ft_putarray =
        llvm::FunctionType::get(void_ty, {int64_ty, int64ptr_ty}, false);
    llvm::Function::Create(ft_putarray, llvm::Function::ExternalLinkage, "putarray", _module.get());

    // getarray
    llvm::FunctionType *ft_getarray =
        llvm::FunctionType::get(int64_ty, {int64ptr_ty}, false);
    llvm::Function::Create(ft_getarray, llvm::Function::ExternalLinkage, "getarray", _module.get());
}
void CodeGenContext::defineGlobalValue(const std::string &ident, llvm::Type *ty, std::vector<llvm::Value *>vals, llvm::Value *array_size) {
    if (_module->getGlobalVariable(ident) != nullptr) {
        CodeGenContext::LogError("redefine global var/const: " + ident);
    }

    if (array_size == nullptr) {
        _module->getOrInsertGlobal(ident, ty);
        auto constantInt =
            llvm::ConstantInt::getIntegerValue(ty,llvm::APInt(64, (llvm::dyn_cast<llvm::ConstantInt>(vals[0])->getSExtValue())));
        _module->getGlobalVariable(ident)->setInitializer(constantInt);
    } else {
        // array
        auto size = llvm::dyn_cast<llvm::ConstantInt>(array_size)->getSExtValue();
        auto array_type = llvm::ArrayType::get(ty, size);
        _module->getOrInsertGlobal(ident, array_type);

        std::vector<llvm::Constant*> init_vals_vec;
        for (int64_t i = 0; i < size; ++i)  {
            init_vals_vec.push_back(llvm::ConstantInt::getIntegerValue(ty,llvm::APInt(64, (llvm::dyn_cast<llvm::ConstantInt>(vals[i])->getSExtValue()))));
        }
        llvm::Constant* init = llvm::ConstantArray::get(array_type, init_vals_vec);
        _module->getGlobalVariable(ident)->setInitializer(init);
    }
}

void CodeGenContext::setGlobalValue(const std::string &ident, llvm::Value *val, llvm::Value *sub_idx) {
    if (_module->getGlobalVariable(ident) == nullptr) {
        CodeGenContext::LogError("global var/const: " + ident + " doesn't exist");
    }

    llvm::GlobalVariable* key = _module->getGlobalVariable(ident);
    llvm::Type *ty = key->getValueType();

    // scalar
    if (sub_idx == nullptr) {
        // set var/const in global domain
        if (_nested_named_values.empty()) {
            auto constantInt = llvm::ConstantInt::getIntegerValue(
                ty, llvm::APInt(
                        64, (llvm::dyn_cast<llvm::ConstantInt>(val)->getSExtValue())));
            key->setInitializer(constantInt);
            return;
        }
        _builder->CreateLoad(ty, key);
        _builder->CreateStore(val, key);
        return ;
    }

    // array
    // we don't support struct, so the first value of indexList always be 0
    auto first_val = llvm::ConstantInt::get(ty, 0);
    llvm::Value* indexList[2] = {first_val, sub_idx};
    auto ptr = this->_builder->CreateGEP(key->getValueType(), key, indexList);
    this->_builder->CreateStore(val, ptr);
}


llvm::Value *CodeGenContext::getGlobalValue(const std::string &ident, llvm::Value* sub_idx) {
    if (_module->getGlobalVariable(ident) == nullptr) {
        CodeGenContext::LogError("global var/const: " + ident + " doesn't exist");
    }
    llvm::GlobalVariable* key = _module->getGlobalVariable(ident);
    llvm::Type *ty = key->getValueType();

    // scalar
    if (sub_idx == nullptr) {
        // get var/const in global domain
        if (_nested_named_values.empty()) {
            return key->getInitializer();
        }
        return _builder->CreateLoad(ty, key);
    }

    // we don't support struct, so the first value of indexList always be 0
    auto first_val = llvm::ConstantInt::get(ty, 0);
    llvm::Value* indexList[2] = {first_val, sub_idx};
    auto ptr = this->_builder->CreateGEP(key->getValueType(), key, indexList);
    return this->_builder->CreateLoad(ty, ptr);
}

} // namespace l24
