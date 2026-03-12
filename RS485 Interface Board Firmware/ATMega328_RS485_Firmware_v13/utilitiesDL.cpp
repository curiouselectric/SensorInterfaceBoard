#include "utilitiesDL.h"

byte check_unit_id(byte _unit_id) {
  // Reads the pins attached and updates the ID appropriately
  bitWrite(_unit_id, 0, !digitalRead(GPIO_ID0));
  bitWrite(_unit_id, 1, !digitalRead(GPIO_ID1));
  bitWrite(_unit_id, 2, !digitalRead(GPIO_ID2));
  return (_unit_id);
}