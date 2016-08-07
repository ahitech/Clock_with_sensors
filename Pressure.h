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

#ifndef __PRESSURE_H__
#define __PRESSURE_H__

class Pressure {
  private:
    unsigned short pressure;    // Stored internally as mbars, since they give better resolution than mm Hg

  public:
    Pressure (unsigned short in);
    Pressure ();

    unsigned short ToMBar ();
    unsigned short ToMmHg ();
    void SetPressure (unsigned short mbars);
    friend bool operator> (const Pressure &left, const Pressure &right);
    inline friend bool operator< (const Pressure &left, const Pressure &right) {
      return !(left > right);
    }
}

#endif    // __PRESSURE_H__
