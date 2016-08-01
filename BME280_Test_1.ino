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

/* ==== Includes ==== */
#include <BME280.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RBD_Timer.h>
#include <RBD_Button.h>
#include <leOS2.h>
#include <RtcDateTime.h>
#include <RtcDS1307.h>
#include <RtcDS3231.h>
#include <RtcTemperature.h>
#include <RtcUtility.h>
/* ====  END Includes ==== */

/* ==== Configurables ==== */
#define DAYS_OF_LOGGING         2       // That means, the unit will save sensor's data from last 48 hours.

// Display configuration
#define DISPLAY_ROWS            2       // Number of lines on the display
#define DISPLAY_CHARS           16      // Number of characters in each line of the display

// Board configuration
#define SET_BUTTON_PIN            27
#define PLUS_BUTTON_PIN           26
#define MINUS_BUTTON_PIN          22
#define LCD_I2C_ADDRESS           0x27  // Address of the I2C chip for display output
#define LED_PIN                   0     // On MT-DB-U6, the onboard LED is connected to pin 0
#define SET_BUTTON_TIMEOUT        500   // Milliseconds
#define PLUS_MINUS_BUTTON_REPEAT  300   // Milliseconds
/* ==== End of configurables ==== */

/* ==== Defines ==== */
#define SERIAL_BAUD 115200

// Custom characters
#define LCD_DEGREE_C_CHARACTER  (byte)0
#define LCD_DEGREE_F_CHARACTER  (byte)1
#define LCD_MM_CHARACTER        (byte)2
#define LCD_MBAR_CHARACTER      (byte)3

/*===== State machine ====*/
#define STATE_NORMAL            0x01    // General state. Everything is displayed "as is", no blinking parts
#define STATE_CONFIGURATION     0x00    // Configuration state. It's 0 because I can use (!STATE_NORMAL) or (!STATE_CONFIGURATION).

#define CONFIG_SET_HOUR           0x01
#define CONFIG_SET_MINUTE         0x02
#define CONFIG_SET_AM_PM_H_HH     0x03    // Am and Pm are for 12-hour clock, HH is for 24-hour, with leading zero configured separately (default: 12-hour)
#define CONFIG_LEADING_ZERO       0x04    // Zero is printed before hour if hour is less then 10 (default: false, zero is not printed).
#define CONFIG_SET_DAY            0x05
#define CONFIG_SET_MONTH          0x06
#define CONFIG_SET_YEAR           0x07
#define CONFIG_SET_HOUR_OF_REC    0x08    // Starting at which hour the data should be saved (only round hours, default is 08:00 am).
#define CONFIG_SET_FREQ_OF_REC    0x09    // How many times in 24 hours should the data be saved (default is 2, maximum is 24; see "settings").
#define CONFIG_SET_DATE_ORDER     0x0A    // Cyclically switching between DD/MM/YY, MM/DD/YY, WE DD/MM, WE MM/DD (WE is day of week) and YY-MM-DD (with fixed separator)
#define CONFIG_SET_DATE_SEPARATOR 0x0B    // Cyclically switch between "/", "-" and "." 
#define CONFIG_SET_CELSIUS_FAHR   0x0C    // Switching between Celsius and Fahrengheit

// Definitions for time display options
#define CLOCK_NO_LEADING_ZERO   0x00    // No "zero" is printed before hour if hour is less then 10 (default)
#define CLOCK_LEADING_ZERO      0x01    // "Zero" is printed before hour if hour is less then 10
#define CLOCK_12H               0x00    // "am"/"pm" mode (default)
#define CLOCK_24H               0x01    // 24-hour mode

// Definitions for day-month-year display options. Leading zero is never printed.
#define DATE_DD_MM_YY           0x01    // Day-month-year (default)
#define DATE_MM_DD_YY           0x02    // Month-day-year
#define DATE_YY_MM_DD           0x03    // Year-month-day
#define DATE_WE_DD_MM           0x04    // Weekday day-month
#define DATE_WE_MM_DD           0x05    // Weekday month-day

// Definitions for date separator
#define SEPARATOR_SLASH         0x01    // Separator is "/" (default)
#define SERARATOR_DOT           0x02    // Separator is "."
#define SEPARATOR_DASH          0x03    // Separator is "-"

