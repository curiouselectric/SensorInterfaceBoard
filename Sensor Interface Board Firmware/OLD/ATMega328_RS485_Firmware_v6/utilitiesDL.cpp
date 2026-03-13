#include "utilitiesDL.h"

// This is a debug routine:
void DEBUG(bool DBG_ENABLE, String info)
{
  if (DBG_ENABLE == HIGH)
  {
    Serial.print(info);
  }
}

void DEBUGLN(bool DBG_ENABLE, String info)
{
  if (DBG_ENABLE == HIGH)
  {
    Serial.println(info);
  }
}

void check_unit_id(byte _unit_id) {
  // Reads the pins attached and updates the ID appropriately
  bitWrite(_unit_id, 0, !digitalRead(GPIO_ID0));
  bitWrite(_unit_id, 1, !digitalRead(GPIO_ID1));
  bitWrite(_unit_id, 2, !digitalRead(GPIO_ID2));
}