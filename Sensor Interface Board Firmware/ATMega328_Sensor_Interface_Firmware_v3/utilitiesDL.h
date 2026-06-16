#pragma once
#include "config.h"

byte check_unit_id(byte my_unit_id);
void startI2Csensor();
String getI2Cdata();