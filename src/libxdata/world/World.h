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
#ifndef SRC_LIBXDATA_WORLD_WORLD_H_
#define SRC_LIBXDATA_WORLD_WORLD_H_

#include <src/libxdata/parsers/objects/CIFPData.h>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include "src/libxdata/world/models/airport/Airport.h"
#include "src/libxdata/world/models/navaids/Fix.h"
#include "src/libxdata/world/models/Region.h"
#include "src/libxdata/world/models/Airway.h"

namespace xdata {

constexpr const double KM_TO_NM = 0.539957;

class World {
public:
    World();

    std::shared_ptr<Airport> findAirportByID(const std::string &id);
    std::shared_ptr<Fix> findFixByRegionAndID(const std::string &region, const std::string &id);
    void forEachAirport(std::function<void(Airport &)> f);

    void addFix(std::shared_ptr<Fix> fix);
    std::shared_ptr<Region> createOrFindRegion(const std::string &id);
    std::shared_ptr<Airport> createOrFindAirport(const std::string &id);
    std::shared_ptr<Airway> createOrFindAirway(const std::string &name, Airway::Level lvl);

private:
    // Unique IDs
    std::map<std::string, std::shared_ptr<Region>> regions;
    std::map<std::string, std::shared_ptr<Airport>> airports;

    // Unique only within region
    std::multimap<std::string, std::shared_ptr<Fix>> fixes;

    // Unique within airway level
    std::multimap<std::string, std::shared_ptr<Airway>> airways;
};

} /* namespace xdata */

#endif /* SRC_LIBXDATA_WORLD_WORLD_H_ */
