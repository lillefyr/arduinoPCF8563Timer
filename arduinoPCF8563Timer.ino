// Example 54.1 - PCF8563 RTC write/read demonstration

#include "Wire.h"
#define PCF8563address 0x51

const byte interruptPin = 2 ;

byte theDate[] = __DATE__;
byte theTime[] = __TIME__;

byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
String days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool interruptOn = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!interruptOn) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

unsigned long lastRead = 0;
unsigned long lastSecond = 0;
unsigned long interval = 20000;//one second
byte cnt=0;

byte bcdToDec(byte value)
{
  return ((value / 16) * 10 + value % 16);
}

byte decToBcd(byte value){
  return (value / 10 * 16 + value % 10);
}


void readPCF8563()
// this gets the time and date from the PCF8563
{
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  Wire.endTransmission();
  Wire.requestFrom(PCF8563address, 7);
  second     = bcdToDec(Wire.read() & B01111111); // remove VL error bit
  minute     = bcdToDec(Wire.read() & B01111111); // remove unwanted bits from MSB
  hour       = bcdToDec(Wire.read() & B00111111); 
  dayOfMonth = bcdToDec(Wire.read() & B00111111);
  dayOfWeek  = bcdToDec(Wire.read() & B00000111);  
  month      = bcdToDec(Wire.read() & B00011111);  // remove century bit, 1999 is over
  year       = bcdToDec(Wire.read());
}

byte getReg(byte reg){
  Wire.beginTransmission(PCF8563address);
  Wire.write(reg);
  Wire.endTransmission();
                    
  Wire.requestFrom(PCF8563address, 1);
  return Wire.read();  
}

void setDateTimePCF8563()
// this sets the time and date to the PCF8563
{
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  Wire.write(decToBcd(second));  
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));     
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(0)); // Unknown at compile time  
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void setup()
{
  Wire.begin();
  delay(1000); // wait for clock to start
  Serial.begin(115200);
  Serial.println("\narduino RTC8563 realtime clock");

  pinMode ( interruptPin , INPUT_PULLUP) ;
  attachInterrupt ( digitalPinToInterrupt ( interruptPin ), setFlag, INPUT_PULLUP ) ;

  switch (theDate[0]) {
    case 'A':
      if (theDate[1] == 'g') { month = 8; } // Aug
      else { month = 4; } // Apr
      break;
    case 'D':
      month = 12; // Dec
      break;
    case 'F':
      month = 2; // Feb
      break;
    case 'J':
      if (theDate[1] == 'a') { month = 1; } // Jan
      else
      {
        if (theDate[2] =='l') { month = 7; } // Jul
        else { month = 6; } // Jun
      }
      break;
    case 'N':
      month = 11; // Nov
      break;
    case 'O':
      month = 10; // Oct
      break;
    case 'M':
      if (theDate[1] == 'r') { month = 3; } // Mar
      else { month = 5; } // May
      break;
    default:
      month = 9; // Sep
      break;
  }

  dayOfMonth = 0;
  if (theDate[4] != ' ') { dayOfMonth = ( theDate[4] - 0x30 ) * 10; }
  dayOfMonth += ( theDate[5] - 0x30 );

  year = ( theDate[9] - 0x30 ) * 10;
  year += ( theDate[10] - 0x30 );

  hour = 0;
  if (theTime[0] != ' ') { hour = ( theTime[0] - 0x30 ) * 10; }
  hour += ( theTime[1] - 0x30 );
  
  minute = 0;
  if (theTime[3] != ' ') { minute = ( theTime[3] - 0x30 ) * 10; }
  minute += ( theTime[4] - 0x30 );

  second = 0;
  if (theTime[6] != ' ') { second = ( theTime[6] - 0x30 ) * 10; }
  second += ( theTime[7] - 0x30 );

  setDateTimePCF8563();

  interruptDisable();
/*
  for (byte i=0; i<0x10; i++) { Serial.print(getReg(i), BIN); Serial.print(" "); } Serial.println();
*/  initAlarm();
  initTimerPCF8563();

  // we're ready to receive more packets,
  // enable interrupt service routine
  interruptOn = true;
  
  lastRead = millis() + interval;
  lastSecond = millis() + 1000;
}

