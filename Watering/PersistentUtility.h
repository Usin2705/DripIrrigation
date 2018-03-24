#pragma once
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <EEPROM.h>


class PersistentUtility
{
public:
	PersistentUtility();
	~PersistentUtility();
private:
	//EEPROM eeprom;

};

