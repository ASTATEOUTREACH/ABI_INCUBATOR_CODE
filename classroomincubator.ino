//Arduino Mega TEC control-Landon Rogers
#include <SPI.h> //SPI comunication for Integrated Circuit (IC)
//SPI on Arduino Mega: MOSI: 51, SCK: 52
#include <Wire.h>
#include <LiquidCrystal_I2C.h> //16x2 LCD

#include <OneWire.h> //DS18B20 Temperature Sensor
#include <DallasTemperature.h>

const byte CS1_pin = 10; //CS pin for IC

float nominalVoltage = 0; //Calculated voltage, based on the 8-bit value
float setTemperature = 30; //Default set temp

int counter = 0; //counts the clicks of the of the rotary encoder, 0-255
int coolingPower = 0; //value of the digital potmeter: 0-255
float tolerance = 0.5; //tolerance for the temperature

//Defining pins
const int rs=7, en=6, d4=5, d5=13, d6=12, d7=11;
const int RotaryCLK = 2; //CLK pin on the rotary encoder
const int RotaryDT = 4; //DT pin on the rotary encoder
const int PushButton = 3; //Button to turn the heating on and off (SW of rotary encoder)
const int sensorPin=8;
//Temperature Startup
OneWire oneWire(sensorPin);
DallasTemperature sensors(&oneWire);
//Statuses for the rotary encoder
int CLKNow;
int CLKPrevious;
int DTNow;
int DTPrevious;
// Time
float TimeNow1;
float TimeNow2; //For the LCD
float TimeNow3; //For the Temp Sensor

bool PSU_Enabled = false; //Default of encoder is not pushed

void setup()
{
  pinMode(CS1_pin, OUTPUT); //Chip select is an output 
  pinMode(2, INPUT_PULLUP); //RotaryDT
  pinMode(4, INPUT_PULLUP); //RotaryCLK
  pinMode(3, INPUT_PULLUP); //RotarySW

  //------------------------------------------------------
  LiquidCrystal_I2C lcd(0x27,16,2); // initialize the lcd
  lcd.backlight();
  lcd.init();
  lcd.init();                    
  lcd.setCursor(0,0); //Defining positon to write from first row,first column .
  lcd.print(" ABI Incubator");
  lcd.setCursor(0,1); //Second row, first column
  lcd.print("  WORMS TEAM "); //You can write 16 Characters per line .
  delay(3000); //wait 3 sec
  //------------------------------------------------------
  //Store states of the rotary encoder
  CLKPrevious = digitalRead(RotaryCLK);
  DTPrevious = digitalRead(RotaryDT);
      
  attachInterrupt(digitalPinToInterrupt(RotaryCLK), rotate, CHANGE); //CLK pin is an interrupt pin
  attachInterrupt(digitalPinToInterrupt(PushButton), pushButton, FALLING); //PushButton pin is an interrupt pin

  TimeNow1 = millis(); //Start time 
  digitalWrite(CS1_pin, HIGH); //Select potmeter
  SPI.begin(); //start SPI for the digital potmeter

  digitalWrite(CS1_pin, LOW);   
  //Default value for the pot should be zero (otherwise, Vout will be 2.5 V by default)  
  SPI.transfer(0x11);  //command 00010001 [00][01][00][11]    
  SPI.transfer(0); //transfer the integer value of the potmeter (0-255 value)     
  delayMicroseconds(100); //wait
  digitalWrite(CS1_pin, HIGH);
  // Start serial communication for debugging
  Serial.begin(9600);
  
  // Start the DallasTemperature library
  sensors.begin();
}

void loop() 
{  
  TimeNow2 = millis();
  
  if(TimeNow2 - TimeNow1 > 200) //update LCD every .2 seconds
  {    
    temperaturesensor(); //run temperature sensor
    printLCD(); //Update LCD
    
    TimeNow1 = millis();
  }

  if(TimeNow2 - TimeNow3 > 2000) //update PSU every 2 s
  {
    writePotmeter(); //write the potmeter value in every loop
    
    if(PSU_Enabled == true) //If the PSU is enabled, we can change the potmeter
    {
      adjustTemperature(); //increase or decrease the value of the coolingPower variable           
      TimeNow3 = millis();
    }
  }

}
void temperaturesensor() //reads temp
{
  sensors.requestTemperatures();
  int Temp_C=sensors.getTempCByIndex(0); //makes temp a number
}
void writePotmeter()
{   
    //CS goes low
    digitalWrite(CS1_pin, LOW);   
        
    SPI.transfer(0x11);    
    SPI.transfer(coolingPower); //transfer the integer value of the potmeter (0-255 value)     
    delayMicroseconds(100); //wait    
    //CS goes high
    digitalWrite(CS1_pin, HIGH);

    nominalVoltage = 5 - (coolingPower * 5.0 / 256.0); //5 V might not be 5.000 V exactly
  
}

