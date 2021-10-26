#include <Wire.h>

//Address of MCP23016 IO Expander, 8 addresses available
#define IO_L        B00100011
#define IO_R        B00100111


#define L0_I_HP1_SEL   3
#define L0_I_LP1_SEL   4
#define L0_I_BP1_SEL   7

#define L0_O_SS_LED    0
#define L0_O_LFO_LED   1
#define L0_O_BPM_LED   2
#define L0_O_HP1_DC    5
#define L0_O_LP1_DC    6


#define L1_I_ON1_SEL   0
#define L1_I_CUE_SND_1 3
#define L1_I_CUE_SND_2 5

#define L1_O_BP1_DC    1
#define L1_O_ON1_DC    2
#define L1_O_CUE_DC_1  4
#define L1_O_CUE_DC_2  6


#define R0_I_LP2_SEL   4
#define R0_I_ON2_SEL   5
#define R0_I_HP2_SEL   6
#define R0_I_BP2_SEL   7

#define R0_O_LP2_DC    0
#define R0_O_ON2_DC    1
#define R0_O_HP2_DC    2
#define R0_O_BP2_DC    3


#define R1_I_CUE_SND_3 2
#define R1_I_CUE_SND_4 3

#define R1_O_CUE_DC    0
#define R1_O_CUE_DC_3  1 
#define R1_O_CUE_DC_4  4


// MCP23016 command byte to register relationship
#define GP0        0x00  // Data Port, Current status of pins
#define GP1        0x01  // Data Port, Current status of pins
#define OLAT0      0x02  // Output Latch, Current status of latched pins
#define OLAT1      0x03  // Output Latch, Current status of latched pins
#define IPOL0      0x04  // Input polarity, 0-normal, 1-inverted
#define IPOL1      0x05  // Input polarity, 0-normal, 1-inverted
#define IODIR0     0x06  // IO Direction, 0-output, 1-input
#define IODIR1     0x07  // IO Direction, 0-output, 1-input
#define INTCAP0    0x08  // Interrupt capture, Read-Only, value of port that generated the interrupt
#define INTCAP1    0x09  // Interrupt capture, Read-Only, value of port that generated the interrupt
#define IOCON0     0x0A  // IO Expander Control, Sampling frequency ofGP pins, 0-normal, 1-fast
#define IOCON1     0x0B  // IO Expander Control, Sampling frequency ofGP pins, 0-normal, 1-fast

//IO Bank 0 (unused)
#define IO_gp00           B00000001
#define IO_gp01           B00000010
#define IO_gp02           B00000100
#define IO_gp03           B00001000
#define IO_gp04           B00010000
#define IO_pg05           B00100000
#define IO_gp06           B01000000
#define IO_gp07           B10000000

//IO Bank 1 (unused)
#define IO_gp10           B00000001
#define IO_gp11           B00000010
#define IO_gp12           B00000100
#define IO_gp13           B00001000
#define IO_gp14           B00010000
#define IO_pg15           B00100000
#define IO_gp16           B01000000
#define IO_gp17           B10000000


int interruptPin = 7;

byte L0 = B00000111;
byte L1 = B00000000;
byte R0 = B00000000;
byte R1 = B00000000;

int cue[5] = {0, 0, 0, 0, 0};
int filter1[4] = {0, 0, 0, 0};
int filter2[4] = {0, 0, 0, 0};

