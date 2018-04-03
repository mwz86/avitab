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
#ifndef SRC_AVITAB_APPS_APPFUNCTIONS_H_
#define SRC_AVITAB_APPS_APPFUNCTIONS_H_

#include <memory>
#include <string>
#include "src/gui_toolkit/rasterizers/RasterJob.h"
#include "src/gui_toolkit/Icon.h"
#include "src/environment/EnvData.h"

namespace avitab {

class AppFunctions {
public:
    virtual std::unique_ptr<RasterJob> createRasterJob(const std::string &path) = 0;
    virtual Icon loadIcon(const std::string &path) = 0;
    virtual void executeLater(std::function<void()> func) = 0;
    virtual std::string getDataPath() = 0;
    virtual EnvData getDataRef(const std::string &dataRef) = 0;
    virtual ~AppFunctions() = default;
};

}

#endif /* SRC_AVITAB_APPS_APPFUNCTIONS_H_ */
