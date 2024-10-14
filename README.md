This Project is to replace the microcontroller board in the stove of the IKEA play kitchen DUKTIG with an energy safing Attiny85. At the result the stove needs only 2 AA batterys (insted of 4 or in old versions 6) and reduced the current in the standby from 1,57mA to 0,2uA. So we neeed les batterys and have a longer battery lifetime.

This is the code for the Attiny85.

Code features:
- debouncing the buttons in software
- auto switch of both hobs after 5 minutes
- power down (deep sleep) the Attiny after all hobs are switched of

Hardware modifications:
- remove 2 of the 3 LEDs from eache hob
- install a 680 ohm resistor on each hob (one of my LEDs need 1,8V and 1,55mA)
- rewire the batterys, so that we use only tow
- rewire the connector to the buttons and to the hobs 