// Definitions for temperature unit of measurement
#define TEMPERATURE_CELSIUS     0x01    // Celsius (default)
#define TEMPERATURE_FAHRENGHEIT 0x02    // Fahrengheit

/* Any guesses what does the following struct hold?
 *  :)
 */
struct Settings {
  unsigned char frequency_of_recording; // How many times per day the data is recorded.
                                        // Possible values: 1 (once a day), 2 (default), 3, 4, 6, 8, 12 or 24 (hourly).
                                        // Note: this is NOT delay between recordings, it's frequency per 24 hours!
  unsigned char time_of_recording;      // What is the time from which the count of frequency is started.
                                        // Can be between 0 (midnight) and 23. Default is 8 (record is taken at 8:00).
  unsigned char date_order;             // Order of the parts of the date
  unsigned char date_separator;         // Separator between parts of the date
  unsigned char clock_leading_zero;     // Print leading zero for the hour display
  unsigned char clock_type;             // 12-hour or 24-hour
  unsigned char temperature_unit;       // Use Celsius for temperature readings
};
struct Settings   global_settings;      // Global settings variable


/* ==== Prototypes ==== */
int sign (short);
/* ==== END Prototypes ==== */

#if 0
/* Specific class that holds temperature.
 * Used by the "Record" class (see below).
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
    void Display (LiquidCrystal_I2C * lcd);
};

Temperature::Temperature(short temperature) {
  SetTemperature(temperature);
}

void Temperature::SetTemperature (short temperature) {
  this->temperature = temperature;
}

char Temperature::ToFahrengheit () {
  return (short)round(32 + this->temperature * 9/5); // Max 32767°F = 18186.1°C
}

char Temperature::ToCelsius () {
  return this->temperature;   // The internally stored temperature is already in Celsius.
}

void Temperature::Display (LiquidCrystal_I2C *lcd) {
  Serial.print("\tTemperature: ");
  Serial.print(this->temperature);

  if (!lcd) { return; }
/*  lcd->setCursor(6, 1);        // Hard-coded position for the temperature
  switch (sign(this->temperature)) {
    case 1:
      lcd->print("+");
      break;
    case -1:
      lcd->print("-");
      break;
    default:
      lcd->print(" ");
      break;
  } *

  lcd->print ("AAA");
*
  lcd->print(this->temperature);

  switch (global_settings.temperature_unit) {
    case TEMPERATURE_CELSIUS:
      lcd->write(LCD_DEGREE_C_CHARACTER); // Printing the degree sign
      break;
    case TEMPERATURE_FAHRENGHEIT:         // Intentional fall-through
    default:
      lcd->write(LCD_DEGREE_F_CHARACTER); // Printing the degree sign
      break;
  }
 */ 
}

/* The following class saves one single record of sensors' data and keeps it along with the time of recording.
 *  Actually, in STATE_NORMAL the display always displays one of "Record"'s objects.
 */
class Record {
  private:
    unsigned short pressure;            // Atmospheric pressure
    Temperature    temperature;         // Duh
    unsigned char  humidity;            // In percents
    RtcDateTime    time_of_recording;   // Date and time of the recording
    char           output_string[17];   // String that represents the sensors' data

    void PrintPressure(LiquidCrystal_I2C * lcd);
    void PrintHumidity(LiquidCrystal_I2C * lcd);
  public:
    Record();
    Record(RtcDateTime *dt, unsigned short pressure, short temperature, unsigned char humidity);
    void SetData(RtcDateTime *dt, unsigned short pressure, short temperature, unsigned char humidity);
    void SetDateTime (RtcDateTime *dt);
    void SetPressure (unsigned short pressure);
    void SetTemperature (short temperature);  // Always in Celsius
    void SetHumidity (unsigned char humidity);
    unsigned char GetHumidity ();
    short         GetTemperature();
    unsigned short GetPressure();
};

Record::Record () {
//  memset (&(this->time_of_recording), 0, sizeof(RtcDateTime));
  memset(this->output_string, ' ', 16);
  this->output_string[17] = '\0';
  SetPressure(760);
  SetTemperature(22);
  SetHumidity(65);
}

