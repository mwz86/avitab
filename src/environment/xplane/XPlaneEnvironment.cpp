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
#include <XPLM/XPLMPlugin.h>
#include <XPLM/XPLMPlanes.h>
#include <XPLM/XPLMScenery.h>
#include <stdexcept>
#include "XPlaneEnvironment.h"
#include "XPlaneGUIDriver.h"
#include "src/Logger.h"
#include "src/platform/Platform.h"

namespace avitab {

XPlaneEnvironment::XPlaneEnvironment() {
    // Called by the X-Plane thread via StartPlugin
    pluginPath = platform::nativeToUTF8(getPluginPath());
    flightLoopId = createFlightLoop();

    std::string rootPath = platform::nativeToUTF8(getXPlanePath());
    xplaneData = std::make_shared<xdata::XData>(rootPath);

    XPLMScheduleFlightLoop(flightLoopId, -1, true);
}

std::string XPlaneEnvironment::getXPlanePath() {
    char buf[2048];
    XPLMGetSystemPath(buf);
    return buf;
}

std::string XPlaneEnvironment::getPluginPath() {
    XPLMPluginID ourId = XPLMGetMyID();
    char pathBuf[2048];
    XPLMGetPluginInfo(ourId, nullptr, pathBuf, nullptr, nullptr);
    char *pathPart = XPLMExtractFileAndPath(pathBuf);
    return std::string(pathBuf, 0, pathPart - pathBuf) + "/../";
}

XPLMFlightLoopID XPlaneEnvironment::createFlightLoop() {
    XPLMCreateFlightLoop_t loop;
    loop.structSize = sizeof(XPLMCreateFlightLoop_t);
    loop.phase = 0; // ignored according to docs
    loop.refcon = this;
    loop.callbackFunc = [] (float f1, float f2, int c, void *ref) -> float {
        if (!ref) {
            return 0;
        }
        auto *us = reinterpret_cast<XPlaneEnvironment *>(ref);
        return us->onFlightLoop(f1, f2, c);
    };

    XPLMFlightLoopID id = XPLMCreateFlightLoop(&loop);
    if (!id) {
        throw std::runtime_error("Couldn't create flight loop");
    }
    return id;
}

std::shared_ptr<LVGLToolkit> XPlaneEnvironment::createGUIToolkit() {
    std::shared_ptr<GUIDriver> driver = std::make_shared<XPlaneGUIDriver>();
    return std::make_shared<LVGLToolkit>(driver);
}

void XPlaneEnvironment::createMenu(const std::string& name) {
    XPLMMenuID pluginMenu = XPLMFindPluginsMenu();
    subMenuIdx = XPLMAppendMenuItem(pluginMenu, name.c_str(), nullptr, 0);

    if (subMenuIdx < 0) {
        throw std::runtime_error("Couldn't create our menu item");
    }

    subMenu = XPLMCreateMenu(name.c_str(), pluginMenu, subMenuIdx, [] (void *ctrl, void *cb) {
        MenuCallback callback = *reinterpret_cast<MenuCallback *>(cb);
        if (callback) {
            callback();
        }
    }, this);

    if (!subMenu) {
        XPLMRemoveMenuItem(pluginMenu, subMenuIdx);
        throw std::runtime_error("Couldn't create our menu");
    }
}

void XPlaneEnvironment::addMenuEntry(const std::string& label, MenuCallback cb) {
    menuCallbacks.push_back(cb);
    int idx = menuCallbacks.size() - 1;
    XPLMAppendMenuItem(subMenu, label.c_str(), &menuCallbacks[idx], 0);
}

void XPlaneEnvironment::destroyMenu() {
    if (subMenu) {
        XPLMDestroyMenu(subMenu);
        subMenu = nullptr;
        XPLMRemoveMenuItem(XPLMFindPluginsMenu(), subMenuIdx);
        subMenuIdx = -1;
    }
}

void XPlaneEnvironment::createCommand(const std::string& name, const std::string& desc, CommandCallback cb) {
    XPLMCommandRef cmd = XPLMCreateCommand(name.c_str(), desc.c_str());
    if (!cmd) {
        throw std::runtime_error("Couldn't create command: " + name);
    }

    RegisteredCommand cmdInfo;
    cmdInfo.callback = cb;
    cmdInfo.inBefore = true;
    cmdInfo.refCon = this;

    commandHandlers.insert(std::make_pair(cmd, cmdInfo));

    XPLMRegisterCommandHandler(cmd, handleCommand, true, this);
}

int XPlaneEnvironment::handleCommand(XPLMCommandRef cmd, XPLMCommandPhase phase, void* ref) {
    if (phase != xplm_CommandBegin) {
        return 1;
    }

    XPlaneEnvironment *us = reinterpret_cast<XPlaneEnvironment *>(ref);
    if (!us) {
        return 1;
    }

    CommandCallback f = us->commandHandlers[cmd].callback;
    if (f) {
        f();
    }

    return 1;
}

void XPlaneEnvironment::destroyCommands() {
    for (auto &iter: commandHandlers) {
        XPLMUnregisterCommandHandler(iter.first, handleCommand, true, this);
    }
    commandHandlers.clear();
}

std::string XPlaneEnvironment::getAirplanePath() {
    char file[512];
    char path[512];
    XPLMGetNthAircraftModel(0, file, path);
    std::string pluginPath = platform::nativeToUTF8(path);
    return platform::getDirNameFromPath(pluginPath) + "/";
}

std::string XPlaneEnvironment::getProgramPath() {
    return pluginPath;
}

void XPlaneEnvironment::runInEnvironment(EnvironmentCallback cb) {
    registerEnvironmentCallback(cb);
}

float XPlaneEnvironment::onFlightLoop(float elapsedSinceLastCall, float elapseSinceLastLoop, int count) {
    runEnvironmentCallbacks();
    return -1;
}

EnvData XPlaneEnvironment::getData(const std::string& dataRef) {
    std::promise<EnvData> dataPromise;
    auto futureData = dataPromise.get_future();

    runInEnvironment([&dataPromise, &dataRef, this] () {
        try {
            dataPromise.set_value(dataCache.getData(dataRef));
        } catch (...) {
            // transfer exceptions across the threads
            dataPromise.set_exception(std::current_exception());
        }
    });

    return futureData.get();
}

double XPlaneEnvironment::getMagneticVariation(double lat, double lon) {
    std::promise<double> dataPromise;
    auto futureData = dataPromise.get_future();

    runInEnvironment([&dataPromise, &lat, &lon, this] () {
        double variation = XPLMGetMagneticVariation(lat, lon);
        dataPromise.set_value(variation);
    });

    return futureData.get();
}

std::shared_ptr<xdata::XData> XPlaneEnvironment::getNavData() {
    return xplaneData;
}

void XPlaneEnvironment::reloadMetar() {
    xplaneData->reloadMetar();
}

XPlaneEnvironment::~XPlaneEnvironment() {
    if (flightLoopId) {
        XPLMDestroyFlightLoop(flightLoopId);
    }

    logger::verbose("~XPlaneEnvironment");
    destroyMenu();
}

} /* namespace avitab */
