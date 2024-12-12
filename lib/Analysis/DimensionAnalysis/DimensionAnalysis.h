#ifndef LIB_ANALYSIS_DIMENSIONANALYSIS_DIMENSIONANALYSIS_H_
#define LIB_ANALYSIS_DIMENSIONANALYSIS_DIMENSIONANALYSIS_H_

#include "mlir/include/mlir/Analysis/DataFlow/SparseAnalysis.h"  // from @llvm-project
#include "mlir/include/mlir/IR/Diagnostics.h"  // from @llvm-project
#include "mlir/include/mlir/IR/Operation.h"    // from @llvm-project
#include "mlir/include/mlir/IR/Value.h"        // from @llvm-project

namespace mlir {
namespace heir {

// This analysis should be used after --mlir-to-secret-arithmetic
// but before --secret-distribute-generic
// where a whole secret::GenericOp is assumed

class DimensionState {
 public:
  using DimensionType = int;

  DimensionState() : dimension(std::nullopt) {}
  explicit DimensionState(DimensionType dimension) : dimension(dimension) {}
  ~DimensionState() = default;

  DimensionType getDimension() const {
    assert(isInitialized());
    return dimension.value();
  }

  bool operator==(const DimensionState &rhs) const {
    return dimension == rhs.dimension;
  }

  bool isInitialized() const { return dimension.has_value(); }

  static DimensionState join(const DimensionState &lhs,
                             const DimensionState &rhs) {
    if (!lhs.isInitialized()) return rhs;
    if (!rhs.isInitialized()) return lhs;

    return DimensionState{std::max(lhs.getDimension(), rhs.getDimension())};
  }

  void print(llvm::raw_ostream &os) const {
    if (isInitialized()) {
      os << "DimensionState(" << dimension.value() << ")";
    } else {
      os << "DimensionState(uninitialized)";
    }
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const DimensionState &state) {
    state.print(os);
    return os;
  }

 private:
  std::optional<DimensionType> dimension;
};

class DimensionLattice : public dataflow::Lattice<DimensionState> {
 public:
  using Lattice::Lattice;
};

class DimensionAnalysis
    : public dataflow::SparseForwardDataFlowAnalysis<DimensionLattice> {
 public:
  using SparseForwardDataFlowAnalysis::SparseForwardDataFlowAnalysis;

  void setToEntryState(DimensionLattice *lattice) override {
    propagateIfChanged(lattice, lattice->join(DimensionState()));
  }

  LogicalResult visitOperation(Operation *op,
                               ArrayRef<const DimensionLattice *> operands,
                               ArrayRef<DimensionLattice *> results) override;
};

void annotateDimension(Operation *top, DataFlowSolver *solver);

}  // namespace heir
}  // namespace mlir

#endif  // LIB_ANALYSIS_DIMENSIONANALYSIS_DIMENSIONANALYSIS_H_