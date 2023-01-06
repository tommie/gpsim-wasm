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
#include "config.h"
#include <math.h>
#include "ds1820.h"


class TemperatureAttribute : public Float {

public:

  TemperatureAttribute()
    : Float("temperature",25.0,"Current temperature")
  {
  }


  void set(int r) {
    Float::set((double)r);
  };
};
class PoweredAttribute : public Boolean
{
public:
	PoweredAttribute()
	  : Boolean ("powered", true, "Externally Powered") {}
};

class Alarm_Th_Attribute : public Integer
{
public:
	Alarm_Th_Attribute()
	  : Integer ("alarm_th", 30, "Temp high or user data1 in EEPROM") {}

    virtual void get(char *buffer, int buf_size)
    {
        if(buffer)
        {
            int i;
            i = getVal();;
            snprintf(buffer,buf_size,"%d",i);
        }

    }
};

class Alarm_Tl_Attribute : public Integer
{
public:
	Alarm_Tl_Attribute()
	  : Integer ("alarm_tl", -5, "Temp low or user data2 in EEPROM") {}

    virtual void get(char *buffer, int buf_size)
    {
        if(buffer)
        {
            int i;
            i = getVal();;
            snprintf(buffer,buf_size,"%d",i);
        }

    }
};

class Config_Attribute : public Integer
{
public:
    Config_Attribute()
	  : Integer ("config_register", 0x7f, "EEPROM value of 18B20 Configuration Register") { }

    virtual void set(int64_t v)
    {
	Integer::set(v);
    }

 virtual void get(char *buffer, int buf_size)
 {
     if(buffer)
     {
         uint8_t i;
         i = getVal();;
         snprintf(buffer,buf_size,"0x%0x",i);
     }

 }

};


namespace DS1820_Modules
{
  void DS1820::doneBits() {
      if(verbose)
	cout << name() << " Setting dsState\n";
      (this->*dsState)();
  }

  void DS1820::resetEngine() {
      if(verbose)
      	cout << name() << " Ready for readCommand" << endl;
      dsState = &DS1820::readCommand;
      bitRemaining = 8;
      isReading = true;
  }

  bool DS1820::isAlarm()
  {
	char Th = scratchpad[2];
	char Tl = scratchpad[3];
        char temp = scratchpad[0]>>1;

	if(scratchpad[1])
	    temp |= 0x80;

      return temp < Tl || temp > Th;
  }

  void DS1820::readCommand() {
      double temp;
      int data;
      unsigned char mode;
      int count;
      if(verbose)
          cout << name() << " Got readCommand! " << hex << (unsigned int)octetBuffer[0] << endl;
      // load scratch from EEPROM here first time through to capture changes
      // from .sim commands in the program
      if (!ds1820_eeprom_loaded)
      {
	ds1820_eeprom_loaded = true;
        loadEEPROM();
      }
      switch(octetBuffer[0]) {
      case 0x44:	// Convert Temperature
	  mode = is_ds18b20 ? (scratchpad[4]>>5) & 3 : 0;
	  temp = attr_Temp->getVal();
	  // Convert the temperature to an integer with one bit more than the
	  // final result to allow for rounding
	  data = (int)round(temp * 32);
	  count = 16 - ((data >> 1) & 0xf);
	  // Round the least significant bit for the selected mode
	  data += 8 >> mode;
	  // Adjust the result depending on the sensor family
	  data >>= is_ds18b20 ? 1 : 4;
	  if (temp > 125 || temp < -55)
	    cout << name() << " Warning temperature " << temp << " outside operating range -55 to 125\n";
          scratchpad[0] = data & 0xFF;
          scratchpad[1] = data >> 8;
          scratchpad[6] = count;
          scratchpad[8] = calculateCRC8(scratchpad, 8);
          if (attr_powered->getVal()) // Can return status if powered
	  {
	     double time_conversion = 0.750; // 750 ms
	     switch(mode)
	     {
	     case 0:	// 9 bit resolution
		// Max time for conversion 93.75 ms
		time_conversion /= 8.;
		break;

	     case 1:	// 10 bit resolution
	      // Max time for conversion 187.5 ms
		time_conversion /= 4.;
		break;

	     case 2:	// 11 bit resolution
	      // Max time for conversion 375 ms
		time_conversion /= 2.;
		break;

	     case 3:	// 12 bit resolution
	      // Max time for conversion 750 ms
		break;
	     }

	     set_status_poll(get_cycles().get(time_conversion));
              return;
          }
          break;

      case 0x48:	//Copy scratchpad to EEPROM
	  attr_thigh->set(scratchpad[2]);
	  attr_tlow->set(scratchpad[3]);
	  if(is_ds18b20)
	      attr_config->set(scratchpad[4]);
          if (attr_powered->getVal()) { // Can return status if powered
	      // Max time to complete 10ms
	      set_status_poll(get_cycles().get(0.010000));
              return;
          }
          break;

      case 0x4E:	//Write scratchpad (master to Th, Tl
          isReading = true;
          bitRemaining = is_ds18b20?24:16;
          dsState = &DS1820::writeScratchpad;
          return;

      case 0xB4:	//Read Power supply
          isReading = false;
          if (attr_powered->getVal()) {
		if(verbose)
		  printf("%s is powered\n", name().c_str());
              bitRemaining = 0;
              return;
          }
	  if(verbose)
	      printf("%s on parasite power\n", name().c_str());
          octetBuffer[0] = 0;
          bitRemaining = 8;
          dsState = &DS1820::readPower;
          return;

      case 0xB8:	// Recall EE (Th, Tl from EEPROM to scratchpad)
          loadEEPROM();
          break;

      case 0xBE:	// Read scratchpad
	  if(verbose)
	     printf("%s scratchpad contents\n", name().c_str());
          for (int i = 0; i < 9; ++i)
	  {
              	octetBuffer[i] = scratchpad[8 - i];
		if(verbose)
	      	   printf("%d %0x\n", i, scratchpad[i]);
	  }
          isReading = false;
          bitRemaining = 72;
          dsState = &DS1820::done;
          return;

      default:
	  cout << name() << " " << __FUNCTION__ << " Unexpected command "  << hex << (unsigned int)octetBuffer[0] << endl;

      }
      isReading = false;
      octetBuffer[0] = 0x32;
      dsState = &DS1820::done;
      bitRemaining = 8;
  }

