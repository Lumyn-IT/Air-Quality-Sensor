Air Quality Sensor PMS5003 MQTT mod ![ESP8266](https://img.shields.io/badge/ESP-8266-000000.svg?longCache=true&style=flat&colorA=CCCC33)

 

I got hold of this sensor for monitoring air quality.
After days of testing, I found that it worked well for home use, well beyond expectations.
Thus was born the idea of being able to interface it with home automation.
After careful analysis, I deduce that the sensor is managed by a single integrated circuit that takes the information from the two sensors, makes it "readable" and writes it on the display.
Unfortunately, it seems to me that this integrated circuit does not have the possibility of being interfaced.
Furthermore, there is nothing on board that has to do with wifi.

I describe in detail what I deduced from the analysis of the components and the article in general.

The PMS5003 device performs a constant analysis of the surrounding air with a sampling rate of around 0.1L/min.
The sensor is extremely reactive and the values are updated approximately every 1sec.

The idea is to equip the device with an ESP8266 to be able to manage streaming and publish the sampled variables on MQTT brokers.

The hardware modifications will be extremely simple for anyone with a minimum of familiarity with electronics.
Obviously I decline any responsibility for damage caused by the interventions described in this article.
You will carry out each intervention at your own risk.

The software part will also be extremely simple.

But let's see in detail:

Hardware

Software
![image](https://github.com/user-attachments/assets/939eb407-66e7-4f9d-9fe3-f3c66a0d77e7)
