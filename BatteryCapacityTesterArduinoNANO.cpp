// Do not remove the include below
#include "BatteryCapacityTesterArduinoNANO.h"

#include "LiquidCrystal_I2C.h"
#include "Thermistor.h"
#include <Wire.h>

#define CHANGE_BATTERY_BUTTON 2
#define SELECT_BATTERY_BUTTON 3

#define MOSFET_PIN 5
#define BAT_PIN A0
#define RES_PIN A1
#define BATTERY_THERM_PIN A3
#define RESISTA_THERM_PIN A2
#define BUZZER_PIN 9

#define BATTERY_MAX_TEMP 50
#define RESISTANCE_MAX_TEMP 60

// Battery voltage resistance
#define BAT_RES_VALUE_GND 10.0
#define BAT_RES_VALUE_VCC 10.0
// Power resistor voltage resistance
#define RES_RES_VALUE_GND 10.0
#define RES_RES_VALUE_VCC 10.0

#define NO_BATTERY_VOLTAGE 1

#define USING_BATTERY_TERMISTOR true
#define USING_RESISTO_TERMISTOR true

#define NUMBER_OF_LOW_TO_RAISE  5
#define WAIT_AFTER_LOW 4000

uint8_t lowRaised = 0;

float capacity = 0.0; 	// capacity in mAh
float resValue = 10.0;  // Resistor Value in Ohm
float vcc = 5.04; 		// Voltage of Arduino 5V pin ( overwritten by function )
float minVoltageDropoutTolerance = 0.1; // If min value declared is 2.9 i apply 2.8 to compensate voltage oscillation
//float tolerance = 0.2; 	// When raise first time the low voltage at next check I apply this tolerance to not restart if It's raise the value
float current = 0.0; 	// current in Amp
float mA = 0;         	// current in mA
float batVolt = 0.0;  	// Battery Voltage
float realBatVolt = 0.0;// Battery voltage + resistance voltage
float resVolt = 0.0;  	// Voltage at lower end of the Resistor

// Valiable to manage some behaivor
int prevStatus = 0; 				// Last state
bool dischargingStarted = false;	// Fist time starting discharging
float startingVcc = 4.2; 			// Voltage at the start of discharging (overwritten at first time)

// Structure of battery type
struct BatteryType {
	char name[10];
	float maxVolt;
	float minVolt;
};

#define BATTERY_TYPE_NUMBER 4
BatteryType batteryTipes[BATTERY_TYPE_NUMBER] = {
		{ "18650", 4.3, 2.9 },
		{ "17550", 4.3, 2.9 },
		{ "14500", 4.3, 2.75 },
		{ "6v Acid", 6.50, 5.91  }
};

// Variable to use in discharging time
float batHigh = 4.3; 	// Battery High Voltage
float batLow = 2.9; 	// Discharge Cut Off Voltage

// Value of button
// button of change battery
uint8_t buttonChangeState = LOW;
// button of select battery
uint8_t buttonSelectState = LOW;

// Temporary selected battery
uint8_t tmpBatterySelected = 0;
// Selected battery
int8_t batterySelected = -1;
// First time must refresh display
bool displayRefresh = false;

unsigned long previousMillis = 0; 		// Previous time in ms
unsigned long millisPassed = 0;  		// Using this variable to update screen more than update values (for animated effect)
unsigned long waitMillisPassed = 3000;  // current time in ms
int x = 0;
int row = 0;

// Variable to hold mosfet status
int mosfetStatus = LOW;

// Inizialize thermistor
Thermistor thermBattery(BATTERY_THERM_PIN);
Thermistor thermResistance(RESISTA_THERM_PIN);

// Inizialize display
LiquidCrystal_I2C lcd(0x38, 16, 2);
// Additional display character
int loadLenght = 3;
byte loader[][8] = { { // 0 loader
		B00001,
		B00011,
		B00110,
		B01110,
		B01100,
		B11000,
		B10000 }, { // 1 loader
		B00000,
		B00000,
		B00000,
		B11111,
		B00000,
		B00000,
		B00000 }, { // 2 loader
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
		B11111 }, { // arrow down load+1
		B01110,
		B01110,
		B01110,
		B01110,
		B11111,
		B01110,
		B00100 }, { // arrow up load+2
		B00100,
		B01110,
		B11111,
		B01110,
		B01110,
		B01110,
		B01110 }, {  // ° load+3
		B00110,
		B01001,
		B00110,
		B00000,
		B00000,
		B00000,
		B00000, }, {  // thermometer load+4
		B00100,
		B01010,
		B01010,
		B01010,
		B11011,
		B10001,
		B01110, } };

uint8_t lastTempVisualizedBattery = 0;
int loadingCursor = 0;

float batteryTemp = NAN;
float resistanceTemp = NAN;

void draw(void);
void beep(unsigned char delay_time);
long readVcc(void);

