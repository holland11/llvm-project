//===-------------- UnsupportedInstructionView.h ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \brief
///
/// This file implements an unsupported instruction view for the llvm-mca tool.
///
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_MCA_UNSUPPORTEDINSTRUCTIONVIEW_H
#define LLVM_TOOLS_LLVM_MCA_UNSUPPORTEDINSTRUCTIONVIEW_H

#include "Views/InstructionView.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace mca {

/// This class is used whenever the --allow-unsupported-instructions command
/// line argument is used with mca. This View will print out a list of all
/// instruction in the source whose scheduling class is invalid and has
/// therefore been converted to not cause an error within mca.
class UnsupportedInstructionView : public InstructionView {
  const std::vector<std::unique_ptr<mca::Instruction>> &Insts;

public:
  UnsupportedInstructionView(
      const llvm::MCSubtargetInfo &STI, llvm::MCInstPrinter &Printer,
      llvm::ArrayRef<llvm::MCInst> S,
      std::vector<std::unique_ptr<mca::Instruction>> &Insts);

  /// Inspect every instruction in the source to determine if any were
  /// unsupported. If there is at least one, print the list of them and
  /// notify the user how they have been converted. If there are none,
  /// print a message saying that there weren't any.
  void printView(llvm::raw_ostream &OS) const override;
  StringRef getNameAsString() const override {
    return "UnsupportedInstructionView";
  }
};
} // namespace mca
} // namespace llvm

#endif
