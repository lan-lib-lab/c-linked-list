#ifndef CUNIT_UTILS_H
#define CUNIT_UTILS_H

#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include <stdbool.h>
#include <inttypes.h>

/// attempts to add suite to registry. reports failure otherwise and clears registry to prevent memory leaks in CUnit lib
/// returns CU_get_error() in case of failure, else CUE_SUCCESS.
CU_ErrorCode CUU_utils_try_add_suite(CU_pSuite *out, const char *suite_name);

/// attempts to add a test to a suite. reports failure otherwise and clears registry to prevent memory leaks in CUnit lib
/// returns CU_get_error() in case of failure, else CUE_SUCCESS.
CU_ErrorCode CUU_utils_try_add_test(CU_pSuite suite, void (*test_callback)(void), const char *msg);

bool CUU_impl_assert_eq_u32(uint32_t actual, uint32_t expected, const char *actual_expr, const char *expected_expr, int line, const char *func_name, const char *file);

/* asserts redefined for better usage:
    - not ignoring return value of assertion
    - better error messages of errors
*/

#define CUU_ASSERT(cond) \
    CU_assertImplementation((cond), __LINE__, "CUU_ASSERT(" #cond ")", __FILE__, __func__, /*FATAL*/ false)

#define CUU_ASSERT_PTR_NULL(ptr) \
    CU_assertImplementation((NULL == (void*)(ptr)), __LINE__, "CUU_ASSERT_PTR_NULL(" #ptr ")", __FILE__, __func__, /*FATAL*/ false)

#define CUU_ASSERT_PTR_NOT_NULL(ptr) \
    CU_assertImplementation((NULL != (void*)(ptr)), __LINE__, "CUU_ASSERT_PTR_NULL(" #ptr ")", __FILE__, __func__, /*FATAL*/ false)

#define CUU_ASSERT_EQ_U32(actual, expected) \
    CUU_impl_assert_eq_u32((actual), (expected), #actual, #expected, __LINE__, __func__, __FILE__)

#endif // header guard
