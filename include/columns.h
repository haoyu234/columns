#pragma once

enum {
  cl_NONE,           // 0
  cl_INT8,           // 1
  cl_INT16,          // 2
  cl_INT32,          // 3
  cl_INT64,          // 4
  cl_INT128,         // 5
  cl_INT256,         // 6
  cl_UINT8,          // 7
  cl_UINT16,         // 8
  cl_UINT32,         // 9
  cl_UINT64,         // 10
  cl_UINT128,        // 11
  cl_UINT256,        // 12
  cl_FLOAT8,         // 13
  cl_FLOAT16,        // 14
  cl_FLOAT32,        // 15
  cl_FLOAT64,        // 16
  cl_FLOAT128,       // 17
  cl_FLOAT256,       // 18
  cl_BOOL,           // 19
  cl_OBJECT,         // 20
  cl_UNION,          // 21
  cl_FIXED_ARRAY,    // 22
  cl_FLEXIBLE_ARRAY, // 23
  cl_STRING,         // 24
  cl_MAX,            // 25
};

#ifdef __cplusplus
#include <cstdalign>
#include <cstddef>
#include <cstdint>
#else
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clFixedArray clFixedArray;
typedef struct clFlexibleArray clFlexibleArray;
typedef struct clObject clObject;
typedef struct clUnion clUnion;
typedef struct clColumn clColumn;
typedef struct clString clString;

struct clObject {
  int32_t num;
  const clColumn *columns;
};

struct clUnion {
  int32_t num;
  const clColumn *columns;
};

struct clFixedArray {
  int8_t tp;
  int32_t capacity;
  const clColumn *columns;
};

struct clFlexibleArray {
  int32_t tp;
  int32_t capacity;
  const clColumn *columns;
};

struct clString {
  int8_t tp;
  int32_t capacity;
};

struct clColumn {
  int8_t tp;

  struct {
    int8_t len;
    const char *string;
  } name;

  int32_t size;
  int32_t align;
  ptrdiff_t offset;

  union {
    clObject via_object;
    clUnion via_union;
    clFixedArray via_fixed_array;
    clFlexibleArray via_flexible_array;
    clString via_string;
  };
};

#ifdef __cplusplus
}
#endif

#ifdef USE_COLUMN_MACROS

#ifdef __cplusplus

namespace {
template <class T> struct clColumnTraits {
  static constexpr uint8_t kind =
      std::is_arithmetic<T>::value
          ? (std::is_integral<T>::value
                 ? (std::is_signed<T>::value
                        ? cl_INT8 + __builtin_ctz(sizeof(T))
                        : cl_UINT8 + __builtin_ctz(sizeof(T)))
                 : cl_FLOAT8 + __builtin_ctz(sizeof(T)))
          : (std::is_enum<T>::value ? cl_INT8 + __builtin_ctz(sizeof(T))
                                    : cl_NONE);
  static_assert(kind, "zero");
};
} // namespace

#define COLUMN_TYPE(PARENT, FIELD)                                             \
  (int(clColumnTraits<std::remove_cv<std::remove_reference<                    \
           decltype(((PARENT *)NULL)->FIELD)>::type>::type>::kind))

#else

#define COLUMN_STATIC_ASSERT(VAL)                                              \
  ((int)(sizeof(struct { int : ((!!(VAL)) - 1); })) + (VAL))

#define COLUMN_TYPE(PARENT, FIELD)                                             \
  COLUMN_STATIC_ASSERT(_Generic((((PARENT *)NULL)->FIELD),                     \
      char: cl_INT8,                                                           \
      int8_t: cl_INT8,                                                         \
      int16_t: cl_INT16,                                                       \
      int32_t: cl_INT32,                                                       \
      int64_t: cl_INT64,                                                       \
      uint8_t: cl_UINT8,                                                       \
      uint16_t: cl_UINT16,                                                     \
      uint32_t: cl_UINT32,                                                     \
      uint64_t: cl_UINT64,                                                     \
      float: cl_FLOAT8 + __builtin_ctz(sizeof(float)),                         \
      double: cl_FLOAT8 + __builtin_ctz(sizeof(double)),                       \
      long double: cl_FLOAT8 + __builtin_ctz(sizeof(long double)),             \
      bool: cl_BOOL,                                                           \
      default: cl_NONE))

#endif

#define DEFINE_FIELD_NUMBER(PARENT, FIELD)                                     \
  {                                                                            \
    .tp = COLUMN_TYPE(PARENT, FIELD),                                          \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
  }

#define DEFINE_FIELD_OBJECT(PARENT, FIELD, FIELDS)                             \
  {                                                                            \
    .tp = cl_OBJECT,                                                           \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_object =                                                          \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .columns = FIELDS,                                             \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_UNION(PARENT, FIELD, FIELDS)                              \
  {                                                                            \
    .tp = cl_UNION,                                                            \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_union =                                                           \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .columns = FIELDS,                                             \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_FIXED_ARRAY(PARENT, FIELD)                                \
  {                                                                            \
    .tp = cl_FIXED_ARRAY,                                                      \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_fixed_array =                                                     \
            {                                                                  \
                .tp = COLUMN_TYPE(PARENT, FIELD[0]),                           \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .columns = NULL,                                               \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_OBJECT_FIXED_ARRAY(PARENT, FIELD, ELEMENT)                \
  {                                                                            \
    .tp = cl_FIXED_ARRAY,                                                      \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_fixed_array =                                                     \
            {                                                                  \
                .tp = cl_OBJECT,                                               \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .columns = ELEMENT,                                            \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_FLEXIBLE_ARRAY(PARENT, FIELD)                             \
  {                                                                            \
    .tp = cl_FLEXIBLE_ARRAY,                                                   \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_flexible_array =                                                  \
            {                                                                  \
                .tp = COLUMN_TYPE(PARENT, FIELD[0]),                           \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .columns = NULL,                                               \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_OBJECT_FLEXIBLE_ARRAY(PARENT, FIELD, ELEMENT)             \
  {                                                                            \
    .tp = cl_FLEXIBLE_ARRAY,                                                   \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_flexible_array =                                                  \
            {                                                                  \
                .tp = cl_OBJECT,                                               \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .columns = ELEMENT,                                            \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_FIELD_STRING(PARENT, FIELD)                                     \
  {                                                                            \
    .tp = cl_STRING,                                                           \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#FIELD) - 1,                                         \
            .string = #FIELD,                                                  \
        },                                                                     \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .align = alignof(__typeof(((PARENT *)NULL)->FIELD)),                       \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_string =                                                          \
            {                                                                  \
                .tp = COLUMN_TYPE(PARENT, FIELD[0]),                           \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_OBJECT(TYPE, FIELDS)                                            \
  {                                                                            \
    .tp = cl_OBJECT,                                                           \
    .name =                                                                    \
        {                                                                      \
            .len = sizeof(#TYPE) - 1,                                          \
            .string = #TYPE,                                                   \
        },                                                                     \
    .size = sizeof(TYPE), .align = alignof(TYPE), .offset = 0,                 \
    {                                                                          \
        .via_object =                                                          \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .columns = FIELDS,                                             \
            },                                                                 \
    },                                                                         \
  }

#endif
