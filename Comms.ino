#include "Comms.h"

void requestData(uint16_t timeout)
{
  SerialBT.setTimeout(timeout);

  // flush input buffer

  SerialBT.write('n');

  // wait for data or timeout
  uint32_t start = millis();
  uint32_t end = start;
  while (SerialBT.available() < 3 && (end - start) < timeout)
  {
    end = millis();
  }

  // if within timeout, read data
  if (end - start < timeout)
  {
    // skip first two bytes
    SerialBT.read(); // 'n'
    SerialBT.read(); // 0x32
    uint8_t dataLen = SerialBT.read();
    SerialBT.readBytes(buffer, dataLen);
  }
}

bool getBit(uint16_t address, uint8_t bit)
{
  return bitRead(buffer[address], bit);
}

uint8_t getByte(uint16_t address)
{
  return buffer[address];
}

uint16_t getWord(uint16_t address)
{
  return makeWord(buffer[address + 1], buffer[address]);
}