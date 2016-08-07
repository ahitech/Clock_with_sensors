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


#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include <FaBoLCD_PCF8574.h>

#define LCD_DEGREE_C_CHARACTER  (byte)0
#define LCD_DEGREE_F_CHARACTER  (byte)1

/* Specific class that holds temperature.
 * Used by the "Record" class.
 * NOTES: The temperature is always stored as degrees centigrade.
 * Max ranges for storage: Absolute zero to 32767°C
 * Max ranges for conversion to Fahrengheit: Absolute zero to 32767°F = 18186.1°C
 */
class Temperature {
  private:
    short temperature;
  public:
    Temperature (short temperature = 20);  // Default temperature is 20°C
    void SetTemperature(short temperature = 20);
    char ToFahrengheit ();
    char ToCelsius ();
    friend bool operator> (const Temperature &left, const Temperature &right);
    inline friend bool operator< (const Temperature &left, const Temperature &right) {
      return !(left > right);
    }
};

#endif    // __TEMPERATURE_H__
