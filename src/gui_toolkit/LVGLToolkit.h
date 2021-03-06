/*
 *   AviTab - Aviator's Virtual Tablet
 *   Copyright (C) 2018 Folke Will <folko@solhost.org>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SRC_GUI_TOOLKIT_LVGLTOOLKIT_H_
#define SRC_GUI_TOOLKIT_LVGLTOOLKIT_H_

#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include "src/environment/GUIDriver.h"
#include "src/gui_toolkit/widgets/Screen.h"
#include "src/gui_toolkit/rasterizers/DocumentRasterizer.h"
#include "src/gui_toolkit/rasterizers/RasterJob.h"

namespace avitab {

class LVGLToolkit {
public:
    using GUITask = std::function<void()>;
    using MouseWheelCallback = std::function<void(int, int, int)>;

    LVGLToolkit(std::shared_ptr<GUIDriver> drv);

    void setMouseWheelCallback(MouseWheelCallback cb);
    void createNativeWindow(const std::string &title);
    void pauseNativeWindow();
    bool hasNativeWindow();
    void signalStop();
    void destroyNativeWindow();
    void setBrightness(float b);
    float getBrightness();

    std::unique_ptr<RasterJob> createRasterJob(const std::string &document);

    std::shared_ptr<Screen> &screen();

    // call can be any task except for the GUI task
    void runInGUI(GUITask func);

    // caller must be the GUI task
    void executeLater(GUITask func);

    ~LVGLToolkit();
private:
    static bool lvglIsInitialized;
    static LVGLToolkit *instance;
    MouseWheelCallback onMouseWheel;
    DocumentRasterizer docRasterizer;
    std::mutex guiMutex;
    std::vector<GUITask> pendingTasks;
    std::shared_ptr<GUIDriver> driver;
    std::unique_ptr<std::thread> guiThread;
    std::atomic_bool guiActive;
    std::shared_ptr<Screen> mainScreen;

    int getFrameWidth();
    int getFrameHeight();

    void initDisplayDriver();
    void initInputDriver();
    void guiLoop();
};

} /* namespace avitab */

#endif /* SRC_GUI_TOOLKIT_LVGLTOOLKIT_H_ */