void print00(){
  byte status = getReg(0x00);
  if (( status & 0b10000000 ) == 0b10000000 ) { Serial.print("EXT_CLK "); } else { Serial.print("normal "); }
  if (( status & 0b00100000 ) == 0b00100000 ) { Serial.print("RTCstop "); } else { Serial.print("RTCrun "); }
  if (( status & 0b00001000 ) == 0b00001000 ) { Serial.print("Override "); } else { Serial.print("normal "); }

}

void print01(){
  byte status = getReg(0x01);
  if (( status & 0b00010000 ) == 0b00010000 ) { Serial.print("TI/TPDisabled "); } else { Serial.print("TI/TPEnabled "); }
  if (( status & 0b00001000 ) == 0b00001000 ) { Serial.print("AlarmFlagSet "); } else { Serial.print("AlarmFlagNotSet "); }
  if (( status & 0b00000100 ) == 0b00000100 ) { Serial.print("TimerFlagSet "); } else { Serial.print("TimerFlagNotSet "); }
  if (( status & 0b00000010 ) == 0b00000010 ) { Serial.print("AlarmEnabled "); } else { Serial.print("AlarmDisabled "); }
  if (( status & 0b00000001 ) == 0b00000001 ) { Serial.print("TimerEnabled "); } else { Serial.print("TimerDisabled "); }
}

void print0d(){
  byte status = getReg(0x0d);
  if (( status & 0b10000000 ) == 0b10000000 ) { Serial.print("CLKOUTDisabled "); } else { Serial.print("CLKOUTEnabled "); }
  if (( status & 0b00000011 ) == 0b00000011 ) { Serial.print("1Hz "); }
  if (( status & 0b00000010 ) == 0b00000010 ) { Serial.print("32Hz "); }
  if (( status & 0b00000001 ) == 0b00000001 ) { Serial.print("1024Hz "); }
  if (( status & 0b00000000 ) == 0b00000000 ) { Serial.print("32.768kHz "); }
}

void print0e(){
  byte status = getReg(0x0e);
  if (( status & 0b10000000 ) == 0b10000000 ) { Serial.print("TimerDisabled "); } else { Serial.print("TimerEnabled "); }
  if (( status & 0b00000011 ) == 0b00000011 ) { Serial.print("1/60Hz "); }
  if (( status & 0b00000010 ) == 0b00000010 ) { Serial.print("1Hz "); }
  if (( status & 0b00000001 ) == 0b00000001 ) { Serial.print("64Hz "); }
  if (( status & 0b00000000 ) == 0b00000000 ) { Serial.print("4096Hz "); }
}

void print0f(){
  byte status = getReg(0x0f);
  Serial.print(status); Serial.print("Sec "); 
}

void interruptDisable() {
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x01); // Control status 0 0 0 
                    // TI/TP=0 (int active)
                    // AF=0 (alarm cleared)
                    // TF=0 (timer cleared)
                    // AIE=1 (alarm INT enabled)
                    // TIE=1 (timer INT enabled)
  Wire.write(0b00000000);
  Wire.endTransmission();
}

void interruptEnable() {
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x01); // Control status 0 0 0 
                    // TI/TP=0 (int active)
                    // AF=0 (alarm cleared)
                    // TF=0 (timer cleared)
                    // AIE=1 (alarm INT enabled)
                    // TIE=1 (timer INT enabled)
  Wire.write(0b00010001);
  Wire.endTransmission();
}

boolean interruptSet() {
  byte status = getReg(0x01);
  if (( status & 0b00010000 ) == 0b00010000) { return true; }
  return false;
}

void initAlarm(){
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x09); // Disabled
  Wire.write(0b100000000);
  Wire.write(0x0A); // Disabled
  Wire.write(0b100000000);
  Wire.write(0x0B); // Disabled
  Wire.write(0b100000000);
  Wire.write(0x0C); // Disabled
  Wire.write(0b100000000);
  Wire.endTransmission();
}