void setup() {
	Serial.begin(115200);

	pinMode(MOSFET_PIN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);
	digitalWrite(MOSFET_PIN, LOW);  // MOSFET is off during the start
	mosfetStatus = LOW;  			// MOSFET status is off during the start
	Serial.println("CLEARDATA");

	// Inizialize LCD
	lcd.init();
	lcd.clear();
	int i;
	for (i = 0; i < loadLenght + 5; i++) {
		lcd.createChar(i, loader[i]);
	}
	lcd.setBacklight(LOW);

	pinMode(SELECT_BATTERY_BUTTON, INPUT);
	pinMode(CHANGE_BATTERY_BUTTON, INPUT);
}

// The loop function is called in an endless loop
void loop() {
	vcc = readVcc() / 1000.0;

	//************ Measuring Battery Voltage ***********
	float sample1 = 0;
	float sample2 = 0;

	for (int i = 0; i < 100; i++) {
		sample1 = sample1 + analogRead((uint8_t)BAT_PIN); //read the voltage from the divider circuit
		delay(2);
	}
	sample1 = sample1 / 100;
	batVolt = (sample1
			/ (1023.0
					- ((BAT_RES_VALUE_GND / (BAT_RES_VALUE_VCC + BAT_RES_VALUE_GND))
							* 1023.0))) * vcc;

	// *********  Measuring Resistor Voltage ***********
	for (int i = 0; i < 100; i++) {
		sample2 = sample2 + analogRead((uint8_t)RES_PIN); //read the voltage from the divider circuit
		delay(2);
	}
	sample2 = sample2 / 100;
	resVolt = (sample2
			/ (1023.0
					- ((RES_RES_VALUE_GND / (RES_RES_VALUE_VCC + RES_RES_VALUE_GND))
							* 1023.0))) * vcc;

	realBatVolt = (mosfetStatus == HIGH) ? batVolt + resVolt : batVolt;

	// ************* Read thermistor temperature ***********************
	batteryTemp = thermBattery.readTemperature();
	resistanceTemp = thermResistance.readTemperature();

	//********************* Checking the different conditions *************
	// No battery selected
	if (batterySelected==-1){
		buttonChangeState = digitalRead(CHANGE_BATTERY_BUTTON);
		if (buttonChangeState==HIGH) {
			tmpBatterySelected++;
			if (tmpBatterySelected>=BATTERY_TYPE_NUMBER){
				tmpBatterySelected = 0;
			}
			displayRefresh = true;
		}else{
			displayRefresh = false;
		}

		buttonSelectState = digitalRead(SELECT_BATTERY_BUTTON);
		if (buttonSelectState==HIGH) {
			batterySelected = tmpBatterySelected;
			batHigh = batteryTipes[tmpBatterySelected].maxVolt; // Battery High Voltage
			batLow = batteryTipes[tmpBatterySelected].minVolt - minVoltageDropoutTolerance; // Discharge Cut Off Voltage
//			startingVcc = batHigh;
			// I put batt low value so I show 0% until the correct voltage is grabbed
			startingVcc = batLow;
		}
		delay(10);

		// Reset millis
		previousMillis = millis();
	// Temperature warning
	}else if ((USING_BATTERY_TERMISTOR && batteryTemp > BATTERY_MAX_TEMP) || (USING_RESISTO_TERMISTOR && resistanceTemp > RESISTANCE_MAX_TEMP)) {
		digitalWrite(MOSFET_PIN, LOW); // Turned Off the MOSFET // No discharge
		mosfetStatus = LOW;
		beep(100);
		delay(10000);

		// Reset millis
		previousMillis = millis();
	// Voltage too hight
	} else if (realBatVolt > batHigh) {
		digitalWrite(MOSFET_PIN, LOW); // Turned Off the MOSFET // No discharge
		mosfetStatus = LOW;
		beep(200);
		delay(1000);

		// Reset millis
		previousMillis = millis();
	// No battery
	} else if (realBatVolt < NO_BATTERY_VOLTAGE) {
		digitalWrite(MOSFET_PIN, LOW);
		mosfetStatus = LOW;
		beep(200);
		delay(1000);

		// Reset millis
		previousMillis = millis();
	// Voltage too low
	} else if (realBatVolt < batLow || lowRaised>NUMBER_OF_LOW_TO_RAISE) {
		// Now discharging is considered started when al least wone time is calculated maha
		// So need to check mosfet status if some people attach discharged battery to device
		if (dischargingStarted || mosfetStatus==HIGH) {
			dischargingStarted = true;
			if (lowRaised<=NUMBER_OF_LOW_TO_RAISE) lowRaised++; // To prevent overflow
		}

		digitalWrite(MOSFET_PIN, LOW);
		mosfetStatus = LOW;
		beep(200);
		delay(WAIT_AFTER_LOW);
	// Discharging ok
	} else if (realBatVolt > batLow && realBatVolt < batHigh
			&& (previousMillis == 0
					|| (millis() - previousMillis) > waitMillisPassed)) { // Check if the battery voltage is within the safe limit
		// startingVcc used to get percentage of discharging
		if (!dischargingStarted && mosfetStatus == HIGH) {
			startingVcc = realBatVolt;
			dischargingStarted = true;
		}

		// When battery is under discharging change his voltage and now I reset previous millis to have a resonable value
		if (mosfetStatus==LOW){
			digitalWrite(MOSFET_PIN, HIGH);
			mosfetStatus = HIGH;
			delay(2000);
			previousMillis = millis();
		}else{
			millisPassed = millis() - previousMillis;
			current = (batVolt - resVolt) / resValue;
			mA = current * 1000.0;
			capacity = capacity + mA * (millisPassed / 3600000.0); // 1 Hour = 3600000ms
			previousMillis = millis();
			row++;
			x++;
		}
		delay(1000);
	}

	draw();
}