///////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void) {
  pinMode(interruptPin, INPUT);

  Serial.begin(9600);  // start serial for output

  
  Wire.begin();          // join i2c bus (address optional for master)

  // Special speedup of I2c bus from 100K to 275K
  // Going from about 595Hz (1.68ms) to 980Hz (1.02ms)
  #define CPU_FREQ 16000000L
  #define TWI_FREQ 200000L
  TWBR = ((CPU_FREQ / TWI_FREQ) - 16) / 2; 
  // Wire.setClock(100000);


  // clear state?
  write_io(IO_L, IO_gp00, 0x00, 0x00);
  write_io(IO_L, IO_gp00, 0x67, 0x02);

  // set I/O for both 0 & 1
  write_io(IO_L, IODIR0, 0x98, 0xA9);
  write_io(IO_R, IODIR0, B11110000, B00001100);

  // set input polarity for both 0 & 1
  write_io(IO_L, IPOL0, 0xFF, 0xFF);
  write_io(IO_R, IPOL0, 0xFF, 0xFF);


  // all LED's on
  // write_io(IO_L, IO_gp00, B11111111, B11111000); // same as below
  write_io(IO_L, IO_gp00, B01010110, B01100000);

  write_io(IO_R, IO_gp00, B11111111, B11111111);
  
  delay(500);

  // all LED's off
  write_io(IO_L, IO_gp00, L1, L0);  
  write_io(IO_R, IO_gp00, R1, R0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void) {
  // Poll the interupt pin here using difitalRead and respond to that usign a function
  if (digitalRead(interruptPin) == LOW) { 
    on_interupt();
  }
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
int read_io(int io_address, int cmd_reg) {
  Wire.beginTransmission(io_address);
  delay(1); // 30us
  Wire.write(cmd_reg);
  delay(1);
  Wire.endTransmission();
  delay(1);
  Wire.requestFrom(io_address, 1);
  delay(1);
  return Wire.read();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void write_io(int io_address, int cmd_reg, int push_this, int and_this) {
  Wire.beginTransmission(io_address);
  delay(1); // 30us
  Wire.write(cmd_reg);
  delay(1);
  Wire.write(push_this);
  delay(1);
  Wire.write(and_this);
  delay(1);
  Wire.endTransmission();
  delay(1);

  // delay(50); //70ms
}

void on_interupt () {
  // both the chips share one int pin get the button states
  byte INTCAP0L = read_io(IO_L, INTCAP0);
  delay(1);
  byte INTCAP1L = read_io(IO_L, INTCAP1);
  delay(1);

  byte INTCAP0R = read_io(IO_R, INTCAP0);
  delay(1);
  byte INTCAP1R = read_io(IO_R, INTCAP1);
  delay(1);


  proccess_inputs(INTCAP0L, INTCAP1L, INTCAP0R, INTCAP1R);

  write_outputs();
}


void write_outputs () {
  // off state for left chip 
  byte out_l_0 = B00000111;
  byte out_l_1 = B00000000;
  // off state for right chip
  byte out_r_0 = B00000000;
  byte out_r_1 = B00000000;

  
  bitWrite(out_l_1, L1_O_CUE_DC_1, cue[0]); // CUE_DC_1
  bitWrite(out_l_1, L1_O_CUE_DC_2, cue[1]); // CUE_DC_2
  bitWrite(out_r_1, R1_O_CUE_DC_3, cue[2]); // CUE_DC_3
  bitWrite(out_r_1, R1_O_CUE_DC_4, cue[3]); // CUE_DC_4
  
  bitWrite(out_r_1, R1_O_CUE_DC, cue[4]); // CUE_DC

  // Does not work like upposed to, toggles modes individuall, also mayby set BP or HP by defaulrt
  bitWrite(out_l_0, L0_O_LP1_DC, filter1[1]);
  bitWrite(out_l_1, L1_O_BP1_DC, filter1[2]); // BP1_DC
  bitWrite(out_l_0, L0_O_HP1_DC, filter1[3]);
  bitWrite(out_l_1, L1_O_ON1_DC, filter1[0]); // ON1_DC

  // Does not work like upposed to, toggles modes individuall, also mayby set BP or HP by defaulrt
  bitWrite(out_r_0, R0_O_LP2_DC, filter2[1]);
  bitWrite(out_r_0, R0_O_BP2_DC, filter2[2]); // BP1_DC
  bitWrite(out_r_0, R0_O_HP2_DC, filter2[3]);
  bitWrite(out_r_0, R0_O_ON2_DC, filter2[0]); // ON1_DC
  

  // write the bytes we just composed
  write_io(IO_L, IO_gp00, out_l_1, out_l_0);
  write_io(IO_R, IO_gp00, out_r_1, out_r_0);
}

void read_inputs () {
  // get the button states
  byte GP0L = read_io(IO_L, GP0);
  byte GP1L = read_io(IO_L, GP1);

  byte GP0R = read_io(IO_R, GP0);
  byte GP1R = read_io(IO_R, GP1);

  // pass them to to the proccess function
  proccess_inputs(GP0L, GP1L, GP0R, GP1R);
}


// bit comparison rising
#define BCR(B0, B1, I) (!bitRead(B0, I) && bitRead(B1, I))


void proccess_inputs (byte GP0L, byte GP1L, byte GP0R, byte GP1R) {
  
  // toggle the cue's on and off (no debounce)
  cue[0] = BCR(L1, GP1L, L1_I_CUE_SND_1) ? !cue[0]: cue[0]; // CUE_SND_1
  cue[1] = BCR(L1, GP1L, L1_I_CUE_SND_2) ? !cue[1]: cue[1]; // CUE_SND_2
  cue[2] = BCR(R1, GP1R, R1_I_CUE_SND_3) ? !cue[2]: cue[2]; // CUE_SND_1
  cue[3] = BCR(R1, GP1R, R1_I_CUE_SND_4) ? !cue[3]: cue[3]; // CUE_SND_2

  // if any of the cue on turn cue router on
  cue[4] = cue[0] || cue[1] || cue[2] || cue[3];


  filter1[0] =  BCR(L1, GP1L, L1_I_ON1_SEL) ? !filter1[0]: filter1[0]; // ON1_SEL
  filter1[1] =  BCR(L0, GP0L, L0_I_LP1_SEL) ? !filter1[1]: filter1[1];
  filter1[2] =  BCR(L0, GP0L, L0_I_BP1_SEL) ? !filter1[2]: filter1[2];
  filter1[3] =  BCR(L0, GP0L, L0_I_HP1_SEL) ? !filter1[3]: filter1[3];
  
  filter2[0] =  BCR(R0, GP0R, R0_I_ON2_SEL) ? !filter2[0]: filter2[0]; // ON2_SEL
  filter2[1] =  BCR(R0, GP0R, R0_I_LP2_SEL) ? !filter2[1]: filter2[1];
  filter2[2] =  BCR(R0, GP0R, R0_I_BP2_SEL) ? !filter2[2]: filter2[2];
  filter2[3] =  BCR(R0, GP0R, R0_I_HP2_SEL) ? !filter2[3]: filter2[3];

  // update state
  L0 = GP0L;
  L1 = GP1L;
  R0 = GP0R;
  R1 = GP1R;
}
