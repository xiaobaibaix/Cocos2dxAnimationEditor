#include <gtest/gtest.h>
#include "core/UndoSystem.h"

using namespace anim;

TEST(UndoSystem, ExecuteAndUndo) {
    UndoSystem us;
    int value = 0;

    us.execute("set 5",
               [&]() { value = 5; },
               [&]() { value = 0; });

    EXPECT_EQ(value, 5);
    EXPECT_TRUE(us.canUndo());

    us.undo();
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(us.canRedo());

    us.redo();
    EXPECT_EQ(value, 5);
    EXPECT_FALSE(us.canRedo());
}

TEST(UndoSystem, UndoChainBreaksOnNewAction) {
    UndoSystem us;
    int value = 0;

    us.execute("set 1", [&]() { value = 1; }, [&]() { value = 0; });
    us.execute("set 2", [&]() { value = 2; }, [&]() { value = 1; });
    us.undo();
    EXPECT_EQ(value, 1);

    us.execute("set 3", [&]() { value = 3; }, [&]() { value = 1; });
    EXPECT_FALSE(us.canRedo());
    EXPECT_EQ(value, 3);

    us.undo();
    EXPECT_EQ(value, 1);
}

TEST(UndoSystem, UndoLimit) {
    UndoSystem us(3);
    int value = 0;

    for (int i = 1; i <= 5; ++i) {
        int prev = value;
        us.execute("step",
                   [&value, i]() { value = i; },
                   [&value, prev]() { value = prev; });
    }

    EXPECT_EQ(value, 5);
    EXPECT_TRUE(us.canUndo());
    us.undo();
    EXPECT_EQ(value, 4);
    us.undo();
    EXPECT_EQ(value, 3);
    us.undo();
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(us.canUndo());
}