Record::Record (RtcDateTime *dt, unsigned short pressure, short temperature, unsigned char humidity) {
  SetData(dt, pressure, temperature, humidity);
}

void Record::SetData (RtcDateTime *dt, unsigned short pressure, short temperature, unsigned char humidity) {
  SetDateTime (dt);
  SetPressure (pressure);
  SetTemperature (temperature);  // Always in Celsius
  SetHumidity (humidity);
}

void Record::SetDateTime (RtcDateTime *dt) {
  if (dt) {
    memcpy (&(this->time_of_recording), dt, sizeof(RtcDateTime));
  } else {
    memset (&(this->time_of_recording), 0, sizeof(RtcDateTime));
  }
}

void Record::SetPressure (unsigned short pressure) {
  this->pressure = pressure;
}

void Record::SetTemperature (short temperature) {
  this->temperature.SetTemperature (temperature);
}

void Record::SetHumidity (unsigned char humidity) {
  this->humidity = humidity;
}
/*
void Record::Display (LiquidCrystal_I2C* lcd) {
  PrintPressure(lcd);
  this->temperature.Display(lcd);
  PrintHumidity(lcd);
}*/

void Record::PrintPressure(LiquidCrystal_I2C* lcd) {
  Serial.print("\tPressure: ");
  Serial.print(this->pressure);

  if (!lcd) { return; }
  lcd->setCursor(0, 1);    // Hardcoded! 0th column, 1st row
  lcd->print(this->pressure);
  lcd->write(LCD_MM_CHARACTER);
}

void Record::PrintHumidity (LiquidCrystal_I2C *lcd) {
  Serial.print("\tHumidity: ");
  Serial.print(this->humidity);

  if (!lcd) { return; }
  lcd->setCursor(12, 1);   // Hardcoded! 11th column, 1st row
  if (this->humidity < 100) {
    lcd->print (" ");
  }
  lcd->print(this->humidity);
  lcd->print("%");  
}

unsigned char Record::GetHumidity () {
  return this->humidity;
}

short Record::GetTemperature () {
  if (global_settings.temperature_unit == TEMPERATURE_CELSIUS) {
    return this->temperature.ToCelsius();
  } else {
    return this->temperature.ToFahrengheit();
  }
}

unsigned short Record::GetPressure () {
  return this->pressure;
}
#endif
/* ==== END Defines ==== */


/* ==== Global Variables ==== */
BME280 bme;                   // Default : forced mode, standby time = 1000 ms
                              // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, DISPLAY_CHARS, DISPLAY_ROWS);
// FaBoLCD_PCF8574 lcd(LCD_I2C_ADDRESS);   // LCD


// Buttons
RBD::Button setButton(SET_BUTTON_PIN);
RBD::Button plusButton(PLUS_BUTTON_PIN);
RBD::Button minusButton(MINUS_BUTTON_PIN);
leOS2 myOS;                   // Timer management

bool led_state = false;
unsigned long time_pressed = 0;
//Record current_record;
unsigned char current_state = STATE_NORMAL;
unsigned char record_to_display = 0;   // The non-saved record
/* ==== END Global Variables ==== */


/* ==== Prototypes ==== */
char char_sign(short data);
/* === Print a message to stream with the temp, humidity and pressure. === */
void printBME280Data(Stream * client);
/* === Print a message to stream with the altitude, and dew point. === */
//void printBME280CalculatedData(Stream* client);
void increase_counter();
/* ==== END Prototypes ==== */

