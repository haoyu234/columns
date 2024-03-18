#include <stdint.h> // IWYU pragma: keep

// cmd = 1
struct stUseItemReq {
  uint32_t itemID;
};

// cmd = 2
struct stDrop {
  uint32_t itemID;
  uint32_t itemNum;
};

/*cmd = 3 */
struct stUseItemRsp {
  uint32_t code;
  uint32_t num;

  struct stDrop drops[10];
};

union stValue {
  int32_t i32;
  uint32_t u32;

  char c;
  uint8_t u8;

  uint64_t other[2];
};

struct stInlineUnion {
  int32_t tag;
  union {
    int32_t i32;
    uint32_t u32;

    char c;
    uint8_t u8;

    uint64_t other[2];
  } abc;
};

struct stFuzz {
  char name[32];
  int tag;
  union stValue v;
};

struct stTests {
  uint32_t epoch;

  char name[32];

  uint32_t fuzzNum;
  struct stFuzz fuzz[20];
  struct stInlineUnion inlineUnion;
};
