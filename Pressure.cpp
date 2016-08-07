/*
Alexey "Hitech" Burshtein
http://ahitech.livejournal.com

Clock and weather station.

Released under CC-BY-NC-SA 4.0 Worldwide license.
You're welcome to share this product, to adapt it and to create derived products for non-commercial use,
but you have to reference me and to use the same license.
*/

/* General description:
 *  
 *  I'm using:
 *  ----------
 *  * MattairTech MT-DB-U6 development board (just because I had it) - https://www.mattairtech.com/index.php/development-boards/mt-db-u6.html
 *  * Bosch BME280 sensor for sensing pressure, temperature and humidity - https://www.bosch-sensortec.com/bst/products/all_products/bme280
 *  * DS1307 Real-Time Battery backed up clock - https://www.maximintegrated.com/en/products/digital/real-time-clocks/DS1307.html
 *  * DS18B20 temperature sensor on DS1307 panel - https://www.maximintegrated.com/en/products/analog/sensors-and-sensor-interface/DS18B20.html
 *  * General 16x2 screen for output, based on PCF8574 chipset, with I2C module.
 *  * Three buttons (Set, Plus, Minus)
 */

#include "Pressure.h"

Pressure::Pressure () {
  this->SetPressure(1006);    // The default is air pressure at sea level
}

Pressure::Pressure (unsigned short mbar) {
  this->SetPressure (mbar);
}

void Pressure::SetPressure (unsigned short mbar) {
  this->pressure = mbar;
}

unsigned short Pressure::ToMBar (void) {
  return this->pressure;    // Internally the pressure is already stored in mbars
}

unsigned short Pressure::ToMmHg (void) {
  return (round (this->pressure * 0.75006375541921));
}

bool operator> (const Pressure &left, const Pressure &right) {
  if (left.ToMBar() > right.ToMBar()) { return true; }
  return false;
}