void adjustTemperature()
{
int Temp_C=sensors.getTempCByIndex(0);
  if(abs(Temp_C - setTemperature) < tolerance) //Diff is less than 0.5°C
  {
    //If we are within the tolerance, the coolingPower variable is not changed anymore.
  }
  else
  {
    if(Temp_C > setTemperature) //if the measured T > set temperature (goal temp)
    {
      if(coolingPower < 256)
      {
        coolingPower++; //this increases the power to the Peltier - more cooling
      }
      else
      {
        //do nothing after reaching the max. value of the digital potentiometer
      }
    }
    
    else //the measured T < set temperature (overcooling)
    {
      if(coolingPower > 0)
      {
        coolingPower--; //this decreases the power to the Peltier - less cooling  
      }
      else
      {
        //do nothing after reaching the min. value of the digital potentiometer
      }    
    }    
  }
  
}


void rotate()
{
  CLKNow = digitalRead(RotaryCLK); //Read the state of the CLK pin

  // If last and current state of CLK are different, then a pulse occurred  
  if (CLKNow != CLKPrevious  && CLKNow == 1){

    // If the DT state is different than the CLK state then
    // the encoder is rotating A direction: increase
    if (digitalRead(RotaryDT) != CLKNow) {
      if(counter < 256) //we do not go above room temperature (assumed to be 30C)
      {
        counter++; //increase counter by 1      
        setTemperature = 30 - (55 * counter/256.0); 
        //counter = 1 - we subtract 0.21 from 30
        //counter = 255 - we subtract 55 from 30 - higher counter, higher current, lower temperature
      }
      else
      {
        //Don't let it go above 255
      }
      
      } else {
      // Encoder is rotating B direction so decrease
      if(counter < 1) //we do not go below -25 (assumed to be the minimum T on the Peltier)
      {
       // Don't let it go below 0
      }
      else
      {
        counter--; //decrease counter by 1      
        setTemperature = 30 - (55 * counter/256.0);
         //counter = 1 - we subtract 0.21 from 30
         //counter = 255 - we subtract 55 from 30 - higher counter, higher current, lower temperature
      }            
    }    
  }
  CLKPrevious = CLKNow;  // Store last CLK state 
}

void pushButton()
{
  if(PSU_Enabled == false) //if the cooling was OFF
  {
    PSU_Enabled = true;  //turn it ON (writes the potmeter)
  }
  else //if the cooling was ON
  {
    PSU_Enabled = false; //turn it OFF (does not write the potmeter)
    //When you disable cooling, you might want to reset the values too!    
    coolingPower = 0;    
  }  
}

void printLCD()
{ 
    //lcd.clear();
    LiquidCrystal_I2C lcd(0x27,16,2);
    lcd.backlight();
    lcd.setCursor(0,0); // Defining positon to write from second row, first column .
    lcd.print("Temp: ");
    lcd.setCursor(6,0); 
    lcd.print("                            ");
    lcd.setCursor(6,0); 
    lcd.print(sensors.getTempCByIndex(0));    //Display the temperature in °C
    lcd.setCursor(11,0);
    lcd.print("C");
    lcd.setCursor(0,1); 
    lcd.print("Set:  ");
    lcd.setCursor(6,1); 
    lcd.print("                ");
    lcd.setCursor(6,1); 
    lcd.print(setTemperature,1);    
    lcd.setCursor(11,1);
    lcd.print("C");
    //Checking the PSU
    if(PSU_Enabled)
    {
      lcd.setCursor(13,0);
      lcd.print("   ");
      lcd.setCursor(13,0);
      lcd.print("ON");
    }
    else
    {
      lcd.setCursor(13,0);
      lcd.print("   ");
      lcd.setCursor(13,0);
      lcd.print("OFF");
    }
}
    