#!/usr/bin/env bash

# this compiles the file once
goto-cc --function s2n_record_writev_harness s2n_record_writev.c a.out

# There’s no contracts involved at all anymore, the file is self contained.
# I think there’s an interaction of three different things:
# - first, two pointers are packed in a same `struct s2n_hash_state`. 
# One of
# them gets havoced in record_writev
# ```
#   mac->inner.hash_impl = nondet_s2n_hash();
# ```
#  and the other one gets checked for __CPROVER_rw_ok.
# ```
#   assert(
#     !s2n_is_in_fips_mode() ||
#     __CPROVER_rw_ok(
#       mac->inner.digest.high_level.evp.ctx->digest,
#       sizeof(*(mac->inner.digest.high_level.evp.ctx->digest))));
# ```
# I suspect the havoc of mac->inner.hash_impl is also havocing
# `mac->inner.digest.high_level.evp.ctx->digest`.
#
# because removing the havoc solves the performance problem on the rw_ok check

# The second problem is the very slight change of form of the rw_ok assertion before
# and after running the model through goto-instrument without any options.
# just loading a.out in goto instrument seems to propagate constants and push
# negations to the leaves.
# A third problem could be the lack of common subexpression sharing between POs,
# specifically on a dereference operation applied to a byte_extract applied to a
# union, which is itself a nondet pointer with a possibly large value set.
# A fourth problem could be a large difference in size of the SAT encoding of
# strict and non-strict arith comparisons ( < vs >= )

########################################
# b.out becomes slightly different
########################################
goto-instrument a.out b.out

# Model a.out can be analysed successfully but model b.out cannot.
#
# goto-diff -i a.out b.out shows only this difference:
#
# // in file a.out
# ASSERT
#  ¬(s2n_record_writev::$tmp::return_value_s2n_is_in_fips_mode ≠ 0) ? true :
# (
#     (false ∨ ¬(pointer_object(digest) = pointer_object(NULL))) ∧
#     (false ∨ ¬(pointer_object(digest) = pointer_object(__CPROVER_deallocated))) ∧
#     (false ∨ ¬(pointer_object(digest) = pointer_object(__CPROVER_dead_object))) ∧
#     ¬(is_invalid_pointer(digest)) ∧
#     (false ∨ ¬(pointer_offset(digest) < 0 ∨ cast(pointer_offset(digest), unsignedbv[64]) + 32 > object_size(digest))) ∧
#     ((pointer_object(NULL) = pointer_object(digest) ∧ NULL ≠ digest) ⇒ false) ? true : false
# )

# // in file b.out
# ASSERT
#  ¬(s2n_record_writev::$tmp::return_value_s2n_is_in_fips_mode ≠ 0) ? true :
# (
#     ¬(pointer_object(digest) = pointer_object(NULL)) ∧
#     ¬(pointer_object(digest) = pointer_object(__CPROVER_deallocated)) ∧
#     ¬(pointer_object(digest) = pointer_object(__CPROVER_dead_object)) ∧
#     ¬(is_invalid_pointer(digest)) ∧
#     pointer_offset(digest) ≥ 0 ∧
#     object_size(digest) ≥ cast(pointer_offset(digest), unsignedbv[64]) + 32 ∧
#     (¬(pointer_object(NULL) = pointer_object(digest)) ∨ digest = NULL) ? true : false
# )

# Where `digest` is the following expression:
# ```
#  *(byte_extract_little_endian(*s2n_record_writev::1::mac.inner.digest, 0, struct tag-s2n_hash_evp_digest).evp.ctx).digest
# ```
# I have named and factored myself the expression, but it occurs inline 8 times in the check expressions.
# My impression is that the dereference expression applies to a nondet pointer-typed expression.

# Pointer-primitive checks are solved relatively fast on a.out and consume 12gigs of ram
cbmc --pointer-primitive-check a.out
# CBMC version 5.70.0 (cbmc-5.70.0-94-g45651af8f6-dirty) 64-bit x86_64 linux
# Reading GOTO program from file a.out
# Generating GOTO Program
# Adding CPROVER library (x86_64)
# Removal of function pointers and virtual functions
# Generic Property Instrumentation
# Running with 8 object bits, 56 offset bits (default)
# Starting Bounded Model Checking
# Runtime Symex: 3.67663s
# size of program expression: 31543 steps
# simple slicing removed 7 assignments
# Generated 5 VCC(s), 5 remaining after simplification
# Runtime Postprocess Equation: 0.00505656s
# Passing problem to propositional reduction
# converting SSA
# Runtime Convert SSA: 22.6952s
# Running propositional reduction
# Post-processing
# Runtime Post-process: 2.01085s
# Solving with CaDiCaL 1.4.1
# 14562845 variables, 72102010 clauses
# SAT checker: instance is UNSATISFIABLE
# Runtime Solver: 12.6473s
# Runtime decision procedure: 35.3457s
# ** 0 of 7 failed (1 iterations)
# VERIFICATION SUCCESSFUL

