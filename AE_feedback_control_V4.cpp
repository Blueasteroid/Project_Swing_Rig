/*************************************************************
Fly Swing Rig
JH@KrappLab
2016-09-28
2016-11-24 add dir
2017-04-10 add az/el encoder feedback
*************************************************************/

//#include "PinChangeInt.h"
#include "EnableInterrupt.h"
#include <String.h>

////////////////////////////////////////////////////////////////

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
char motor=0,dir=0,power=0;

char A_idx=0,A_idx_prev=0;
char E_idx=0,E_idx_prev=0;

//int auto_tx_flag=0;
String tx_msg;

//char A_enc_1=0,A_enc_2=0,A_enc_1_prev=0,A_enc_2_prev=0;
//char E_enc_1=0,E_enc_2=0,E_enc_1_prev=0,E_enc_2_prev=0;
int A_enc_count,E_enc_count;

//int A_aim_sign;int A_aim_ang;int E_aim_sign;int E_aim_ang;
byte A_highbyte,A_lowbyte,E_highbyte,E_lowbyte;

////////////////////////////////////////////////////////////////

//void EL_ENCODER_ZERO_ISR(){  E_enc_count=0;  }
//void AZ_ENCODER_ZERO_ISR(){  A_enc_count=0;  }

void EL_ENCODER_ISR()
{
  if (digitalRead(4)==1)
  {
    digitalWrite(A5, HIGH); //DEBUG
    E_enc_count++;
  }
  else
  {
    digitalWrite(A5, LOW);  //DEBUG
    E_enc_count--;
  }
/*
  if (digitalRead(2)==1)
  {
    E_enc_count=0;
  }
  */
}

void AZ_ENCODER_ISR()
{
  if (digitalRead(7)==1)
  {
    //digitalWrite(A5, HIGH);
    A_enc_count++;
  }
  else
  {
    //digitalWrite(A5, LOW);
    A_enc_count--;
  }
/*
  if (digitalRead(10)==1)
  {
    //digitalWrite(A5, HIGH);
    A_enc_count=0;
  }
  */
}

////////////////////////////////////////////////////////////////

void setup() 
{ 
  // initialize digital pin inputs.
  pinMode(2, INPUT);  // Elevation encoder index
  pinMode(4, INPUT);  // Elevation encoder red
  pinMode(5, INPUT);  // Elevation encoder white
  pinMode(6, INPUT);  // Azimuth encoder white
  pinMode(7, INPUT);  // Azimuth encoder red
  pinMode(10, INPUT); // Azimuth encoder index
    
  // Motor Setup Channel A - for azimuth
  pinMode(12, OUTPUT); //Initiates Motor Channel A pin
  pinMode(9, OUTPUT); //Initiates Brake Channel A pin

  // Motor Setup Channel B - for elevation
  pinMode(13, OUTPUT); //Initiates Motor Channel A pin
  pinMode(8, OUTPUT);  //Initiates Brake Channel A pin
  

  // start serial port at 9600 bps and wait for port to open:
  inputString.reserve(200);
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.flush();


  enableInterrupt(5 | PINCHANGEINTERRUPT, EL_ENCODER_ISR, RISING );
  enableInterrupt(6 | PINCHANGEINTERRUPT, AZ_ENCODER_ISR, RISING );
  //enableInterrupt(2 | PINCHANGEINTERRUPT, EL_ENCODER_ZERO_ISR, RISING );
  //enableInterrupt(10 | PINCHANGEINTERRUPT, AZ_ENCODER_ZERO_ISR, RISING );

  //DEBUG LED init
  pinMode(A4, OUTPUT);  // DEBUG LED GND
  pinMode(A5, OUTPUT);  // DEBUG LED
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  for (int i=0;i<4;i++)
  {
    digitalWrite(A5, HIGH);
    delay(100);
    digitalWrite(A5, LOW);
    delay(200);
  }

}// end of setup

////////////////////////////////////////////////////////////////
// LOOP
////////////////////////////////////////////////////////////////

