#include <columns.h>
#include <cstddef>
#include <gtest/gtest.h>

#include "messages.h"
#include "messages_def.h"

TEST(base, sizeof) {
  EXPECT_EQ(sizeof(stUseItemRsp), stUseItemRspObject[0].size);
  EXPECT_EQ(alignof(stUseItemRsp), alignof(typeof(stUseItemRspObject[0].size)));
  EXPECT_EQ(offsetof(stFuzz, v), stFuzzObject[0].via_object.columns[2].offset);
  EXPECT_EQ(alignof(stValue), stFuzzObject[0].via_object.columns[2].align);
}
