# Battery Capacity Tester Arduino NANO
	Another Battery capacity tester.
	Create from an idea of http://www.instructables.com/id/DIY-Arduino-Battery-Capacity-Tester-V10-/.

## Wire schema

![Breadboard schema](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/BTester_bb.png)
![Schema](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/BTester_schem.png)

## Assembly list
	Label Part Type		 						Properties
	10K Trimmer potentiometer					TRIMPOT	package trimpot_pth_s3_lock_3386P; variant variant 2
	18650 Battery								lipo
	2N2222	NPN-Transistor						tipo NPN (EBC); package TO92 [THT]; part number 2N2222
	Beeper	Piezo Speaker	
	Display	LCD screen							tipo Character; pins 16
	For battery	Screw terminal - 2 pins			package THT; hole size 1.0mm,0.508mm; pins 2; pin spacing 0.1in (2.54mm)
	For power resistor	Screw terminal - 2 pins	package THT; hole size 1.0mm,0.508mm; pins 2; pin spacing 0.1in (2.54mm)
	i2c controller	PCF8574						tipo PCF8574; package DIP16 [THT]
	IRF744N	Basic FET N-Channel					tipo n-channel; package TO220 [THT]
	MF58	Temperature Sensor (Thermistor)		tipo thermistor; package THT; resistance at 25° 10kΩ; thermistor type NTC
	MF58	Temperature Sensor (Thermistor)		tipo thermistor; package THT; resistance at 25° 10kΩ; thermistor type NTC
	Microcontroller	Arduino Pro Mini clone (compatible Nano)	tipo Arduino Pro Mini (Clone comp Nano); variant variant 1
	Power resistor								10kΩ Resistor
	R1	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil
	R2	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil
	R3	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil
	R4	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil
	R10	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil
	R11	10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil

## Shopping List

	Amount	Part Type								Properties
	1		TRIMPOT									package trimpot_pth_s3_lock_3386P; variant variant 2
	1		LIPO									18650 variant 
	1		NPN-Transistor							tipo NPN (EBC); package TO92 [THT]; part number 2N2222
	1		Piezo Speaker	
	1		LCD screen								tipo Character; pins 16
	2		Screw terminal - 2 pins					package THT; hole size 1.0mm,0.508mm; pins 2; pin spacing 0.1in (2.54mm)
	1		PCF8574									tipo PCF8574; package DIP16 [THT]
	1		Basic FET N-Channel						tipo n-channel; package TO220 [THT]
	2		Temperature Sensor (Thermistor)			tipo thermistor; package THT; resistance at 25° 10kΩ; thermistor type NTC
	1		Arduino Pro Mini clone (compatible Nano)tipo Arduino Pro Mini (Clone comp Nano); variant variant 1
	1		10kΩ Resistor							Power resistor
	6		10kΩ Resistor							package THT; tolerance ±5%; resistenza 10kΩ; bands 4; pin spacing 400 mil

## Realization
	To measuring voltage we use the principle of Voltage divider
	https://en.wikipedia.org/wiki/Voltage_divider
	https://startingelectronics.org/articles/arduino/measuring-voltage-with-arduino/
	in simple word this code 
		batVolt = (sample1 / (1023.0 - ((batResValueGnd / (batResValueVolt + batResValueGnd)) * 1023.0))) * vcc;
	that measure battery voltage is:
	batResValueGnd / (batResValueVolt + batResValueGnd) --> this is the multipler factor of the reading voltage because I inserted 2 resistance of batResValueVolt and batResValueGnd ohom value after and before the analog read wire;
	sample1 --> is the average analog readings;
	vcc --> reference arduino voltage;
	1023.0 --> is the reference maximun value of analog read (arduino analog read go from 0 to 1023)
	
	The principle is that we measure the voltage afther and before the power resistor and so we can calculate millihampere that consumes the battery.
	The MOSFET is used to stard and stop battery drain from power resistor.
	I'm pretty scary, so I inserted 2 thermistors to monitorage battery and power resistor temperature.

![Breadboard](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/breadboard01.jpg)
![lcd on discharging](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/lcdDischarging02.jpg)

Thermistor on power resistor (heat shrink sleeve to hold).

![Thermistor on power resistance](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/thermistorPowerResistance.jpg)

Thermistor on battery.

![Thermistor on battery](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/thermistorBattery.jpg)

