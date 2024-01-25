#include <CUnit/CUnit.h>
#include <CUnit/Console.h>

#define USE_COLUMN_MACROS
#include <columns.h>

void testINTN() {
  union T {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
  };

  CU_ASSERT_EQUAL(cl_COLUMN_INT8, COLUMN_TYPE(union T, i8));
  CU_ASSERT_EQUAL(cl_COLUMN_INT16, COLUMN_TYPE(union T, i16));
  CU_ASSERT_EQUAL(cl_COLUMN_INT32, COLUMN_TYPE(union T, i32));
  CU_ASSERT_EQUAL(cl_COLUMN_INT64, COLUMN_TYPE(union T, i64));

  CU_ASSERT_EQUAL(cl_COLUMN_UINT8, COLUMN_TYPE(union T, u8));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT16, COLUMN_TYPE(union T, u16));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, u32));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT64, COLUMN_TYPE(union T, u64));
}

void testFLOATN() {
  union T {
    float f;
    double d;
    long double ld;
  };

  static int SIZE[] = {0,
                       cl_COLUMN_FLOAT8,
                       cl_COLUMN_FLOAT16,
                       0,
                       cl_COLUMN_FLOAT32,
                       0,
                       0,
                       0,
                       cl_COLUMN_FLOAT64,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       cl_COLUMN_FLOAT128};

  CU_ASSERT_EQUAL(SIZE[sizeof(float)], COLUMN_TYPE(union T, f));
  CU_ASSERT_EQUAL(SIZE[sizeof(double)], COLUMN_TYPE(union T, d));
  CU_ASSERT_EQUAL(SIZE[sizeof(long double)], COLUMN_TYPE(union T, ld));
}

void testENUM() {
  union T {
    enum { V0 = 0 } v0;
    enum { V1 = 1 } v1;
    enum { V127 = 127 } v127;
    enum { V128 = 128 } v128;
    enum { V255 = 255 } v255;
    enum { V256 = 256 } v256;
    enum { V1024 = 1024 } v1024;
    enum { V65535 = 65535 } v65535;
    enum { V65536 = 65536 } v65536;
  };

  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v0));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v1));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v127));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v128));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v255));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v256));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v1024));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v65535));
  CU_ASSERT_EQUAL(cl_COLUMN_UINT32, COLUMN_TYPE(union T, v65536));
}

static CU_TestInfo tests[] = {{"intN", testINTN},
                              {"floatN", testFLOATN},
                              {"enum", testENUM},
                              CU_TEST_INFO_NULL};

static CU_SuiteInfo suites[] = {{"kind", NULL, NULL, NULL, NULL, tests},
                                CU_SUITE_INFO_NULL};

int main() {
  CU_ErrorCode error = CU_initialize_registry();
  if (error) {
    return 1;
  }

  error = CU_register_suites(suites);
  if (error) {
    return 1;
  }

  CU_console_run_tests();
  CU_cleanup_registry();
  return 0;
}