
/****************************************************************************/
/***        Including Files                                               ***/
/****************************************************************************/
#include <Arduino.h>
#include <SoftwareSerial.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Class Definition                                              ***/
/****************************************************************************/
class moistureSensor
{
public:
	void begin();
  bool readHumiture(byte ID);   // Where ID is the ID of the unit (default 1, can be 1-255)
  void changeID(byte IDold, byte IDnew);  // Conver the ID from IDold to IDnew
  float moisturePercent;
  float soilTemperatureC;
private:
  bool  testing = true;   // If this flag is set then will return HEX data and testing info (otherwise not!)
  unsigned int calc_CRC16(unsigned char *buf, int len); 
};
extern moistureSensor soilMoistureSensor;