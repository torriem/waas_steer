waas_steer
==========

**If you live outside of North America or Europe, WAAS and EGNOS are not available, and the ITC will *not* get a WAAS3D fix.  waas_steer performance will be unacceptable.**

 waas_steer is a simple sketch for the Arduino Due or the Teensy 4.0 that implements a simple CAN bus filter which allows an older yellow receiver to continue to steer a tractor using the lower-quality WAAS signal after the receiver can no longer work with the higher quality correction signal.

Important Information
---------------------

Please read carefully.  This Arduino sketch is licensed under the terms of the [GPLv3](LICENSE) license. This code is for educational purposes only!  Use at your own risk of permanently damaging sensitive electronics.

Before building any hardware, please ensure that your receiver gets a WAAS GPS fix.  ITC receivers must have the latest [firmware](https://talk.newagtalk.com/forums/thread-view.asp?tid=1008473) installed to reliably get a WAAS or EGNOS fix.  Please note that even with the firmware update these receives will stop getting any GPS signals sometime after 2026 when GPS satellites stop broadcasting on the L1 band.  No firmware or hacks will be able to change this.

Also please note that waas_steer will not work reliably or acceptably if your receiver cannot get a WAAS or EGNOS fix.  In your monitor you can access the information page for your receiver and ensure that you are getting a WAAS or EGNOS fix.  In North America, the monitor will say the fix is "WAAS3D."  This provides for an accuracy of about 12" pass to pass, or more.  **If the monitor says only "3D" that is *not* sufficient GPS accuracy for steering.**

**If you live outside of North America or Europe, WAAS and EGNOS are not available, and the ITC will *not* get a WAAS3D fix.  waas_steer performance will be unacceptable.**  Please re-read the previous paragraph before proceeding.

Where to Connect
----------------

Two CAN transceivers are required to interface the Due or Teensy 4.0 with the CAN bus.  A simple pigtail can be created which plugs into the receiver and into the tractor, and directs CAN H (yellow wire) and CAN L (green wire) from the receiver to one CAN transceiver on the Due, and the tractor's side to the other CAN transceiver on the Due's other CAN port.  Each CAN transceiver should have the termination jumper in place, providing 120 ohms resistance across the CAN high and low wires. 

I recommend this board (also available from [skpang](https://www.skpang.co.uk/collections/teensy/products/teensy-4-0-triple-can-board-include-teensy-4-0)):
https://copperhilltech.com/teensy-4-0-triple-can-bus-board-with-two-can-2-0b-and-one-can-fd-port/

or an Arduino Due with this breakout shield:
https://copperhilltech.com/dual-can-bus-interface-for-arduino-due/

Required software and libraries
-------------------------------
Download and install the latest 1.8 version of the Arduino IDE.

### Teensy 4.x
Download and install [TeensyDuino](https://www.pjrc.com/teensy/teensyduino.html) which comes with all the libraries necessary to build waas_steer.

Set your hardware type to Teensy 4.0 (or 4.1 if that's what you're using) in the Arduino IDE.

### Arduino Due
Using the Arduino IDE, use "Tools -> Board -> Boards manager" and search for "Arduino SAM Boards" and click "install."

Click on "Tools -> Manage Libraries" and search for "due_can."  Scroll down to the library called "due_can" and click "install."

