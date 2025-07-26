#include "asm_ast.hpp"

using namespace asm_ast;

void AstOperandRegister::into_stream(std::ostream& os) const {
  os << "a0";
}

AstOperandImmediate::AstOperandImmediate(int64_t value) : value{value} {}

void AstOperandImmediate::into_stream(std::ostream& os) const {
  os << value;
}

AstInstructionMov::AstInstructionMov(std::unique_ptr<AstOperandRegister> src,
                                     std::unique_ptr<AstOperandRegister> dst)
    : source{std::move(src)}, destination{std::move(dst)} {}

void AstInstructionMov::into_stream(std::ostream& os) const {
  os << "  mv " << *destination << " " << *source;
}

AstInstructionLi::AstInstructionLi(std::unique_ptr<AstOperandImmediate> src,
                                   std::unique_ptr<AstOperandRegister> dst)
    : source{std::move(src)}, destination{std::move(dst)} {}

void AstInstructionLi::into_stream(std::ostream& os) const {
  os << "  li " << *destination << ", " << *source;
}

void AstInstructionRet::into_stream(std::ostream& os) const {
  os << "  ret";
}

void AstFunctionDef::into_stream(std::ostream& os) const {
  // Temporarily for now
  // Calls main, sets mscratch to 1, then calls debugger
  os << ".globl _start\n_start:\n  call main\n  csrrwi zero, mscratch, 1\n  "
        "ebreak\n\n# Compiled program\n";
  os << ".globl " << name << "\n" << name << ":\n";
  for (auto& instruction : instructions) {
    os << *instruction << "\n";
  }
}

AstProgram::AstProgram(AstFunctionDef&& fn) : function{std::move(fn)} {}

void AstProgram::into_stream(std::ostream& os) const {
  os << function << "\n";
  os << ".section .note.GNU-stack,\"\",@progbits\n";
}

std::ostream& operator<<(std::ostream& os, const IPrintable& p) {
  p.into_stream(os);
  return os;
}