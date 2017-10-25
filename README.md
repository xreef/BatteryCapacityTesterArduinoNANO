# Battery Capacity Tester Arduino NANO #
Another Battery capacity tester.
With this device you can check the 18650 battery capacity. For safe I added with thermistor the temperature of power resistance and battery to prevent explosion.
I'm doing some change from original project and now It's quite fine:
v0.1
 - VCC of Arduino now It's automatically calculated;
 - Voltage on screen now It's correct; 
 - Added variable to change setting in more confortable way.
 - Added percentage of discharging
 - Added temperature of battery and power resistor
v0.2
 - Added possiblity of battery selection

In project you can find fritzing, schema, photo and other.

*I use generic character display, and I build the i2c controller and use It with my custom library, but if you want You can take a normal i2c controller (less than 1€) with standard library, the code remain the same.*
*All code of display is in draw function so you can change that without change other.*

Created from an idea of [OpenGreenEnergy](http://www.instructables.com/id/DIY-Arduino-Battery-Capacity-Tester-V10-/).

## Wire schema ##

![Breadboard schema](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/BTester_bb.png)
![Schema](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/BTester_schem.png)

## Assembly list ##
| Label | Part Type | Properties |
| --- | --- | --- |
| **(LCD bb)** Display* | LCD screen | pins 16; tipo Character |
| **(LCD bb)** i2c controller* | PCF8574 | tipo PCF8574; package DIP16 [THT] |
| **(LCD bb)** 10K Trimmer potentiometer* | TRIMPOT | variant variant 2; package trimpot_pth_s3_lock_3386P |
| **(LCD bb)** 2N2222* | NPN-Transistor | tipo NPN (EBC); package TO92 [THT]; part number 2N2222 |
| 18650 Battery | LIPO-1000mAh | variant 1000mAh; package lipo-1000 |
| Beeper | Piezo Speaker |
| For battery | Screw terminal - 2 pins | pin spacing 0.1in (2.54mm); pins 2; hole size 1.0mm,0.508mm; package THT |
| For power resistor | Screw terminal - 2 pins | pin spacing 0.1in (2.54mm); pins 2; hole size 1.0mm,0.508mm; package THT |
| IRF744N | Basic FET N-Channel | tipo n-channel; package TO220 [THT] |
| MF58 | Temperature Sensor (Thermistor) | resistance at 25° 10kΩ; tipo thermistor; package THT; thermistor type NTC |
| MF58 | Temperature Sensor (Thermistor) | resistance at 25° 10kΩ; tipo thermistor; package THT; thermistor type NTC |
| Microcontroller | Arduino Pro Mini clone (compatible Nano) | variant variant 1; tipo Arduino Pro Mini (Clone comp Nano) |
| Power resistor | 10kΩ Resistor | pin spacing 400 mil; bands 5; resistenza 10kΩ; tolerance ±5%; package THT; part number Power resistor |
| Resistance of battery volt. 1/2 | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |
| Resistance of battery volt. 2/2 | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |
| Resistance of power res. volt. 1/2 | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |
| Resistance of power res. volt. 2/2 | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |
| Thermistor battery resistance | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |
| Thermistor power res. resistance | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |

## Shopping List

| Amount | Part Type | Properties |
| --- | --- | --- |
| 1* | LCD screen | pins 16; tipo Character |
| 1* | PCF8574 | tipo PCF8574; package DIP16 [THT] |
| 1* | TRIMPOT | variant variant 2; package trimpot_pth_s3_lock_3386P |
| 1* | NPN-Transistor | tipo NPN (EBC); package TO92 [THT]; part number 2N2222 |
| 1 | LIPO-1000mAh | variant 1000mAh; package lipo-1000 |
| 1 | Piezo Speaker |
| 2 | Screw terminal - 2 pins | pin spacing 0.1in (2.54mm); pins 2; hole size 1.0mm,0.508mm; package THT |
| 1 | Basic FET N-Channel | tipo n-channel; package TO220 [THT] |
| 2 | Temperature Sensor (Thermistor) | resistance at 25° 10kΩ; tipo thermistor; package THT; thermistor type NTC |
| 1 | Arduino Pro Mini clone (compatible Nano) | variant variant 1; tipo Arduino Pro Mini (Clone comp Nano) |
| 1 | 10kΩ Resistor | pin spacing 400 mil; bands 5; resistenza 10kΩ; tolerance ±5%; package THT; part number Power resistor |
| 6 | 10kΩ Resistor | pin spacing 400 mil; bands 4; resistenza 10kΩ; tolerance ±5%; package THT |

*\* Only if you want assembly LCD i2c character controller*

## Realization ##
To measuring voltage we use the principle of Voltage divider, more information on:
[Wikipedia](https://en.wikipedia.org/wiki/Voltage_divider) or [Blog](https://startingelectronics.org/articles/arduino/measuring-voltage-with-arduino/), in simple word this code 
```cpp
batVolt = (sample1 / (1023.0 - ((batResValueGnd / (batResValueVolt + batResValueGnd)) * 1023.0))) * vcc;
```
measure battery voltage,
`batResValueGnd / (batResValueVolt + batResValueGnd)` this is the multipler factor of the reading voltage, I inserted 2 resistance of `batResValueVolt` and `batResValueGnd` ohom after and before the analog read wire;
`sample1 -->` is the average analog readings;
`vcc -->` reference arduino voltage;
`1023.0 -->` is the reference maximun value of analog read (arduino analog read go from 0 to 1023)
	
The principle is that we measure the voltage afther and before the power resistor and so we can calculate milliampere that consumes the battery.

The MOSFET is used to stard and stop battery drain from power resistor.
I'm pretty scary, so I inserted 2 thermistors to monitorage battery and power resistor temperature.

![Breadboard](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/breadboard01.jpg)
![lcd on discharging](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/lcdDischarging02.jpg)

Thermistor on power resistor (heat shrink sleeve to hold).

![Thermistor on power resistance](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/thermistorPowerResistance.jpg)

Thermistor on battery.

![Thermistor on battery](https://github.com/xreef/BatteryCapacityTesterArduinoNANO/blob/master/Resources/thermistorBattery.jpg)

