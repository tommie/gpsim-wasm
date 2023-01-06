/*  Copyright (C) 2012 Eduard Timotei Budulea
    Copyright (C) 2013 Roy R. Rankin 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DS1820_H
#define DS1820_H
#ifndef IN_MODULE
#define IN_MODULE
#endif

#include "rom1w.h"

class TemperatureAttribute;
class PoweredAttribute;
class Alarm_Th_Attribute;
class Alarm_Tl_Attribute;
class Config_Attribute;

namespace DS1820_Modules
{
  class DS1820 : public Rom1W 
  {
    virtual void doneBits();
    virtual void resetEngine();
    virtual bool isAlarm();

    TemperatureAttribute *attr_Temp;
    PoweredAttribute *attr_powered;
    Alarm_Th_Attribute *attr_thigh;
    Alarm_Tl_Attribute *attr_tlow;
    Config_Attribute *attr_config;

    bool ds1820_eeprom_loaded;
    unsigned char scratchpad[9];
    bool is_ds18b20;

    void (DS1820::* dsState)();

    void readCommand();
    void writeScratchpad();
    void readPower();
    void done();

    void loadEEPROM();
public:
    DS1820(const char *name, bool isDS18B20);
    ~DS1820();
    static Module* construct(const char *name = 0);
    static Module* constructB(const char *name = 0);
  };
} // end of namespace

#endif
