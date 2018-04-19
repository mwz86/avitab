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
#include <iostream>
#include "AirportParser.h"
#include "src/libxdata/loaders/objects/AirportData.h"

namespace xdata {

AirportParser::AirportParser(const std::string& file):
    parser(file)
{
    header = parser.parseHeader();
}

void AirportParser::setAcceptor(Acceptor a) {
    acceptor = a;
}

std::string AirportParser::getHeader() const {
    return header;
}

void AirportParser::loadAirports() {
    using namespace std::placeholders;
    curPort = {};
    parser.eachLine(std::bind(&AirportParser::parseLine, this));
}

void AirportParser::parseLine() {
    int rowCode = parser.parseInt();

    switch (rowCode) {
    case 1:
    case 16:
    case 17:
        finishAirport();
        startAirport();
        break;
    case 100:
        parseRunway();
        break;
    case 1302:
        parseMetaData();
        break;
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
        parseFrequency(rowCode);
        break;
    case 99:
        finishAirport();
        break;
    }
}

void AirportParser::startAirport() {
    curPort.elevation = parser.parseInt();
    parser.parseInt(); // not required
    parser.parseInt(); // not required;
    curPort.id = parser.parseWord();
    curPort.name = parser.restOfLine();
}

void AirportParser::parseRunway() {
    AirportData::RunwayData rwy;

    rwy.width = parser.parseDouble();
    rwy.surfaceType = parser.parseInt();
    parser.parseInt(); // shoulder
    parser.parseDouble(); // smoothness
    parser.parseInt(); // center lights
    parser.parseInt(); // edge lights
    parser.parseInt(); // autogen signs

    AirportData::RunwayEnd end;
    while (parseRunwayEnd(end)) {
        rwy.ends.push_back(end);
    }
    curPort.runways.push_back(rwy);
}

bool AirportParser::parseRunwayEnd(AirportData::RunwayEnd &end) {
    std::string firstWord = parser.parseWord();
    if (firstWord.empty()) {
        return false;
    }

    end.name = firstWord;
    end.latitude = parser.parseDouble();
    end.longitude = parser.parseDouble();
    end.displace = parser.parseDouble();
    parser.parseDouble(); // overrun length
    parser.parseInt(); // markings
    parser.parseInt(); // approach lights
    parser.parseInt(); // touchdown lights
    parser.parseInt(); // REIL lights

    return true;
}

void AirportParser::parseMetaData() {
    std::string key = parser.parseWord();

    if (key == "country") {
        curPort.country = parser.restOfLine();
    } else if (key == "region_code") {
        curPort.region = parser.restOfLine();
    } else if (key == "datum_lat") {
        curPort.latitude = parser.parseDouble();
    } else if (key == "datum_lon") {
        curPort.longitude = parser.parseDouble();
    } else if (key == "icao_code") {
        curPort.icaoCode = parser.parseWord();
    }
}

void AirportParser::parseFrequency(int code) {
    AirportData::Frequency entry;
    entry.code = code;
    entry.frq = parser.parseInt();
    entry.desc = parser.restOfLine();
    curPort.frequencies.push_back(entry);
}

void AirportParser::finishAirport() {
    if (!curPort.id.empty()) {
        acceptor(curPort);
    }
    curPort = {};
}

}
/* namespace xdata */