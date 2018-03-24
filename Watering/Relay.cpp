#include "Relay.h"

Relay::Relay(int pinNumber){
	this->pinNumber = pinNumber;
	pinMode(pinNumber, OUTPUT);
	digitalWrite(pinNumber, this->state);
}

Relay::~Relay(){
}

bool Relay::getState(){
	return false;
}

void Relay::setState(bool state){
	this->state = state;
}
