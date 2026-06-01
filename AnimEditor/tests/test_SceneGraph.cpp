#include <gtest/gtest.h>
#include "core/SceneGraph.h"

using namespace anim;

TEST(SceneGraph, AddRootNode) {
    SceneGraph sg;
    auto node = sg.addNode("n1", NodeType::Sprite, "Hero", std::nullopt);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(sg.getRootNodes().size(), 1u);
    EXPECT_EQ(sg.getRootNodes()[0]->id, "n1");
    EXPECT_EQ(sg.getRootNodes()[0]->name, "Hero");
    EXPECT_EQ(sg.getRootNodes()[0]->type, NodeType::Sprite);
}

TEST(SceneGraph, AddChildNode) {
    SceneGraph sg;
    sg.addNode("parent", NodeType::Node, "Parent", std::nullopt);
    auto child = sg.addNode("child", NodeType::Sprite, "Child", "parent");
    ASSERT_TRUE(child.has_value());
    auto parent = sg.findById("parent");
    ASSERT_TRUE(parent.has_value());
    EXPECT_EQ((*parent)->children.size(), 1u);
    EXPECT_EQ((*parent)->children[0]->id, "child");
}

TEST(SceneGraph, RemoveNode) {
    SceneGraph sg;
    sg.addNode("parent", NodeType::Node, "Parent", std::nullopt);
    sg.addNode("child", NodeType::Sprite, "Child", "parent");
    EXPECT_TRUE(sg.removeNode("child"));
    auto parent = sg.findById("parent");
    ASSERT_TRUE(parent.has_value());
    EXPECT_EQ((*parent)->children.size(), 0u);
    EXPECT_FALSE(sg.findById("child").has_value());
}

TEST(SceneGraph, RemoveNonexistentNode) {
    SceneGraph sg;
    EXPECT_FALSE(sg.removeNode("no_such_node"));
}

TEST(SceneGraph, FindNodeById) {
    SceneGraph sg;
    sg.addNode("root", NodeType::Node, "Root", std::nullopt);
    sg.addNode("child", NodeType::Sprite, "Child", "root");
    sg.addNode("grandchild", NodeType::Label, "Grandchild", "child");
    auto found = sg.findById("grandchild");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ((*found)->name, "Grandchild");
    EXPECT_EQ((*found)->type, NodeType::Label);
}

TEST(SceneGraph, RenameNode) {
    SceneGraph sg;
    sg.addNode("n1", NodeType::Node, "OldName", std::nullopt);
    EXPECT_TRUE(sg.renameNode("n1", "NewName"));
    auto node = sg.findById("n1");
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ((*node)->name, "NewName");
    EXPECT_FALSE(sg.renameNode("nonexistent", "Whatever"));
}
