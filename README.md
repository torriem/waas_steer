waas_steer
==========

 waas_steer  is a simple sketch for the Arduino Due or the Teensy 4.0 that implements a simple CAN bus filter which allows an older yellow receiver to continue to steer a tractor using the lower-quality WAAS signal after the receiver can no longer work with the higher quality correction signal.

Two can transceivers are required to interface the Due or Teensy 4.0 with the CAN bus.  A simple pigtail can be created which plugs into the receiver and into the tractor, and directs CAN H and CAN L from the receiver to one CAN transceiver on the Due, and the tractor's side to the other CAN transceiver on the Due's other CAN port.

It's also possible to place the filter between the implement bus and the back of the brown box monitor.

If using the Teensy Triple CAN breakout board, it can be powered by 12V on the CAN3 connector.

The ability of this old receiver to even get WAAS will end sometime between 2024 and 2026, depending on when the new GPS rollout is complete.

