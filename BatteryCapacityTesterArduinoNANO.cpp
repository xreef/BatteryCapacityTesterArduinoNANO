// Do not remove the include below
#include "BatteryCapacityTesterArduinoNANO.h"

#include "LiquidCrystal_I2C.h"
#include "Thermistor.h"
#include <Wire.h>

#define MOSFET_Pin 2
#define BAT_PIN A0
#define RES_PIN A1
#define BATTERY_THERM_PIN A2
#define RESISTA_THERM_PIN A3
#define BUZZER_PIN 9

#define BATTERY_MAX_TEMP 50
#define RESISTANCE_MAX_TEMP 70

float batResValueGnd = 10;
float batResValueVolt = 10;

float resResValueGnd = 10;
float resResValueVolt = 10;

float capacity = 0.0; // capacity in mAh
float resValue = 10.2;  // Resistor Value in Ohm
float vcc = 5.04; // Voltage of Arduino 5V pin ( Mesured by Multimeter )
float tolerance = 0.1;
float current = 0.0; // current in Amp
float mA = 0;         // current in mA
float batVolt = 0.0;  // Battery Voltage
float realBatVolt = 0.0;
float resVolt = 0.0;  // Voltage at lower end of the Resistor
float batHigh = 4.3; // Battery High Voltage
float batLow = 2.9; // Discharge Cut Off Voltage
unsigned long previousMillis = 0; // Previous time in ms
unsigned long millisPassed = 0;  // current time in ms
unsigned long waitMillisPassed = 3000;  // current time in ms
float sample1 = 0;
float sample2 = 0;
int x = 0;
int row = 0;

int loadLenght = 3;

int mosfetStatus = LOW;

LiquidCrystal_I2C lcd(0x38, 16, 2);

Thermistor thermBattery(BATTERY_THERM_PIN);
Thermistor thermResistance(RESISTA_THERM_PIN);

byte loader[][8] = { { // 0
		B00001,
		B00011,
		B00110,
		B01110,
		B01100,
		B11000,
		B10000 }, { // 1
		B00000,
		B00000,
		B00000,
		B11111,
		B00000,
		B00000,
		B00000 }, { // 2
		B10000,
		B11000,
		B01100,
		B01110,
		B00110,
		B00011,
		B00001 }, { // no battery load + 0
		B01110,
		B11011,
		B11001,
		B10101,
		B10101,
		B10011,
		B11111 }, { // down load+1
		B01110,
		B01110,
		B01110,
		B01110,
		B11111,
		B01110,
		B00100 }, { // up load+2
		B00100,
		B01110,
		B11111,
		B01110,
		B01110,
		B01110,
		B01110 }, {  // up load+3
		B00110,
		B01001,
		B00110,
		B00000,
		B00000,
		B00000,
		B00000, }, {  // up load+4
		B00100,
		B01010,
		B01010,
		B01010,
		B11011,
		B10001,
		B01110, } };

int prevStatus = 0;
bool dischargingStarted = false;
float startingVcc = 4.2;

uint8_t lastTempVisualizedBattery = 0;
int loadingCursor = 0;

float batteryTemp = NAN;
float resistanceTemp = NAN;

void draw(void);
void beep(unsigned char delay_time);
long readVcc(void);

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(115200);

	pinMode(MOSFET_Pin, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);
	digitalWrite(MOSFET_Pin, LOW);  // MOSFET is off during the start
	mosfetStatus = LOW;  // MOSFET is off during the start
	Serial.println("CLEARDATA");

	lcd.init();

	lcd.clear();

	int i;
	for (i = 0; i < loadLenght + 5; i++) {
		lcd.createChar(i, loader[i]);
	}

	lcd.setBacklight(LOW);
}