# Activating --pointer-primitive-check on b.out generates the same 5 extra POs as on a.out.
# But the CNF size and memory consumption is x2.17 compared to a.out
# I suspect that the increase in CNF size between a.out and b.out could be due
# to relational operators encoding < vs >= and
# The sheer size itself could be due to the lack of common subexpression elimination
# on the byte_extract expression combined with the fact that this pointer dereference
# expression is nondeterministic with a large value set.
#
#  *(byte_extract_little_endian(*s2n_record_writev::1::mac.inner.digest, 0, struct tag-s2n_hash_evp_digest).evp.ctx).digest


# Uses 25gigs of ram and much longer
cbmc --pointer-primitive-check b.out
# ▶cbmc --pointer-primitive-check  b.out
# CBMC version 5.70.0 (cbmc-5.70.0-94-g45651af8f6-dirty) 64-bit x86_64 linux
# Reading GOTO program from file b.out
# Generating GOTO Program
# Adding CPROVER library (x86_64)
# Removal of function pointers and virtual functions
# Generic Property Instrumentation
# Running with 8 object bits, 56 offset bits (default)
# Starting Bounded Model Checking
# Runtime Symex: 4.66708s
# size of program expression: 31563 steps
# simple slicing removed 7 assignments
# Generated 5 VCC(s), 5 remaining after simplification
# Runtime Postprocess Equation: 0.00515586s
# Passing problem to propositional reduction
# converting SSA
# Runtime Convert SSA: 51.0316s
# Running propositional reduction
# Post-processing
# Runtime Post-process: 8.13558s
# Solving with CaDiCaL 1.4.1
# 31605581 variables, 156569186 clauses
# SAT checker: instance is UNSATISFIABLE
# Runtime Solver: 77.6588s
# Runtime decision procedure: 128.694s
# ** 0 of 7 failed (1 iterations)
# VERIFICATION SUCCESSFUL


# pointer checks are solved on a.out
cbmc --pointer-check a.out
# CBMC version 5.70.0 (cbmc-5.70.0-94-g45651af8f6-dirty) 64-bit x86_64 linux
# Reading GOTO program from file a.out
# Generating GOTO Program
# Adding CPROVER library (x86_64)
# Removal of function pointers and virtual functions
# Generic Property Instrumentation
# Running with 8 object bits, 56 offset bits (default)
# Starting Bounded Model Checking
# Runtime Symex: 3.3263s
# size of program expression: 31547 steps
# simple slicing removed 7 assignments
# Generated 445 VCC(s), 19 remaining after simplification
# Runtime Postprocess Equation: 0.00516588s
# Passing problem to propositional reduction
# converting SSA
# Runtime Convert SSA: 11.1785s
# Running propositional reduction
# Post-processing
# Runtime Post-process: 0.394296s
# Solving with CaDiCaL 1.4.1
# 6900635 variables, 33618359 clauses
# SAT checker: instance is UNSATISFIABLE
# Runtime Solver: 5.19706s
# Runtime decision procedure: 16.379s
# ** 0 of 243 failed (1 iterations)
# VERIFICATION SUCCESSFUL

# pointer checks on b.out cannot be solved
# blows up >128 gigs, do not run without monitoring
cbmc --pointer-check b.out
# CBMC version 5.70.0 (cbmc-5.70.0-94-g45651af8f6-dirty) 64-bit x86_64 linux
# Reading GOTO program from file b.out
# Generating GOTO Program
# Adding CPROVER library (x86_64)
# Removal of function pointers and virtual functions
# Generic Property Instrumentation
# Running with 8 object bits, 56 offset bits (default)
# Starting Bounded Model Checking
# Runtime Symex: 23.0768s
# size of program expression: 31991 steps
# simple slicing removed 7 assignments
# Generated 529 VCC(s), 103 remaining after simplification
# Runtime Postprocess Equation: 0.00661826s
# Passing problem to propositional reduction
# converting SSA

# Weirdly, there are more pointer-checks generated by b.out (529 vs 445), which
# could be due to the slight difference on program expressions caused by the
# simplifications applied by goto-instrument between a.out and b.out

