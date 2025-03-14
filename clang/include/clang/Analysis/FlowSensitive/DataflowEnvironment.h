//===-- DataflowEnvironment.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines an Environment class that is used by dataflow analyses
//  that run over Control-Flow Graphs (CFGs) to keep track of the state of the
//  program at given program points.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_DATAFLOWENVIRONMENT_H
#define LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_DATAFLOWENVIRONMENT_H

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeOrdering.h"
#include "clang/Analysis/FlowSensitive/DataflowAnalysisContext.h"
#include "clang/Analysis/FlowSensitive/DataflowLattice.h"
#include "clang/Analysis/FlowSensitive/StorageLocation.h"
#include "clang/Analysis/FlowSensitive/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

namespace clang {
namespace dataflow {

/// Indicates what kind of indirections should be skipped past when retrieving
/// storage locations or values.
///
/// FIXME: Consider renaming this or replacing it with a more appropriate model.
/// See the discussion in https://reviews.llvm.org/D116596 for context.
enum class SkipPast {
  /// No indirections should be skipped past.
  None,
  /// An optional reference should be skipped past.
  Reference,
};

/// Holds the state of the program (store and heap) at a given program point.
class Environment {
public:
  Environment(DataflowAnalysisContext &DACtx) : DACtx(&DACtx) {}

  bool operator==(const Environment &) const;

  LatticeJoinEffect join(const Environment &);

  /// Creates a storage location appropriate for `Type`. Does not assign a value
  /// to the returned storage location in the environment.
  ///
  /// Requirements:
  ///
  ///  `Type` must not be null.
  StorageLocation &createStorageLocation(QualType Type);

  /// Creates a storage location for `D`. Does not assign the returned storage
  /// location to `D` in the environment. Does not assign a value to the
  /// returned storage location in the environment.
  StorageLocation &createStorageLocation(const VarDecl &D);

  /// Creates a storage location for `E`. Does not assign the returned storage
  /// location to `E` in the environment. Does not assign a value to the
  /// returned storage location in the environment.
  StorageLocation &createStorageLocation(const Expr &E);

  /// Assigns `Loc` as the storage location of `D` in the environment.
  ///
  /// Requirements:
  ///
  ///  `D` must not be assigned a storage location in the environment.
  void setStorageLocation(const ValueDecl &D, StorageLocation &Loc);

  /// Returns the storage location assigned to `D` in the environment, applying
  /// the `SP` policy for skipping past indirections, or null if `D` isn't
  /// assigned a storage location in the environment.
  StorageLocation *getStorageLocation(const ValueDecl &D, SkipPast SP) const;

  /// Assigns `Loc` as the storage location of `E` in the environment.
  ///
  /// Requirements:
  ///
  ///  `E` must not be assigned a storage location in the environment.
  void setStorageLocation(const Expr &E, StorageLocation &Loc);

  /// Returns the storage location assigned to `E` in the environment, applying
  /// the `SP` policy for skipping past indirections, or null if `E` isn't
  /// assigned a storage location in the environment.
  StorageLocation *getStorageLocation(const Expr &E, SkipPast SP) const;

  /// Creates a value appropriate for `Type`, assigns it to `Loc`, and returns
  /// it, if `Type` is supported, otherwise return null. If `Type` is a pointer
  /// or reference type, creates all the necessary storage locations and values
  /// for indirections until it finds a non-pointer/non-reference type.
  ///
  /// Requirements:
  ///
  ///  `Type` must not be null.
  Value *initValueInStorageLocation(const StorageLocation &Loc, QualType Type);

  /// Assigns `Val` as the value of `Loc` in the environment.
  void setValue(const StorageLocation &Loc, Value &Val);

  /// Returns the value assigned to `Loc` in the environment or null if `Loc`
  /// isn't assigned a value in the environment.
  Value *getValue(const StorageLocation &Loc) const;

  /// Equivalent to `getValue(getStorageLocation(D, SP), SkipPast::None)` if `D`
  /// is assigned a storage location in the environment, otherwise returns null.
  Value *getValue(const ValueDecl &D, SkipPast SP) const;

  /// Equivalent to `getValue(getStorageLocation(E, SP), SkipPast::None)` if `E`
  /// is assigned a storage location in the environment, otherwise returns null.
  Value *getValue(const Expr &E, SkipPast SP) const;

  /// Transfers ownership of `Loc` to the analysis context and returns a
  /// reference to `Loc`.
  StorageLocation &takeOwnership(std::unique_ptr<StorageLocation> Loc);

  /// Transfers ownership of `Val` to the analysis context and returns a
  /// reference to `Val`.
  Value &takeOwnership(std::unique_ptr<Value> Val);

private:
  /// Returns the value assigned to `Loc` in the environment or null if `Type`
  /// isn't supported.
  ///
  /// Recursively initializes storage locations and values until it sees a
  /// self-referential pointer or reference type. `Visited` is used to track
  /// which types appeared in the reference/pointer chain in order to avoid
  /// creating a cyclic dependency with self-referential pointers/references.
  ///
  /// Requirements:
  ///
  ///  `Type` must not be null.
  Value *initValueInStorageLocationUnlessSelfReferential(
      const StorageLocation &Loc, QualType Type,
      llvm::DenseSet<QualType> &Visited);

  StorageLocation &skip(StorageLocation &Loc, SkipPast SP) const;
  const StorageLocation &skip(const StorageLocation &Loc, SkipPast SP) const;

  // `DACtx` is not null and not owned by this object.
  DataflowAnalysisContext *DACtx;

  // Maps from program declarations and statements to storage locations that are
  // assigned to them. Unlike the maps in `DataflowAnalysisContext`, these
  // include only storage locations that are in scope for a particular basic
  // block.
  llvm::DenseMap<const ValueDecl *, StorageLocation *> DeclToLoc;
  llvm::DenseMap<const Expr *, StorageLocation *> ExprToLoc;

  llvm::DenseMap<const StorageLocation *, Value *> LocToVal;

  // FIXME: Add flow condition constraints.
};

} // namespace dataflow
} // namespace clang

#endif // LLVM_CLANG_ANALYSIS_FLOWSENSITIVE_DATAFLOWENVIRONMENT_H
