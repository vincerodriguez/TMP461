#ifndef TEMPWIRE_H
#define TEMPWIRE_H

#include "Arduino.h"

class tempWire
{
public:
  tempWire();

  void Init(uint8_t);
  double readTemp();

private:
  uint8_t slave_addr;
  void Wait();
  void Start();
  void Stop();
  void Restart();
  void Ack();
  void Nack();
  void Write(unsigned char, uint8_t);
  uint8_t Read(uint8_t);
};

extern tempWire tWire;

#endif