// The loop function is called in an endless loop
void loop() {

	vcc = readVcc() / 1000.0;

	//************ Measuring Battery Voltage ***********

	for (int i = 0; i < 100; i++) {
		sample1 = sample1 + analogRead(BAT_PIN); //read the voltage from the divider circuit
		delay(2);
	}
	sample1 = sample1 / 100;
	batVolt = (sample1
			/ (1023.0
					- ((batResValueGnd / (batResValueVolt + batResValueGnd))
							* 1023.0))) * vcc;
	// *********  Measuring Resistor Voltage ***********

	for (int i = 0; i < 100; i++) {
		sample2 = sample2 + analogRead(RES_PIN); //read the voltage from the divider circuit
		delay(2);
	}
	sample2 = sample2 / 100;
	resVolt = (sample2
			/ (1023.0
					- ((resResValueGnd / (resResValueVolt + resResValueGnd))
							* 1023.0))) * vcc;

	realBatVolt = (mosfetStatus == HIGH) ? batVolt + resVolt : batVolt;

	//********************* Checking the different conditions *************

	batteryTemp = thermBattery.readTemperature();
	resistanceTemp = thermResistance.readTemperature();

	if (batteryTemp > BATTERY_MAX_TEMP || resistanceTemp > RESISTANCE_MAX_TEMP) {
		digitalWrite(MOSFET_Pin, LOW); // Turned Off the MOSFET // No discharge
		mosfetStatus = LOW;
		beep(100);
		// Reset millis
		previousMillis = millis();
		//	    Serial.print( "Warning High-V! ");
		//		Serial.println( batVolt,2);  // display Battery Voltage in Volt
		delay(1000);
	} else if (realBatVolt > batHigh) {
		digitalWrite(MOSFET_Pin, LOW); // Turned Off the MOSFET // No discharge
		mosfetStatus = LOW;
		beep(200);
//	    Serial.print( "Warning High-V! ");
//		Serial.println( batVolt,2);  // display Battery Voltage in Volt
		delay(1000);
	}

	else if (realBatVolt < batLow) {
		digitalWrite(MOSFET_Pin, LOW);
		mosfetStatus = LOW;
		beep(200);
//	      Serial.println( "Warning Low-V! ");
		delay(1000);
	} else if (realBatVolt > batLow && realBatVolt < batHigh
			&& (previousMillis == 0
					|| (millis() - previousMillis) > waitMillisPassed)) { // Check if the battery voltage is within the safe limit
		digitalWrite(MOSFET_Pin, HIGH);
		mosfetStatus = HIGH;
		millisPassed = millis() - previousMillis;
		current = (batVolt - resVolt) / resValue;
		mA = current * 1000.0;
		capacity = capacity + mA * (millisPassed / 3600000.0); // 1 Hour = 3600000ms
		previousMillis = millis();
//	      Serial.print("DATA,TIME,"); Serial.print(batVolt); Serial.print(","); Serial.println(capacity);
		row++;
		x++;

		delay(1000);
	}

	draw();
}