  void DS1820::writeScratchpad() {
      if(verbose)
          cout << "GOT writeScratchpad!" << hex << (unsigned int)octetBuffer[0] << ',' << (unsigned int) octetBuffer[1] << endl;
      if(is_ds18b20)
      {
      	scratchpad[2] = octetBuffer[2];
      	scratchpad[3] = octetBuffer[1];
       	scratchpad[4] = (octetBuffer[0] & 0x60) | 0x1f;
      }
      else
      {
          scratchpad[2] = octetBuffer[1];
          scratchpad[3] = octetBuffer[0];
      }
      scratchpad[8] = calculateCRC8(scratchpad, 8);
      return;
  }

  void DS1820::readPower() {
      if(verbose) cout << "Got readPower!" << endl;
      bitRemaining = 8;
      return;
  }

  void DS1820::done() {
      return;
  }

  void DS1820::loadEEPROM() {
      scratchpad[2] = (char)attr_thigh->getVal();
      scratchpad[3] = (char)attr_tlow->getVal();
      if(is_ds18b20)
	scratchpad[4] = (char)((attr_config->getVal() & 0x60) | 0x1f);
      scratchpad[8] = calculateCRC8(scratchpad, 8);
  }

  DS1820::DS1820(const char *name, bool ds18B20):
      Rom1W(name, "DS1820 - 1Wire thermomether.", ds18B20),
      ds1820_eeprom_loaded(false),
      dsState(&DS1820::done)
  {
      is_ds18b20 = ds18B20;
      scratchpad[0] = 0xAA;
      scratchpad[1] = 0;
      scratchpad[4] = 0xFF;
      scratchpad[5] = 0xFF;
      scratchpad[6] = 0x0C;
      scratchpad[7] = 0x10;
      attr_Temp = new TemperatureAttribute();
      attr_thigh = new Alarm_Th_Attribute();
      attr_tlow = new Alarm_Tl_Attribute();
      attr_powered = new PoweredAttribute();
      addSymbol(attr_Temp);
      addSymbol(attr_thigh);
      addSymbol(attr_tlow);
      addSymbol(attr_powered);
      if(is_ds18b20)
      {
	attr_config = new Config_Attribute();
	addSymbol(attr_config);
        cout << "===created a ds18b20 with name " << (name ?: "unnamed!") << endl;
      }
      else
        cout << "===created a ds1820 with name " << (name ?: "unnamed!") << endl;
  }

  DS1820::~DS1820() {
      removeSymbol(attr_Temp);
      removeSymbol(attr_thigh);
      removeSymbol(attr_tlow);
      removeSymbol(attr_powered);
	delete attr_Temp;
	delete attr_thigh;
	delete attr_tlow;
	delete attr_powered;
     if(is_ds18b20)
     {
	removeSymbol(attr_config);
	delete attr_config;
     }
  }

  Module* DS1820::construct(const char *name) {
      return new DS1820(name, false);
  }
  Module* DS1820::constructB(const char *name) {
      return new DS1820(name, true);
  }
} //end of namespace
