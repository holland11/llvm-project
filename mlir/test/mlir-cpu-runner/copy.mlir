// RUN: mlir-opt %s -convert-scf-to-std -convert-arith-to-llvm -convert-memref-to-llvm -convert-std-to-llvm -reconcile-unrealized-casts \
// RUN: | mlir-cpu-runner -e main -entry-point-result=void \
// RUN: -shared-libs=%mlir_runner_utils_dir/libmlir_runner_utils%shlibext,%mlir_runner_utils_dir/libmlir_c_runner_utils%shlibext \
// RUN: | FileCheck %s

func private @print_memref_f32(memref<*xf32>) attributes { llvm.emit_c_interface }

func @main() -> () {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index

  // Initialize input.
  %input = memref.alloc() : memref<2x3xf32>
  %dim_x = memref.dim %input, %c0 : memref<2x3xf32>
  %dim_y = memref.dim %input, %c1 : memref<2x3xf32>
  scf.parallel (%i, %j) = (%c0, %c0) to (%dim_x, %dim_y) step (%c1, %c1) {
    %prod = arith.muli %i,  %dim_y : index
    %val = arith.addi %prod, %j : index
    %val_i64 = arith.index_cast %val : index to i64
    %val_f32 = arith.sitofp %val_i64 : i64 to f32
    memref.store %val_f32, %input[%i, %j] : memref<2x3xf32>
  }
  %unranked_input = memref.cast %input : memref<2x3xf32> to memref<*xf32>
  call @print_memref_f32(%unranked_input) : (memref<*xf32>) -> ()
  // CHECK: rank = 2 offset = 0 sizes = [2, 3] strides = [3, 1]
  // CHECK-NEXT: [0,   1,   2]
  // CHECK-NEXT: [3,   4,   5]

  %copy = memref.alloc() : memref<2x3xf32>
  memref.copy %input, %copy : memref<2x3xf32> to memref<2x3xf32>
  %unranked_copy = memref.cast %copy : memref<2x3xf32> to memref<*xf32>
  call @print_memref_f32(%unranked_copy) : (memref<*xf32>) -> ()
  // CHECK: rank = 2 offset = 0 sizes = [2, 3] strides = [3, 1]
  // CHECK-NEXT: [0,   1,   2]
  // CHECK-NEXT: [3,   4,   5]

  %copy_two = memref.alloc() : memref<3x2xf32>
  %copy_two_casted = memref.reinterpret_cast %copy_two to offset: [0], sizes: [2, 3], strides:[1, 2]
    : memref<3x2xf32> to memref<2x3xf32, offset: 0, strides: [1, 2]>
  memref.copy %input, %copy_two_casted : memref<2x3xf32> to memref<2x3xf32, offset: 0, strides: [1, 2]>
  %unranked_copy_two = memref.cast %copy_two : memref<3x2xf32> to memref<*xf32>
  call @print_memref_f32(%unranked_copy_two) : (memref<*xf32>) -> ()
  // CHECK: rank = 2 offset = 0 sizes = [3, 2] strides = [2, 1]
  // CHECK-NEXT: [0,   3]
  // CHECK-NEXT: [1,   4]
  // CHECK-NEXT: [2,   5]

  %input_empty = memref.alloc() : memref<3x0x1xf32>
  %copy_empty = memref.alloc() : memref<3x0x1xf32>
  // Copying an empty shape should do nothing (and should not crash).
  memref.copy %input_empty, %copy_empty : memref<3x0x1xf32> to memref<3x0x1xf32>
  memref.dealloc %copy_empty : memref<3x0x1xf32>
  memref.dealloc %input_empty : memref<3x0x1xf32>
  memref.dealloc %copy_two : memref<3x2xf32>
  memref.dealloc %copy : memref<2x3xf32>
  memref.dealloc %input : memref<2x3xf32>
  return
}
