#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace asm_ast {

struct IPrintable {
  virtual ~IPrintable() = default;
  virtual void into_stream(std::ostream& os) const = 0;
};

struct AstOperand : IPrintable {};

struct AstOperandRegister : AstOperand {
  void into_stream(std::ostream& os) const override;
};

struct AstOperandImmediate : AstOperand {
  AstOperandImmediate(int64_t value);

  int64_t value;

  void into_stream(std::ostream& os) const;
};

struct AstInstruction : IPrintable {};

struct AstInstructionMov : AstInstruction {
  AstInstructionMov(std::unique_ptr<AstOperandRegister> src,
                    std::unique_ptr<AstOperandRegister> dst);

  std::unique_ptr<AstOperandRegister> source;
  std::unique_ptr<AstOperandRegister> destination;

  void into_stream(std::ostream& os) const override;
};

struct AstInstructionLi : AstInstruction {
  AstInstructionLi(std::unique_ptr<AstOperandImmediate> src,
                   std::unique_ptr<AstOperandRegister> dst);

  std::unique_ptr<AstOperandImmediate> source;
  std::unique_ptr<AstOperandRegister> destination;

  void into_stream(std::ostream& os) const override;
};

struct AstInstructionRet : AstInstruction {
  void into_stream(std::ostream& os) const override;
};

struct AstFunctionDef : IPrintable {
  std::string name;
  std::vector<std::unique_ptr<AstInstruction>> instructions;

  void into_stream(std::ostream& os) const override;
};

struct AstProgram : IPrintable {
  AstProgram(AstFunctionDef&& fn);

  AstFunctionDef function;

  void into_stream(std::ostream& os) const override;
};

} // namespace asm_ast

std::ostream& operator<<(std::ostream& os, const asm_ast::IPrintable& p);