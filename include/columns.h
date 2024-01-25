#pragma once

#define cl_COLUMN_NONE 0
#define cl_COLUMN_INT8 1
#define cl_COLUMN_INT16 2
#define cl_COLUMN_INT32 3
#define cl_COLUMN_INT64 4
#define cl_COLUMN_INT128 5
#define cl_COLUMN_INT256 6
#define cl_COLUMN_UINT8 7
#define cl_COLUMN_UINT16 8
#define cl_COLUMN_UINT32 9
#define cl_COLUMN_UINT64 10
#define cl_COLUMN_UINT128 11
#define cl_COLUMN_UINT256 12
#define cl_COLUMN_FLOAT8 13
#define cl_COLUMN_FLOAT16 14
#define cl_COLUMN_FLOAT32 15
#define cl_COLUMN_FLOAT64 16
#define cl_COLUMN_FLOAT128 17
#define cl_COLUMN_FLOAT256 18
#define cl_COLUMN_BOOL 50
#define cl_COLUMN_OBJECT 51
#define cl_COLUMN_UNION 52
#define cl_COLUMN_FIXED_ARRAY 53
#define cl_COLUMN_FLEXIBLE_ARRAY 54

#ifdef __cplusplus
#include <cinttypes>
#include <cstdbool>
#include <cstddef>
#include <type_traits>
#else
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clFixedArray clFixedArray;
typedef struct clFlexibleArray clFlexibleArray;
typedef struct clObject clObject;
typedef struct clUnion clUnion;
typedef struct clColumn clColumn;

struct clObject {
  uint32_t num;
  const clColumn *fields;
};

struct clUnion {
  uint32_t num;
  const clColumn *fields;
};

struct clFixedArray {
  uint8_t flags;
  uint32_t capacity;
  const clColumn *element;
};

struct clFlexibleArray {
  uint8_t flags;
  uint32_t capacity;
  const clColumn *element;
};

struct clColumn {
  const char *name;
  uint8_t kind;
  uint32_t size;
  ptrdiff_t offset;
  union {
    clObject via_object;
    clUnion via_union;
    clFixedArray via_fixed_array;
    clFlexibleArray via_flexible_array;
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
                        ? cl_COLUMN_INT8 + __builtin_ctz(sizeof(T))
                        : cl_COLUMN_UINT8 + __builtin_ctz(sizeof(T)))
                 : cl_COLUMN_FLOAT8 + __builtin_ctz(sizeof(T)))
          : (std::is_enum<T>::value ? cl_COLUMN_INT8 + __builtin_ctz(sizeof(T))
                                    : cl_COLUMN_NONE);
  static_assert(kind, "zero");
};
} // namespace

#define COLUMN_TYPE_IMPL(PARENT, FIELD)                                        \
  (int(clColumnTraits<std::remove_cv<std::remove_reference<                    \
           decltype(((PARENT *)NULL)->FIELD)>::type>::type>::kind))

#else

#define COLUMN_STATIC_ASSERT(VAL)                                              \
  ((int)(sizeof(struct { int : ((!!(VAL)) - 1); })) + (VAL))

#define COLUMN_TYPE_IMPL(PARENT, FIELD)                                        \
  COLUMN_STATIC_ASSERT(_Generic((((PARENT *)NULL)->FIELD),                     \
      char: cl_COLUMN_INT8,                                                    \
      int8_t: cl_COLUMN_INT8,                                                  \
      int16_t: cl_COLUMN_INT16,                                                \
      int32_t: cl_COLUMN_INT32,                                                \
      int64_t: cl_COLUMN_INT64,                                                \
      uint8_t: cl_COLUMN_UINT8,                                                \
      uint16_t: cl_COLUMN_UINT16,                                              \
      uint32_t: cl_COLUMN_UINT32,                                              \
      uint64_t: cl_COLUMN_UINT64,                                              \
      float: cl_COLUMN_FLOAT8 + __builtin_ctz(sizeof(float)),                  \
      double: cl_COLUMN_FLOAT8 + __builtin_ctz(sizeof(double)),                \
      long double: cl_COLUMN_FLOAT8 + __builtin_ctz(sizeof(long double)),      \
      bool: cl_COLUMN_BOOL,                                                    \
      default: cl_COLUMN_NONE))

#endif

#define COLUMN_TYPE(PARENT, FIELD) (COLUMN_TYPE_IMPL(PARENT, FIELD))

#define DEFINE_COLUMN_NUMBER(PARENT, FIELD)                                    \
  {                                                                            \
    .name = #FIELD, .kind = COLUMN_TYPE(PARENT, FIELD),                        \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
  }

#define DEFINE_COLUMN_OBJECT(PARENT, FIELD, FIELDS)                            \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_OBJECT,                                  \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_object =                                                          \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .fields = FIELDS,                                              \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_COLUMN_UNION(PARENT, FIELD, FIELDS)                             \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_UNION,                                   \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_union =                                                           \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .fields = FIELDS,                                              \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_COLUMN_FIXED_ARRAY(PARENT, FIELD)                               \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_FIXED_ARRAY,                             \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_fixed_array =                                                     \
            {                                                                  \
                .flags = COLUMN_TYPE(PARENT, FIELD[0]),                        \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .element = NULL,                                               \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_COLUMN_OBJECT_FIXED_ARRAY(PARENT, FIELD, ELEMENT)               \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_FIXED_ARRAY,                             \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_fixed_array =                                                     \
            {                                                                  \
                .flags = cl_COLUMN_OBJECT,                                     \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .element = ELEMENT,                                            \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_COLUMN_FLEXIBLE_ARRAY(PARENT, FIELD)                            \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_FLEXIBLE_ARRAY,                          \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_flexible_array =                                                  \
            {                                                                  \
                .flags = COLUMN_TYPE(PARENT, FIELD[0]),                        \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .element = NULL,                                               \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_COLUMN_OBJECT_FLEXIBLE_ARRAY(PARENT, FIELD, ELEMENT)            \
  {                                                                            \
    .name = #FIELD, .kind = cl_COLUMN_FLEXIBLE_ARRAY,                          \
    .size = sizeof(((PARENT *)NULL)->FIELD),                                   \
    .offset = offsetof(PARENT, FIELD),                                         \
    {                                                                          \
        .via_flexible_array =                                                  \
            {                                                                  \
                .flags = cl_COLUMN_OBJECT,                                     \
                .capacity = sizeof(((PARENT *)NULL)->FIELD) /                  \
                            sizeof(((PARENT *)NULL)->FIELD[0]),                \
                .element = ELEMENT,                                            \
            },                                                                 \
    },                                                                         \
  }

#define DEFINE_OBJECT(TYPE, FIELDS)                                            \
  {                                                                            \
    .name = #TYPE, .kind = cl_COLUMN_OBJECT, .size = sizeof(TYPE),             \
    .offset = 0,                                                               \
    {                                                                          \
        .via_object =                                                          \
            {                                                                  \
                .num = sizeof(FIELDS) / sizeof(FIELDS[0]),                     \
                .fields = FIELDS,                                              \
            },                                                                 \
    },                                                                         \
  }

#endif
