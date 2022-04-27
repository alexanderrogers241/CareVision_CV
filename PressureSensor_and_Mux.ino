
// include the library code:
#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
// const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

//Mux control pins
int s0 = 8;
int s1 = 9;
int s2 = 10;
int s3 = 11;
int en1 = 12;
int en2 = 13;

//Mux in "SIG" pin
int SIG_pin = 0;

// DATA OUTPUT TO JETSON
byte data[] = {0,0,0,0,
               0,0,0,0,
               0,0,0,0,
               0,0,0,0,
               0,0,0,0,
               0,0,0,0,
               0,0,0,0,
               0,0,0,0};
char c;
  
               
void setup(){
  // Signals for selecting channels on mux 1 and 2
  pinMode(s0, OUTPUT); 
  pinMode(s1, OUTPUT); 
  pinMode(s2, OUTPUT); 
  pinMode(s3, OUTPUT); 
  // Enable 1 for mux 1
  pinMode(en1, OUTPUT);
  // Enable 2 for mux 2
  pinMode(en2, OUTPUT);
  // intialize all signals to low
  digitalWrite(s0, LOW);
  digitalWrite(s1, LOW);
  digitalWrite(s2, LOW);
  digitalWrite(s3, LOW);
  digitalWrite(en1, LOW);
  digitalWrite(en2, LOW);
  // Open Serial Port
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop(){

  do
  {
    c=Serial.read();
  }
  while (c != 'w'); // w = ascii = 0x77 


  // filling the array that is transmitted thru serial to the jetson
  //  for (size_t i = 0; i < 32 ; ++i)
  //  {
  //    //data[i] = (byte)readMux(i);
  //    Serial.write(data[i]);
  //  }

 
    // ENABLE MUX 1
    digitalWrite(en1,LOW);
    // DISABLE MUX 2
    digitalWrite(en2, HIGH);
    // Samples 16 channels from Mux 1
    for(int j = 0; j < 16; j ++)
  {
    // Read and place in array
    float temp = readMux(j);
    data[j] = (byte)temp;
    // print to serial
    //   Serial.print("MUX1_CH");
    //   Serial.print(j);
    //   Serial.print(" is : ");
    //   Serial.print(temp);
    //   Serial.println("(lbs)");
    //   delay(100);

  }

    // DISABLE MUX 1
    digitalWrite(en1, HIGH);
    // ENABLE MUX 2
    digitalWrite(en2, LOW);
    // Samples 16 channels from Mux 2
    for(int j = 0; j < 16; j ++)
  {
    // read and place in array
    float temp = readMux(j);
    int i_offset = j + 16;
    data[i_offset] = (byte)temp;
   //  Rrint to serial
   //  Serial.print("MUX2_CH");
   //  Serial.print(j);
   //  Serial.print(" is : ");
   //  Serial.print(temp);
   //  Serial.println("(lbs)");
   //  lcd.begin(16, 2);
   //  lcd.print("Channel ");
   //  lcd.print(j);
   //  lcd.print(": ");
   //  lcd.print(readMux(j));
   //  delay(100);
   //  lcd.clear();
  }
    for (size_t i = 0; i < 32 ; ++i)
    {
      Serial.write(data[i]);
    }
  //  delay(2000);
}

float readMux(int channel_para)
{
  
   int controlPins[] = {s0, s1, s2, s3};


   // Array of channel bit addresses
   int muxChannel[16][4]={
   {0,0,0,0}, //channel 0
   {1,0,0,0}, //channel 1
   {0,1,0,0}, //channel 2
   {1,1,0,0}, //channel 3
   {0,0,1,0}, //channel 4
   {1,0,1,0}, //channel 5
   {0,1,1,0}, //channel 6
   {1,1,1,0}, //channel 7
   {0,0,0,1}, //channel 8
   {1,0,0,1}, //channel 9
   {0,1,0,1}, //channel 10
   {1,1,0,1}, //channel 11
   {0,0,1,1}, //channel 12
   {1,0,1,1}, //channel 13
   {0,1,1,1}, //channel 14
   {1,1,1,1}  //channel 15
  };

  //loop through the singals
  // S0 = mux[Channel_para][0]
  // S1 = mux[Channel_para][1]
  // S2 = mux[Channel_para][2]
  // ...
  
  for(int i = 0; i < 4; i ++)
  {
    digitalWrite(controlPins[i], muxChannel[channel_para][i]);
  }
  
  // Read the value at the SIG pin
  int val = analogRead(SIG_pin);

  // Return the value
  float voltage = val/50.00;
  return voltage;
}
