#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class Relay
{
public:
	Relay(int pinNumber);
	~Relay();
	bool getState();
	void setState(bool state);
private:
	int pinNumber;
	bool state;
};

