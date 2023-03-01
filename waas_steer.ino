/*
	A simple CAN bridge filter that ensures that old yellow GPS receivers
	always shows up as SF1, even if it's in WAAS mode, so steering can
	be done, albeit at much lower accuracy.  
	
	This version is for the Arduino Due, Teensy 4.x, or Teensy 3.6
	with built-in CAN support.  For Teensy I recommend the nice SKPang
	Teensy 4.0 Triple CAN breakout board:
	http://skpang.co.uk/catalog/teensy-40-triple-can-board-include-teensy-40-p-1575.html
	https://copperhilltech.com/teensy-4-0-triple-can-bus-board-with-two-can-2-0b-and-one-can-fd-port/

	If the Teensy 3.6 works out, I recommend:
	https://copperhilltech.com/teensy-3-6-dual-can-bus-breakout-board/

	For Due boards:
	https://copperhilltech.com/dual-can-bus-interface-for-arduino-due/

	The Teensy 4 breakout board can be powered by 12V on the CAN3 port.
	Teensy 3.6 can be powered by 12V on the CAN0 port.
	
	Should be installed between the tractor and the brown box monitor.
	The Implement CAN bus wires can be cut into two pairs. The pair 
	going to the monitor goes in one CAN, the pair going to the tractor 
	in the other CAN port. On Teensy 4 use CAN1 and CAN2 only, but power
	can go to CAN3.

	Both CAN ports used should have terminator jumpers installed into
	the breakout board.

	This sketch is designed to work only with the Brown Box monitor and
	iTC receivers.

	Note that WAAS support for ITC receivers will not work after 2024 or
	2026, depending on when the US gov't finishes the new GPS rollout.

	This file is released under the GPL v3 or greater.

        Requires the following libraries installed into Arduino IDE for Due:

        https://github.com/collin80/can_common
        https://github.com/collin80/due_can

	Requires this library for Teensy 4.0:

	https://github.com/tonton81/FlexCAN_T4

	IMPORTANT: on Teensy you need the latest git version of FlexCAN_T4
	from https://github.com/tonton81/FlexCAN_T4.  The version included
	in the TeensyDuino addon for Arduino IDE is too old. Overwrite the
	version that is in the hardware/teensy/avr/libraries folder inside
	the Arduino install folder.  If you don't do this this sketch will
	not function.
 */

//Detect if we're compiling for Teensy 4 or 4.1
#ifdef ARDUINO_TEENSY40
#define TEENSY 1
#endif
#ifdef ARDUINO_TEENSY41
#define TEENSY 1
#endif
#ifdef ARDUINO_TEENSY36
#define TEENSY 1
#endif

#ifdef TEENSY
#  include <FlexCAN_T4.h> //Be sure to grab the latest from github
#else //Due
#  include <due_can.h>
#endif 

#ifdef TEENSY
//Union for parsing CAN bus data messages. Warning:
//Invokes type punning, but this works here because
//CAN bus data is always little-endian (or should be)
//and the ARM processor on these boards is also little
//endian.
typedef union {
    uint64_t uint64;
    uint32_t uint32[2]; 
    uint16_t uint16[4];
    uint8_t  uint8[8];
    int64_t int64;
    int32_t int32[2]; 
    int16_t int16[4];
    int8_t  int8[8];

    //deprecated names used by older code
    uint64_t value;
    struct {
        uint32_t low;
        uint32_t high;
    };
    struct {
        uint16_t s0;
        uint16_t s1;
        uint16_t s2;
        uint16_t s3;
    };
    uint8_t bytes[8];
    uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

#ifdef ARDUINO_TEENSY36
FlexCAN_T4<CAN0, RX_SIZE_1024, TX_SIZE_1024> Can0;
FlexCAN_T4<CAN1, RX_SIZE_1024, TX_SIZE_1024> Can1;
#else
FlexCAN_T4<CAN1, RX_SIZE_1024, TX_SIZE_1024> Can0;
FlexCAN_T4<CAN2, RX_SIZE_1024, TX_SIZE_1024> Can1;
#endif


#endif

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

	*PGN = ID & 0x01FFFF00;
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

void got_frame(uint32_t id, uint8_t extended, uint8_t length, BytesUnion *data) {
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;
	uint8_t signal_type;

	j1939Decode(id, &PGN, &priority, &srcaddr, &destaddr);

	if (srcaddr == 28 && PGN == 65535 && data->bytes[0] == 0x53) {

		//most significant nibble (4 bits) indicates the signal
		//type the GPS has
		signal_type = data->bytes[3] >> 4;

		if (signal_type < 4) { //if we don't have SF1, pretend we do
			data->bytes[4] = 0x40; //indicate that the receiver is set to SF1

			//Serial.println("DEBUG: Steer with low-quality signal.");

			//the least significant nibble is the signal strength
			//bar graph indicator
			if (signal_type > 1)
				//WAAS, so pretend SF1, medium accuracy
				data->bytes[3] = 0x46; 
			else
				//3D+ so pretend SF1, low accuracy
				data->bytes[3] = 0x43; 
		}
		//otherwise we'll let it through as is
	}
}

#ifdef TEENSY
void can0_got_frame(const CAN_message_t &orig_frame) {
	//copy frame so we can modify it
	CAN_message_t frame = orig_frame;
	got_frame(frame.id, frame.flags.extended, frame.len,
	          (BytesUnion *)frame.buf);
	frame.seq = 1;
	Can1.write(frame);
}

void can1_got_frame(const CAN_message_t &orig_frame) {
	//copy frame so we can modify it
	CAN_message_t frame = orig_frame;
	got_frame(frame.id, frame.flags.extended, frame.len,
	          (BytesUnion *)frame.buf);
	frame.seq = 1;
	Can0.write(frame);
}

#else
void can0_got_frame(CAN_FRAME *frame) {
	got_frame(frame->id, frame->extended, frame->length, &(frame->data));
	Can1.sendFrame(*frame);
}

void can1_got_frame(CAN_FRAME *frame) {
	got_frame(frame->id, frame->extended, frame->length, &(frame->data));
	Can0.sendFrame(*frame);
}
#endif

void setup()
{
	//Serial.begin(115200);

	//Turn on the built-in LED as a power indicator
	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH);
#ifdef TEENSY
	//Teensy FlexCAN_T4 setup
	Can0.begin();
	Can0.setBaudRate(250000);
#ifdef ARDUINO_TEENSY36
	Can0.setMaxMB(16);
#else
	Can0.setMaxMB(32);
#endif
	Can0.enableFIFO();
	Can0.onReceive(can0_got_frame);

	Can1.begin();
	Can1.setBaudRate(250000);
#ifdef ARDUINO_TEENSY36
	Can1.setMaxMB(16);
#else
	Can1.setMaxMB(32);
#endif
	Can1.enableFIFO();
	Can1.onReceive(can1_got_frame);

	Can0.enableFIFOInterrupt();
	Can1.enableFIFOInterrupt();
#else
	Can0.begin(CAN_BPS_250K);
	Can1.begin(CAN_BPS_250K);

	for (int filter=0;filter <3; filter ++) {
		Can0.setRXFilter(0,0,true);
		Can1.setRXFilter(0,0,true);
	}

	Can0.attachCANInterrupt(can0_got_frame);
	Can1.attachCANInterrupt(can1_got_frame);
#endif
}

void loop()
{
}

