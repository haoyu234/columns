#include <columns.h>
#include <gtest/gtest.h>

TEST(kind, intN)
{
    union T
    {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    };

    EXPECT_EQ(cl_COLUMN_INT8, COLUMN_TYPE(T, i8));
    EXPECT_EQ(cl_COLUMN_INT16, COLUMN_TYPE(T, i16));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, i32));
    EXPECT_EQ(cl_COLUMN_INT64, COLUMN_TYPE(T, i64));

    EXPECT_EQ(cl_COLUMN_UINT8, COLUMN_TYPE(T, u8));
    EXPECT_EQ(cl_COLUMN_UINT16, COLUMN_TYPE(T, u16));
    EXPECT_EQ(cl_COLUMN_UINT32, COLUMN_TYPE(T, u32));
    EXPECT_EQ(cl_COLUMN_UINT64, COLUMN_TYPE(T, u64));
}

TEST(kind, floatN)
{
    union T
    {
        float f;
        double d;
        #if __HAVE_FLOAT16
            _Float16 f16;
        #endif
        #if __HAVE_FLOAT32
            _Float32 f32;
        #endif
        #if __HAVE_FLOAT64
            _Float64 f64;
        #endif
        #if __HAVE_FLOAT128
            _Float128 f128;
        #endif
    };

    static int SIZE[] = {
        0,
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
        cl_COLUMN_FLOAT128
    };

    EXPECT_EQ(SIZE[sizeof(float)], COLUMN_TYPE(T, f));
    EXPECT_EQ(SIZE[sizeof(double)], COLUMN_TYPE(T, d));

    #if __HAVE_FLOAT16
        EXPECT_EQ(cl_COLUMN_FLOAT16, COLUMN_TYPE(T, f16));
    #endif

    #if __HAVE_FLOAT32
        EXPECT_EQ(cl_COLUMN_FLOAT32, COLUMN_TYPE(T, f32));
    #endif

    #if __HAVE_FLOAT64
        EXPECT_EQ(cl_COLUMN_FLOAT64, COLUMN_TYPE(T, f64));
    #endif

    #if __HAVE_FLOAT128
        EXPECT_EQ(cl_COLUMN_FLOAT128, COLUMN_TYPE(T, f128));
    #endif
}

TEST(kind, enum)
{
    union T
    {
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

    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v0));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v1));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v127));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v128));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v255));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v256));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v1024));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v65535));
    EXPECT_EQ(cl_COLUMN_INT32, COLUMN_TYPE(T, v65536));
}
