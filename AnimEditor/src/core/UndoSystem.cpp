#include "core/UndoSystem.h"
#include <algorithm>

namespace anim {

void UndoSystem::execute(const std::string& name, Action doFn, Action undoFn) {
    history_.resize(index_);
    history_.push_back({name, std::move(undoFn), doFn});
    index_ = history_.size();

    if (history_.size() > maxHistory_) {
        history_.erase(history_.begin());
        --index_;
    }

    doFn();
}

bool UndoSystem::undo() {
    if (!canUndo()) return false;
    --index_;
    history_[index_].undo();
    return true;
}

bool UndoSystem::redo() {
    if (!canRedo()) return false;
    history_[index_].redo();
    ++index_;
    return true;
}

bool UndoSystem::canUndo() const {
    return index_ > 0;
}

bool UndoSystem::canRedo() const {
    return index_ < history_.size();
}

void UndoSystem::clear() {
    history_.clear();
    index_ = 0;
}

const std::string& UndoSystem::undoName() const {
    static const std::string empty;
    return canUndo() ? history_[index_ - 1].name : empty;
}

const std::string& UndoSystem::redoName() const {
    static const std::string empty;
    return canRedo() ? history_[index_].name : empty;
}

} // namespace anim