//************************ OLED Display Draw Function *******************************************************
void draw(void) {
	if (batteryTemp > BATTERY_MAX_TEMP || resistanceTemp > RESISTANCE_MAX_TEMP) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 4));

			lcd.setCursor(2, 0);
			if (batteryTemp > BATTERY_MAX_TEMP) {
				lcd.print("BHOT");
			} else {
				lcd.print("RHOT");
			}
		} else if (prevStatus != 1) {
			Serial.print("Battery temp: ");
			Serial.print(batteryTemp);
			Serial.print("*C, Resistance temp: ");
			Serial.print(resistanceTemp);
			Serial.println("*C");

			lcd.setCursor(0, 0);
			lcd.print("Res: ");
			lcd.print(resistanceTemp);
			lcd.write(byte(loadLenght + 3));
			lcd.print("C");

			lcd.setCursor(0, 1);
			lcd.print("Bat: ");
			lcd.print(batteryTemp, 2);
			lcd.write(byte(loadLenght + 3));
			lcd.print("C");
		}
	} else if (realBatVolt < 1) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 0));

			lcd.setCursor(2, 0);
			lcd.print("Batt");
		} else if (prevStatus != 1) {
			Serial.println("No Battery!");

			lcd.setCursor(0, 0);
			lcd.print("No Battery!");
		}
		prevStatus = 1;

	} else if (realBatVolt > batHigh) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 2));

			lcd.setCursor(2, 0);
			lcd.print("High");
		} else if (prevStatus != 2) {
			lcd.setCursor(0, 0);
			lcd.print("High-V!");

			Serial.print("High-V!");

			Serial.print(" - ");

			Serial.print("Volt: ");
			Serial.print(realBatVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("DISCHARGING: ");
			Serial.println(mosfetStatus);     // display capacity in mAh

		}
		prevStatus = 2;
	} else if (realBatVolt < batLow) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 1));

			lcd.setCursor(2, 0);
			lcd.print("Low ");
		} else if (prevStatus != 3) {
			lcd.setCursor(0, 0);
			lcd.print("Low-V!");

			Serial.print("Low-V!");

			Serial.print(" - ");

			Serial.print("Volt: ");
			Serial.print(realBatVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("DISCHARGING: ");
			Serial.println(mosfetStatus);     // display capacity in mAh

		}
		prevStatus = 3;

	} else if (realBatVolt >= batLow && realBatVolt < batHigh) {
		if (!dischargingStarted) {
			startingVcc = realBatVolt;
		}
		dischargingStarted = true;
		if (!(prevStatus == 3 && ((realBatVolt - tolerance) < batLow))) {
			lcd.clear();

			lcd.setCursor(0, 0);
			lcd.write(byte(loadingCursor));

			loadingCursor++;
			if (loadingCursor >= loadLenght)
				loadingCursor = 0;

			int percentage = (realBatVolt - batLow) * 100
					/ (startingVcc - batLow);

			if (percentage > 100)
				percentage = 100;

			lcd.setCursor(2, 0);
			lcd.print(percentage);
			lcd.print("%");

			lcd.setCursor(7, 0);

			lcd.print("mAh:");
			lcd.print(capacity, 1);     // display capacity in mAh

			lcd.setCursor(0, 1);
			lcd.print(realBatVolt, 2);  // display Battery Voltage in Volt
			lcd.print("V");

			lcd.print(" ");

			lcd.print(mA, 0);  // display current in mA
			lcd.print("mA");

			if (lastTempVisualizedBattery < 3) {
				lcd.print(" B");

				lcd.print(batteryTemp, 0);  // display current in mA
				lcd.write(byte(loadLenght + 3));

			} else if (lastTempVisualizedBattery < 7) {
				lcd.print(" R");

				lcd.print(resistanceTemp, 0);  // display current in mA
				lcd.write(byte(loadLenght + 3));
			}
			lastTempVisualizedBattery++;
			if (lastTempVisualizedBattery >= 7) {
				lastTempVisualizedBattery = 0;
			}

			Serial.print("Volt: ");
			Serial.print(realBatVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("B Volt: ");
			Serial.print(batVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("M Volt: ");
			Serial.print(((batVolt + realBatVolt) / 2), 2); // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("Res Volt: ");
			Serial.print(resVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("Curr: ");
			Serial.print(mA, 0);  // display current in mA
			Serial.print("mA");

			Serial.print(" - ");

			Serial.print("mAh: ");
			Serial.print(capacity, 1);     // display capacity in mAh

			Serial.print(" - ");

			Serial.print("DISCHARGING: ");
			Serial.print(mosfetStatus);     // display capacity in mAh

			Serial.print(" - ");

			Serial.print("Battery temp: ");
			Serial.print(batteryTemp, 2);     // display capacity in mAh

			Serial.print(" - ");

			Serial.print("Resistance temp: ");
			Serial.println(resistanceTemp, 2);     // display capacity in mAh
		}
		prevStatus = 4;
	}
}
//******************************Buzzer Beep Function *********************************************************

void beep(unsigned char delay_time) {
	analogWrite(9, 20);      // PWM signal to generate beep tone
	delay(delay_time);          // wait for a delayms ms
	analogWrite(BUZZER_PIN, 0);  // 0 turns it off
	delay(delay_time);          // wait for a delayms ms
}

long readVcc() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC))
		; // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;

	result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}
