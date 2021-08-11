#include <Wire.h>

// MCP23016 I/O Expander
// Can toggle both ports (16 pins) at rates up to 595Hz (1.68ms)
// Arduino analog input 5 - I2C SCL
// Arduino analog input 4 - I2C SDA


//Address of MCP23016 IO Expander, 8 addresses available
#define IO_L        B00100011
#define IO_R        B00100111

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


int interruptPin = 4;

int cue[5] = {0, 0, 0, 0, 0};
int filter1[4] = {0, 0, 0, 0};
int filter2[4] = {0, 0, 0, 0};
///////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void) {

  Serial.begin(9600);  // start serial for output
  // pinMode(interruptPin, INPUT);

  
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

  // set I/O
  write_io(IO_L, IODIR0, 0x98, 0xA9);

  // set input polarity
  write_io(IO_L, IPOL0, 0xFF, 0xFF);


  // all LED's on
  // write_io(IO_L, IO_gp00, B11111111, B11111000); // same as below
  write_io(IO_L, IO_gp00, B01010110, B01100000);
  delay(500);

  // all LED's off
  write_io(IO_L, IO_gp00, B00000000, B00000111);
  
  
  // attachInterrupt(digitalPinToInterrupt(interruptPin), read_input, FALLING);
  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void) {

  // get the button states
  byte GP0L = read_io(IO_L, GP0);
  byte GP1L = read_io(IO_L, GP1);

  byte GP0R = read_io(IO_R, GP0);
  byte GP1R = read_io(IO_R, GP1);

  // toggle the cue's on and off (no debounce)
  cue[0] = bitRead(GP1L, 3) ? !cue[0]: cue[0]; // CUE_SND_1
  cue[1] = bitRead(GP1L, 5) ? !cue[1]: cue[1]; // CUE_SND_2

  filter1[0] =  bitRead(GP1L, 0) ? !filter1[1]: filter1[1]; // ON1_SEL

  // off state for left chip 
  byte out_l_0 = B00000000;
  byte out_l_1 = B00000111;

  
  bitWrite(out_l_0, 4, cue[0]); // CUE_DC_1
  bitWrite(out_l_0, 6, cue[1]); // CUE_DC_2

  // DEMO DOES NOT WORK LIKE IT SHOULD DOES A WIERD TING on purposse
  bitWrite(out_l_0, 1, filter1[0]);
  bitWrite(out_l_0, 2, filter1[0]);

  write_io(IO_L, IO_gp00, out_l_0, out_l_1);
  

}

///////////////////////////////////////////////////////////////////////////////////////////////////////
int read_io(int io_address, int cmd_reg) {
  Wire.beginTransmission(io_address);
  Wire.write(cmd_reg);
  Wire.endTransmission();
  Wire.requestFrom(io_address, 1);
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

  delay(50); //70ms
}