void loop()
{
  if (stringComplete) 
  {
    motor=inputString.charAt(0);
    dir=inputString.charAt(1);
    power=inputString.charAt(2);

    if (motor=='A' || motor=='E')
    {drive(motor,dir ,power);}
    else if (motor=='Q')  //query for angles
    {
      tx_msg = "";
      Serial.println(tx_msg + "A" + A_enc_count + "E" + E_enc_count + '\r');
      /*
      if (dir==1 && power==1)
      {
        auto_tx_flag=1; 
      }
      else
      {        
        auto_tx_flag=0; 
      }
      */
    }
    else if (motor=='O')  //finding origin (0,0)
    {
      find_origin();
    }
    else if (motor=='Z')  //set origin (0,0)
    {
      set_origin( A_highbyte, A_lowbyte, E_highbyte, E_lowbyte);
      //set_origin(dir, power);  // use "dir" and "power" as Az/El angles
    }
    
    else if (motor=='X' )  
    {
      A_highbyte = dir;
      A_lowbyte = power;
    }  
    else if (motor=='Y')  
    {
      E_highbyte = dir;
      E_lowbyte = power;
    } 
    else if (motor=='F' )  
    {
      find_angle( A_highbyte, A_lowbyte, E_highbyte, E_lowbyte );
    }
     
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
/*
  encoder_zeroing();
  encoder_polling();
*/

/*
  if (auto_tx_flag==1)
  {
    tx_msg = "";
    Serial.println(tx_msg + "A" + A_enc_count + "E" + E_enc_count + '\r');
  }
  */
  
  //Serial.flush();
}//end of main loop

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void find_angle(byte A_hb,byte A_lb,byte E_hb,byte E_lb)
{
  //drive('A', 0, 255);
  
  int A_aim;  int E_aim; 
  
  A_aim = ((A_hb & 0x7F)*256 + A_lb);
  if ((A_hb & 0x80) != 0) {A_aim *= -1 ;}
  E_aim = ((E_hb & 0x7F)*256 + E_lb);
  if ((E_hb & 0x80) != 0) {E_aim *= -1 ;}

  
 
  while(1)
  {
    if (A_enc_count>A_aim)
    {
      drive('A', 0, 255);
    }
    else if(A_enc_count<A_aim)
    {
      drive('A', 1, 255);
    }
    else if(A_enc_count==A_aim)
    {
      drive('A', 0, 0);
      //A_enc_count=0;
      //break;    
    }
  //}
  //while(1)
  //{
    if (E_enc_count>E_aim)
    {
      drive('E', 0, 255);
      if (abs(E_aim)<=80)
      {drive('E', 0, 192);}
      if (abs(E_aim)<=40)
      {drive('E', 0, 127);}
    }
    else if(E_enc_count<E_aim)
    {
      drive('E', 1, 255);
      if (abs(E_aim)<=80)
      {drive('E', 1, 192);}
      if (abs(E_aim)<=40)
      {drive('E', 1, 127);}
    }
    else if(E_enc_count==E_aim)
    {
      drive('E', 0, 0);
      //E_enc_count=0;
      //break;
    }
    if (E_enc_count==E_aim && A_enc_count==A_aim)
    {
      drive('A', 0, 0);
      drive('E', 0, 0);
      break;
    }
  }
}

void set_origin(byte A_hb,byte A_lb,byte E_hb,byte E_lb)
{
  int A_aim;  int E_aim; 
  
  A_aim = ((A_hb & 0x7F)*256 + A_lb);
  if ((A_hb & 0x80) != 0) {A_aim *= -1 ;}
  E_aim = ((E_hb & 0x7F)*256 + E_lb);
  if ((E_hb & 0x80) != 0) {E_aim *= -1 ;}



  A_enc_count = A_aim;
  E_enc_count = E_aim;
}


void find_origin()
{
  drive('A', 0, 255);
  while(1)
  {
    if (digitalRead(10)==1)
    {
      drive('A', 0, 0);
      A_enc_count=0;
      break;
    }
  }
  
  drive('E', 1, 255);
  while(1)
  {
    if (digitalRead(2)==1)
    {
      drive('E', 0, 0);
      E_enc_count=0;
      break;
    }
  }
  
}


////////////////////////////////////////////////////////////////


void serialEvent() {
  while (Serial.available()) {

    char inChar = (char)Serial.read();    // get the new byte:
    inputString += inChar;                // add it to the inputString:
    
    if (inChar == '\r') //... '\r' is carriage return
    { 
      stringComplete = true;
    }
    //else
    //{
    //  inputString += inChar;
    //}
  }
}

void drive(char motor, char dir, char power)
{
  if (motor=='A')
  {
    if(dir==0)
    {
      digitalWrite(12, LOW); //Establishes forward direction of Channel A
      digitalWrite(9, LOW);   //Disengage the Brake for Channel A
      analogWrite(3, power);   //Spins the motor on Channel A at full speed
    }
    else
    {
      digitalWrite(12, HIGH); //Establishes forward direction of Channel A
      digitalWrite(9, LOW);   //Disengage the Brake for Channel A
      analogWrite(3, power);   //Spins the motor on Channel A at full speed
    }    
  }
  else if (motor=='E')
  {
    if(dir==1)
    {
      digitalWrite(13, LOW); //Establishes forward direction of Channel A
      digitalWrite(8, LOW);   //Disengage the Brake for Channel A
      analogWrite(11, power);   //Spins the motor on Channel A at full speed
    }
    else
    {
      digitalWrite(13, HIGH); //Establishes forward direction of Channel A
      digitalWrite(8, LOW);   //Disengage the Brake for Channel A
      analogWrite(11, power);   //Spins the motor on Channel A at full speed
    }    
  }
  else
  {
    Serial.println("Wrong motor address!");
  }
}

