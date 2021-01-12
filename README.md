waas_steer
==========

 waas_steer  is a simple sketch for the Arduino Due or the Teensy 4.0 that implements a simple CAN bus filter which allows an older yellow receiver to continue to steer a tractor using the lower-quality WAAS signal after the receiver can no longer work with the higher quality correction signal.

Two can transceivers are required to interface the Due or Teensy 4.0 with the CAN bus.  A simple pigtail can be created which plugs into the receiver and into the tractor, and directs CAN H and CAN L from the receiver to one CAN transceiver on the Due, and the tractor's side to the other CAN transceiver on the Due's other CAN port.  Each CAN port should have the termination jumper in place.

I recommend this board:
https://copperhilltech.com/teensy-4-0-triple-can-bus-board-with-two-can-2-0b-and-one-can-fd-port/

or an Arduino Due with this breakout shield:
https://copperhilltech.com/dual-can-bus-interface-for-arduino-due/

It's also possible to place the filter between the implement bus and the back of the brown box monitor, on the implement BUS.  Looking at the plug in the brown box, these twisted pair of yellow and green wires that are the implement bus are the ones that are offset from each other by one pin.

If using the Teensy Triple CAN breakout board, it can be powered by 12V on the CAN3 connector.

The bottom plug into the brown box has the following pinout:
A - tractor CAN low
B - Implement CAN low
C - unused
D - Ground
E - brown wire
F - white wire
G - 12V keyed power
H - Implement CAN high
J - unused
K - tractor CAN high

A pigtail could be made to direct the implement CAN wires into one CAN port on the microcontroller board, and then bring them back from the other CAN port to the brown box.  Keep the CAN pairs twisted, apparently in a left-hand twist.

Note that the ability of this old receiver to even get WAAS will end sometime between 2024 and 2026, depending on when the new GPS rollout is complete.

IMPORTANT: on Teensy you need the latest git version of FlexCAN_T4 from https://github.com/tonton81/FlexCAN_T4.  The version included in the TeensyDuino addon for Arduino IDE is too old. Overwrite the version that is in the hardware/teensy/avr/libraries folder inside the Arduino install folder.  If you don't do this this sketch will not function.