/* ==== Setup ==== */
void setup() {
  Serial.begin(SERIAL_BAUD);
 // while(!Serial) {} // Wait
  while(!bme.begin()){
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  createAdditionalCharacters();

  // Initialize time machine :)
  myOS.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // Turn off the LED

  // Setting up the defaults
  global_settings.temperature_unit = TEMPERATURE_CELSIUS;
  global_settings.frequency_of_recording = 2;
  global_settings.time_of_recording = 8;
  global_settings.date_order = DATE_DD_MM_YY;
  global_settings.date_separator = SEPARATOR_SLASH;
  global_settings.clock_leading_zero = CLOCK_NO_LEADING_ZERO;
  global_settings.clock_type = CLOCK_12H;

  myOS.addTask(printBME280Data, myOS.convertMs(1000), SCHEDULED_IMMEDIATESTART);
/*
  myOS.addTask(AcquireDataFromSensors, myOS.convertMs(1000), SCHEDULED_IMMEDIATESTART);
  delay(500);
  myOS.addTask(DisplaySensorsData, myOS.convertMs(1000), SCHEDULED);
*/
}

/* ==== END Setup ==== */

/* ==== Loop ==== */
void loop() {
  
   
  printBME280Data();
 /*  printBME280CalculatedData(&Serial);
   */
  if (setButton.onPressed()) {
    time_pressed = millis();
    myOS.addTask(increase_counter, myOS.convertMs(20), SCHEDULED_IMMEDIATESTART);
  }
  if (setButton.onReleased()) {
    interrupt_set_button ();
    myOS.removeTask(increase_counter);
  }

  if (plusButton.onPressed()) {
    Serial.println("Plus button pressed");
  }
  if (minusButton.onPressed()) {
    Serial.println("Minus button pressed");
  }
  if (plusButton.onReleased()) {
    Serial.println("Plus button released");
  }
  if (minusButton.onReleased()) {
    Serial.println("Minus button released");
  }

  
}
/* ==== End Loop ==== */

unsigned int count = 0;
void increase_counter() {
  count++;
  Serial.println ("Increased counter");
}

void interrupt_set_button () {
    unsigned long current_delay = 0;
    current_delay = millis() - time_pressed;
    if (current_delay >= SET_BUTTON_TIMEOUT) {
      switch_led();
    }
    Serial.print("The delay was ");
    Serial.print(current_delay);
    Serial.println(" millis");
    Serial.print("Counter was increased ");
    Serial.print(count);
    Serial.println(" times");
    Serial.print("Status of the task is ");
    unsigned char status_task = (unsigned char)myOS.getTaskStatus(increase_counter);
    Serial.println(status_task);
    
    time_pressed = 0;
    count = 0;
}

void switch_led() {
  if (led_state) {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Switching LED off --");
  } else {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Switching LED on ^^");
  }
  led_state = !led_state;
}

/* ==== Functions ==== */
void printTemperature(int temperature) {
  lcd.print (char_sign(temperature));
  lcd.print (temperature);
//  if (global_settings.temperature_unit == TEMPERATURE_CELSIUS) {
    lcd.write(LCD_DEGREE_C_CHARACTER);
//  } else {
//    lcd.write(LCD_DEGREE_F_CHARACTER);
//  }

}

void printBME280Data(void) {
  static unsigned long loc_time = 0;
  unsigned long millis_time = millis();
  float temp(NAN), hum(NAN), pres(NAN);
  static unsigned long turn = 0;

  // Parameters: (float& pressure, float& temp, float& humidity, bool hPa = true, bool celsius = false)
  uint8_t pressureUnit(2);

  if (loc_time == 0 || (millis_time - 1000) >= loc_time || millis_time < loc_time) {
    loc_time = millis_time;
  } else {
    return;
  }
  bme.ReadData(pres, temp, hum, true, pressureUnit);
  temp = bme.ReadTemperature(true);
  /* Alternatives to ReadData():
    float ReadTemperature(bool celsius = false);
    float ReadPressure(uint8_t unit = 0);
    float ReadHumidity();

    Keep in mind the temperature is used for humidity and
    pressure calculations. So it is more effcient to read
    temperature, humidity and pressure all together.
   */
/*  client->print("Temp: ");
  client->print(temp);
  client->print("°"+ String(metric ? 'C' :'F'));
  client->print("\t\tHumidity: ");
  client->print(hum);
  client->print("% RH");
  client->print("\t\tPressure: ");
  client->print(pres * 25.4);
  client->print(" mm Hg");
*/
  lcd.setCursor(0, 1);
  lcd.print(round(pres * 25.4));
//  lcd.write(LCD_MM_CHARACTER);
  lcd.write (LCD_MBAR_CHARACTER);
  lcd.print("  ");
//  printTemperature(round(temp));
  if (temp > 0) { lcd.print ("+"); }
  else if (temp < 0) { lcd.print ("-"); }
  else { lcd.print(" "); }
  lcd.print (abs(round(temp)));
  lcd.write(LCD_DEGREE_C_CHARACTER);
  lcd.print(" ");
  if (hum < 100) {
    lcd.print (" ");
  }
  lcd.print(round(hum));
  lcd.print("%");

  /* Checking Fahrengheit display
  switch_to_fahrengheit++;
  if (switch_to_fahrengheit >= 20) {
    switch_to_fahrengheit = 0;
    celsius = !celsius;
  }
  */
}

/*
void printBME280CalculatedData(Stream* client){
  float altitude = bme.CalculateAltitude(metric);
  float dewPoint = bme.CalculateDewPoint(metric);
  client->print("\t\tAltitude: ");
  client->print(altitude);
  client->print((metric ? "m" : "ft"));
  client->print("\t\tDew point: ");
  client->print(dewPoint);
  client->println("°"+ String(metric ? 'C' :'F'));
}
*/
#if 0
void AcquireDataFromSensors (void) {
  RtcDateTime dt;
  float temp(NAN), hum(NAN), pres(NAN);
  unsigned short pressure;
  unsigned char  humidity;
  short          temperature;
  // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi
  uint8_t pressureUnit(2);
  bme.ReadData(pres, temp, hum, true, pressureUnit);  // Metric system, inches Hg
  temp = bme.ReadTemperature(true);
  pressure = (unsigned short)round(pres * 25.4);      // Converting to mm Hg
  temperature = (short )round(temp);
  humidity = (unsigned char)round(hum);
  current_record.SetData (&dt, pressure, temperature, humidity);
}

void DisplaySensorsData (void) {
  short temperature, humidity;
  lcd.setCursor(0, 1);
  if (current_state == STATE_NORMAL) {
    if (record_to_display == 0) {
      temperature = current_record.GetTemperature();
      humidity = current_record.GetHumidity();
      lcd.print (current_record.GetPressure());
      lcd.write (LCD_MM_CHARACTER);
//      lcd.write (LCD_MBAR_CHARACTER);
      lcd.print ("  ");
      lcd.print (char_sign(temperature));
      if (global_settings.temperature_unit == TEMPERATURE_CELSIUS) {
        lcd.write(LCD_DEGREE_C_CHARACTER);
      } else {
        lcd.write(LCD_DEGREE_F_CHARACTER);
      }
      lcd.print ("  ");
      if (humidity < 100) {
        lcd.print (" ");
      }
      lcd.print (humidity);
      lcd.print ("%");      
    }
  }
}
#endif

void createAdditionalCharacters(void) {
  byte degreeC_Char[8] = {
    0b01000,
    0b10100,
    0b01000,
    0b00011,
    0b00100,
    0b00100,
    0b00011,
    0b00000
  };
  byte degreeF_Char[8] = {
    0b01000,
    0b10100,
    0b01000,
    0b00111,
    0b00100,
    0b00111,
    0b00100,
    0b00000
  };
  byte mmChar[8] = {
    0b11010,
    0b10101,
    0b10001,
    0b00000,
    0b11010,
    0b10101,
    0b10001,
    0b00000
  };
  byte mbarChar[8] = {
    0b11011,
    0b10101,
    0b10001,
    0b01000,
    0b01110,
    0b01001,
    0b01110,
    0b00000
  };
  lcd.createChar(LCD_DEGREE_C_CHARACTER, degreeC_Char);
  lcd.createChar(LCD_DEGREE_F_CHARACTER, degreeF_Char);
  lcd.createChar(LCD_MM_CHARACTER, mmChar);
  lcd.createChar(LCD_MBAR_CHARACTER, mbarChar);
}

int sign(short data) {
  if (data == 0) return 0;
  return (int)((data / (abs(data))));
}

char char_sign(short data) {
  char sign_ = sign(data);
  if (sign_ > 0) return '+';
  if (sign_ < 0) return '-';
  return ' ';
}
/* ==== END Functions ==== */
