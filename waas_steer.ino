/*
	A simple CAN bridge filter that ensures that old yellow GPS receivers
	always shows up as SF1, even if it's in WAAS mode, so steering can
	be done, albeit at much lower accuracy.  
	
	This version is for the Arduino Due or Teensy 4.0 only, with CAN
	transceivers attached.
	
	Should be installed between the receiver and the tractor on the CAN
	lines.  Break the CAN wires and have the receiver CAN wires plug into a
	transceiver attached to one of the Due's CAN ports, and the tractor
	CAN wires in the other port. Doesn't matter which.  Should work 
	with older receivers such as the iTC.

	Also works by inserting between the post and where the implement
	bus plugs into the brown box monitor.

	Note that WAAS support for this old receiver will not work after 2024.

	This file is released under the GPL v3 or greater.

        Requires the following libraries installed into Arduino IDE for Due:

        https://github.com/collin80/can_common
        https://github.com/collin80/due_can

	Requires this library for Teensy 4.0:

	https://github.com/tonton81/FlexCAN_T4

 */

#ifdef ARDUINO_TEENSY40
#define TEENSY 1
#endif

#ifdef TEENSY
#  include <FlexCAN_T4.h>
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

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;
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

void got_frame(uint32_t id, uint8_t extended, uint8_t length, BytesUnion *data) {
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;
	uint8_t signal_type;

	j1939Decode(id, &PGN, &priority, &srcaddr, &destaddr);

	if (srcaddr == 28 && PGN == 65535 && data->bytes[0] == 0x53) {

		//Allow WAAS to steer
		data->bytes[4] = 0x40;
		signal_type = data->bytes[3] >> 4;
		if (signal_type > 0 and signal_type < 4) {
			//Serial.println("Steer with low-quality differential signal.");
			data->bytes[3] = 0x43; //SF1, low accuracy
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
	Can1.write(frame);
}

void can1_got_frame(const CAN_message_t &orig_frame) {
	//copy frame so we can modify it
	CAN_message_t frame = orig_frame;
	got_frame(frame.id, frame.flags.extended, frame.len,
	          (BytesUnion *)frame.buf);
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
#ifdef TEENSY
	//Teensy FlexCAN_T4 setup
	Can0.begin();
	Can0.setBaudRate(250000);
	Can0.enableFIFO();
	Can0.enableFIFOInterrupt();
	Can0.onReceive(can0_got_frame);
	Can0.enableMBInterrupts(FIFO);
	Can0.enableMBInterrupts();

	Can1.begin();
	Can1.setBaudRate(250000);
	Can1.enableFIFO();
	Can1.enableFIFOInterrupt();
	Can1.onReceive(can1_got_frame);
	Can1.enableMBInterrupts(FIFO);
	Can1.enableMBInterrupts();
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
#ifdef TEENSY
	//process collected frames
	Can0.events();
#endif
}

