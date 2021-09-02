//===------------- UnsupportedInstructionView.cpp ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \brief
///
/// This file implements the UnsupportedInstructionView interface.
///
//===----------------------------------------------------------------------===//

#include "Views/UnsupportedInstructionView.h"

namespace llvm {
namespace mca {

UnsupportedInstructionView::UnsupportedInstructionView(
    const MCSubtargetInfo &STI, MCInstPrinter &Printer,
    llvm::ArrayRef<llvm::MCInst> S,
    std::vector<std::unique_ptr<mca::Instruction>> &Insts)
    : InstructionView(STI, Printer, S), Insts(Insts) {}

void UnsupportedInstructionView::printView(llvm::raw_ostream &OS) const {
  formatted_raw_ostream FOS(OS);
  FOS << "\n\nUnsupported Instructions:\n";
  FOS.flush();
  const MCSchedModel &SM = getSubTargetInfo().getSchedModel();
  int Index = 0;
  bool FoundUnsupported = false;
  for (const std::unique_ptr<Instruction> &Inst : Insts) {
    const MCInstrDesc &MCDesc = MCII.get(Inst->getOpcode());
    unsigned SchedClassID = MCDesc.getSchedClass();
    bool IsVariant = SM.getSchedClassDesc(SchedClassID)->isVariant();

    // Try to solve variant scheduling classes.
    if (IsVariant) {
      unsigned CPUID = SM.getProcessorID();
      while (SchedClassID && SM.getSchedClassDesc(SchedClassID)->isVariant())
        SchedClassID =
            STI.resolveVariantSchedClass(SchedClassID, &MCI, &MCII, CPUID);

      assert(SchedClassID && "This should be caught earlier within InstrBuilder which should cause an error.")
    }
    
    const MCSchedClassDesc &SCDesc =
        *SM.getSchedClassDesc(Inst->getDesc().SchedClassID);
    
    if (SCDesc.NumMicroOps == MCSchedClassDesc::InvalidNumMicroOps) {
      FoundUnsupported = true;
      FOS << "[" << Index << "]";
      FOS << "  " << printInstructionString(getSource()[Index]) << '\n';
      FOS.flush();
    }
    
    ++Index;
  }
  if (!FoundUnsupported) {
    FOS << "No unsupported instructions in the input sequence.\n";
    FOS.flush();
  } else {
    FOS << "(These instructions use an invalid scheduling class.\n"
        << " They have been given default values of 1 latency,\n"
        << " 1 uOp, 0 hardware units, 0 reads, and 0 defs.)\n";
    FOS.flush();
  }
}
} // namespace mca
} // namespace llvm