void initTimerPCF8563(){
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x00); // ControlStatus1
                    // TEST1=0 (normal)
                    // 0
                    // STOP=0 (RTC run)
                    // 0
                    // TESTC=0 (normal)
                    // 0 0 0
  Wire.write(0b000000000);
  Wire.endTransmission();

  Wire.beginTransmission(PCF8563address);
  Wire.write(0x0D); // CLKOUT control
                    // FE=0 (clkout disabled)
                    // FD1=1
                    // FD0=1  (1 Hz, work, other values not)
  Wire.write(0b00000011);
  Wire.endTransmission();

  Wire.beginTransmission(PCF8563address);
  Wire.write(0x0E); // Timer Control
                    // TE=1 (timer enabled)
                    // 00000 NA
                    // TD1=0
                    // TD0=0  (en 4096 Hz, works, other values not)
  Wire.write(0b10000010);
  Wire.endTransmission();
}


void setTimerPCF8563(byte cnt){
/*  Wire.beginTransmission(PCF8563address);
  Wire.write(0x00); // ControlStatus1
                    // TEST1=0 (normal)
                    // 0
                    // STOP=0 (RTC run)
                    // 0
                    // TESTC=0 (normal)
                    // 0 0 0
  Wire.write(0b000000000);
  Wire.endTransmission();


  Wire.beginTransmission(PCF8563address);
  Wire.write(0x0D); // CLKOUT control
                    // FE=0 (clkout disabled)
                    // FD1=1
                    // FD0=1  (1 Hz, work, other values not)
  Wire.write(0b00000011);
  Wire.endTransmission();

*/
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x0F); // Timer
                    // Set time (3B = 60 @ 1Hz = 1 minute)
  Wire.write(cnt);
  Wire.endTransmission();
}

void printVal(byte reg) {
  byte status = getReg(reg);
  
  Serial.print(" Reg 0x0");
  Serial.print(reg,HEX);
  Serial.print(" = 0b");
  if ( status < 0b10000000 ) { Serial.print("0"); }
  if ( status < 0b01000000 ) { Serial.print("0"); }
  if ( status < 0b00100000 ) { Serial.print("0"); }
  if ( status < 0b00010000 ) { Serial.print("0"); }
  if ( status < 0b00001000 ) { Serial.print("0"); }
  if ( status < 0b00000100 ) { Serial.print("0"); }
  if ( status < 0b00000010 ) { Serial.print("0"); }
  Serial.print(status, BIN);
 
}

void readAFPCF8563(){
/*  printVal(0x00);
  printVal(0x01);
  printVal(0x0D);
  printVal(0x0E);
  printVal(0x0F);
  Serial.println();
  */
//  print00();
//  print01();
//  print0d();
  print0e();
  print0f();
  Serial.println();
  /*
  byte status = getReg(0x01);

  if (( status & 0b00000101 ) == 0b00000000 ){
    setTimerPCF8563();
  }
  */
}

void loop()
{  
  if ((interruptSet() == false) && (millis() > lastSecond)) //read interrupt count every second
  {
/*    Serial.print(lastSecond);
    Serial.print(" ");
    Serial.print(getReg(0x08));
    Serial.println();
*/
    lastSecond = millis() + 1000;
    
    setTimerPCF8563(17);
    interruptEnable();
  }

  if (millis() - lastRead >= interval) //read interrupt count every second
  {
    Serial.print("timeout at ");
    lastRead = millis();
    receivedFlag = false;
    
    readPCF8563();

//    Serial.print(days[dayOfWeek]); //
    Serial.print(" ");  
    Serial.print(dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(month, DEC);
    Serial.print("/20");
    Serial.print(year, DEC);
    Serial.print(" - ");
    Serial.print(hour, DEC);
    Serial.print(":");
    if (minute < 10)
    {
      Serial.print("0");
    }
    Serial.print(minute, DEC);
    Serial.print(":");  
    if (second < 10)
    {
      Serial.print("0");
    }  
    Serial.print(second, DEC);
    Serial.print(" ");
/*
    print01();
    Serial.print(" ");
    print0f();
    Serial.println(" ");
*/
/*
    print01();
    Serial.print(" ");
    print0f();
    */
    Serial.println();
/*    
    for (byte i=0; i<0x10; i++) { Serial.print(getReg(i), BIN); Serial.print(" "); } Serial.println();
*/
  }
  
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    
    interruptDisable();
    interruptOn = false;

    Serial.print("Interrupt   ");
/*
    print01();
    Serial.print(" ");
    print0f();
    Serial.print(" ");
*/
    Serial.println();
    // reset flag
 
    receivedFlag = false;
    interruptOn = true;
  }
}
