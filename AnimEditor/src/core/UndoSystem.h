#pragma once
#include <string>
#include <vector>
#include <functional>

namespace anim {

class UndoSystem {
public:
    using Action = std::function<void()>;
    explicit UndoSystem(size_t maxHistory = 100) : maxHistory_(maxHistory) {}

    void execute(const std::string& name, Action doFn, Action undoFn);
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;
    void clear();
    const std::string& undoName() const;
    const std::string& redoName() const;

private:
    struct Entry {
        std::string name;
        Action undo;
        Action redo;
    };
    std::vector<Entry> history_;
    size_t index_ = 0;
    size_t maxHistory_;
};

} // namespace anim
