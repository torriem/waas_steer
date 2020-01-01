/*
	A simple CAN bridge filter that ensures that old yellow GPS receivers
	always shows up as SF1, even if it's in WAAS mode, so steering can
	be done, albeit at much lower accuracy.  
	
	This version is for the Arduino Due with CAN transceivers attached
	only.
	
	Should be installed between the receiver and the tractor on the CAN
	lines.  Break the CAN wires and have the receiver CAN wires plug into a
	transceiver attached to one of the Due's CAN ports, and the tractor
	CAN wires in the other port. Doesn't matter which.  Should work 
	with older receivers such as the iTC.

	Might work inserting between the post and where the implement
	bus plugs into the brown box monitor.

	This file is released under the GPL v3 or greater.

        Requires the following libraries installed into Arduino IDE:

        https://github.com/collin80/can_common
        https://github.com/collin80/due_can
 */

#include <due_can.h>

static inline void print_hex(uint8_t *data, int len) {
	char temp[4];
	for (int b=0;b < len; b++) {
		sprintf(temp, "%.2x ",data[b]);
		Serial.print(temp);
	}
	Serial.println("");
}

void j1939Decode(long ID, unsigned long* PGN, byte* priority, byte* src_addr, byte *dest_addr)
{
	/* decode j1939 fields from 29-bit CAN id */
	*src_addr = 255;
	*dest_addr = 255;

	*priority = (int)((ID & 0x1C000000) >> 26);

	*PGN = ID & 0x00FFFF00;
	*PGN = *PGN >> 8;

	ID = ID & 0x000000FF;
	*src_addr = (int)ID;

	/* decode dest_addr if message is peer to peer */
	if( (*PGN > 0 && *PGN <= 0xEFFF) ||
	    (*PGN > 0x10000 && *PGN <= 0x1EFFF) ) { 
		*dest_addr = (int)(*PGN & 0xFF);
		*PGN = *PGN & 0x01FF00;
	}
}

/****
 Real work done below here
 ****/

void got_frame(CAN_FRAME *frame, int which) {
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;
	byte signal_type;

	j1939Decode(frame->id, &PGN, &priority, &srcaddr, &destaddr);

	if (srcaddr == 28 && PGN == 65535 && frame->data.bytes[0] == 0x53) {
		uint8_t signal_type;

		//Allow WAAS to steer
		frame->data.bytes[4] = 0x40;
		signal_type = frame->data.bytes[3] >> 4;
		if (signal_type > 0 and signal_type < 4) {
			//Serial.println("Steer with low-quality differential signal.");
			frame->data.bytes[3] = 0x43; //SF1, low accuracy
		}
		//otherwise we'll let it through as is
	}

	if (which == 0) {
		//transmit it out Can1
		//Serial.print(">");
		Can1.sendFrame(*frame);
	} else {
		//Serial.print("<");
		//transmit it out Can0
		Can0.sendFrame(*frame);
	}
}

void can0_got_frame(CAN_FRAME *frame) {
	//Serial.print(">");
	got_frame(frame,0);
}

void can1_got_frame(CAN_FRAME *frame) {
	//Serial.print("<");
	got_frame(frame,1);
}

void setup()
{
	//Serial.begin(115200);
	Can0.begin(CAN_BPS_250K);
	Can1.begin(CAN_BPS_250K);

	for (int filter=0;filter <3; filter ++) {
		Can0.setRXFilter(0,0,true);
		Can1.setRXFilter(0,0,true);
	}

	Can0.attachCANInterrupt(can0_got_frame);
	Can1.attachCANInterrupt(can1_got_frame);
}

void loop()
{
}