bool firstTimeRefresh = true;
/************************ Display Draw Function *******************************************************
 * Change this if you use a different display
 */

void draw(void) {
	// First time of battery selected refresh lcd
	if (batterySelected!=-1 && prevStatus==0){
		lcd.clear();
	}

	// If not already select a battery type
	if (batterySelected==-1){
		if (displayRefresh || firstTimeRefresh){
			lcd.clear();

			lcd.setCursor(0, 0);
			lcd.print("B Tp: ");
			lcd.print(batteryTipes[tmpBatterySelected].name);
			lcd.setCursor(0, 1);
			lcd.print("Volt: ");
			lcd.print(batteryTipes[tmpBatterySelected].minVolt, 1);
			lcd.print("V/");
			lcd.print(batteryTipes[tmpBatterySelected].maxVolt,1 );
			lcd.print("V");

			firstTimeRefresh = false;
		}
	// If battery or resistance temp is out of range
	}else if (batteryTemp > BATTERY_MAX_TEMP || resistanceTemp > RESISTANCE_MAX_TEMP) {
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
	// No battery
	} else if (realBatVolt < NO_BATTERY_VOLTAGE) {
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

	// Battery votlage is wrong, probably wrong type of battery
	} else if (realBatVolt > batHigh) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 2));

			lcd.setCursor(2, 0);
			lcd.print("High");
		} else if (prevStatus != 2) {
			lcd.setCursor(0, 0);
			lcd.print("High-V!");

			lcd.setCursor(0, 1);
			lcd.print("Wrong battery!");

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
	// Battery is discharged
	} else if (realBatVolt < batLow  || lowRaised>NUMBER_OF_LOW_TO_RAISE) {
		if (dischargingStarted /*prevStatus==4*/) {
			lcd.setCursor(0, 0);
			lcd.write(byte(loadLenght + 1));

			lcd.setCursor(2, 0);
			lcd.print("Low ");

			// Need to display voltege battery when It's start discharged
			if (prevStatus != 3){
				lcd.setCursor(0, 1);
				lcd.print(realBatVolt, 2);  // display Battery Voltage in Volt
				lcd.print("V");
			}
		} else if (prevStatus != 3) {
			lcd.setCursor(0, 0);
			lcd.print("Low-V!");

			lcd.setCursor(0, 1);
			lcd.print(realBatVolt, 2);  // display Battery Voltage in Volt
			lcd.print("V");
		}



		Serial.print("Low-V!");

		Serial.print(" - ");

		Serial.print("Volt: ");
		Serial.print(realBatVolt, 2);  // display Battery Voltage in Volt
		Serial.print("V");

		Serial.print(" - ");
		Serial.print("lowRaised");
		Serial.print(lowRaised);  // display Battery Voltage in Volt

		Serial.print(" - ");

		Serial.print("DISCHARGING: ");
		Serial.println(mosfetStatus);     // display capacity in mAh

		prevStatus = 3;

	// Discharging time
	} else if (realBatVolt >= batLow && realBatVolt < batHigh) {
		// If prevState is low voltage and voltage less the tolerance not greater than battery low voltage cycle is finished
//		if (!(prevStatus == 3 && ((realBatVolt - tolerance) < batLow))) {
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

			if ((USING_BATTERY_TERMISTOR && lastTempVisualizedBattery < 3)) {
				lcd.print(" B");

				lcd.print(batteryTemp, 0);  // Battery Temp in °C
				lcd.write(byte(loadLenght + 3));

			} else if ((USING_RESISTO_TERMISTOR && lastTempVisualizedBattery < 7)) {
				lcd.print(" R");

				lcd.print(resistanceTemp, 0);  // Resitor Temp in °C
				lcd.write(byte(loadLenght + 3));
			}
			lastTempVisualizedBattery++;
			if (lastTempVisualizedBattery >= 7) {
				lastTempVisualizedBattery = 0;
			}

			Serial.print("Volt: ");
			Serial.print(realBatVolt, 2);  // display Battery Voltage + res voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("B Volt: ");
			Serial.print(batVolt, 2);  // display Battery Voltage in Volt
			Serial.print("V");

			Serial.print(" - ");

			Serial.print("Res Volt: ");
			Serial.print(resVolt, 2);  // display Resistor Voltage in Volt
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
			Serial.print(mosfetStatus);     // display mosfet status

			Serial.print(" - ");

			Serial.print("Battery temp: ");
			Serial.print(batteryTemp, 2);     // display battery temp

			Serial.print(" - ");

			Serial.print("Resistance temp: ");
			Serial.println(resistanceTemp, 2);     // display resistance temp
//		}
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
