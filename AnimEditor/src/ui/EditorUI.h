#pragma once
#include "ui/FileBrowserPanel.h"
#include "ui/NodeTreePanel.h"
#include "ui/PreviewCanvas.h"
#include "ui/PropertyPanel.h"
#include "ui/TimelinePanel.h"
#include "debug/DebugHost.h"
#include "core/AnimData.h"
#include "core/SceneGraph.h"
#include "core/UndoSystem.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace anim {

class EditorUI {
public:
    bool init();
    void shutdown();
    void render();

    void openProject(const std::string& folderPath);
    void openAnimFile(const std::string& filePath);

    bool isDirty() const { return dirty_; }
    bool wantsToQuit() const { return wantsToQuit_; }
    void showConfirmDiscard();

    template<typename T, typename... Args>
    void registerDebugPanel(Args&&... args) {
        debugHost_.registerPanel(std::make_unique<T>(std::forward<Args>(args)...));
    }

private:
    FileBrowserPanel fileBrowser_;
    NodeTreePanel nodeTreePanel_;
    PreviewCanvas previewCanvas_;
    PropertyPanel propertyPanel_;
    TimelinePanel timelinePanel_;
    DebugHost debugHost_;
    SceneGraph sceneGraph_;
    UndoSystem undoSystem_;
    std::shared_ptr<anim::AnimProject> currentProject_;
    std::string currentFilePath_;
    std::string workspacePath_;

    // Dirty flag
    bool dirty_ = false;
    bool wantsToQuit_ = false;

    // Recent files
    std::vector<std::string> recentFiles_;
    static constexpr size_t kMaxRecentFiles = 10;
    void loadRecentFiles();
    void saveRecentFiles();
    void addRecentFile(const std::string& path);
    std::string recentFilesPath();

    // Workspace persistence
    void loadWorkspacePath();
    void saveWorkspacePath();

    // Popup state
    bool showNewClipPopup_ = false;
    bool showConfirmDiscard_ = false;
    char popupTextBuf_[256] = {};

    // Pending action after confirm discard
    enum class PendingAction { None, NewAnimation, OpenAnim };
    PendingAction pendingAction_ = PendingAction::None;
    std::string pendingOpenPath_;

    void markDirty() { dirty_ = true; }
    void markClean() { dirty_ = false; }

    void renderMenuBar();
    void renderPopups();
    void syncProjectToUI();
    void saveAs(const std::string& path);
    void doSave();
    void doNewAnimation();
    void doOpenAnimFile(const std::string& path);
};

} // namespace anim
