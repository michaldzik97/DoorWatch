/*
*Made by: Michal Dzik
*Version: 0.5
*/


#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "FPS_GT511C3.h"
#include "Keypad.h"


/*******************************VARIABLES******************************************/

FPS_GT511C3 fps(12, 13);


//get password from this address
const int eepromPass = 0;
const int eepromIsPass = 50;

//pins to leds
const int ledPins[5] = {A0, A1, A2, A3, A4};
//pins to RGB
/*
*
*/

Servo servo;
int lastAngle;
const int servPin = 11;
//goes to transistor
const int enableServo = 8; //seems like it wont work


//number of rows and columns
const int ROWS = 2;
const int COLUMNS = 4;

//make keypad with these buttons
char keys[ROWS][COLUMNS] = {{'1', '2', '3', '4'},
                            {'5', 'o', 'c', 'r'}};
//row pins
byte rowPins[ROWS] = {5, 6};
//column pins
byte colPins[COLUMNS] = {1, 2, 3, 4};
//create keypad object
Keypad keyPad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLUMNS);

//to check if there is password in memory
 bool checkPass;


/*******************************FUNCTIONS******************************************/

//declaring functions
void SetPass();

void Clear();

void Enroll();

void Options(char c);

void SetLeds(int len, bool state)
{
  for (int i = 0; i < len; i++)
  {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], state);
    delay(100);
  }
}

void ServoController(int angle)
{
  lastAngle = angle;
  digitalWrite(enableServo, true);
  servo.write(angle);
  delay(2000);
  digitalWrite(enableServo, false);
}

//use unsigned integers to not get negative numbers
unsigned int GetPass()
{
  //store the numbers
  String currentPass;
  int i = 0;
  //increment i every time number gets added
  while(i != 5)
  {
    char num = keyPad.getKey();
    if(num)
    {
      //filter out letters
      if(isDigit(num))
      {
        //concantate numbers to currentPass
        currentPass += num;
        //no idea why this works
        SetLeds(i+1, true);
        i++;
      }
      else
      {
        switch(num)
        {
          case 'o':
          ServoController(0);
          break;

          case 'c':
          ServoController(100);
          break;

          case 'r':
          Serial.println("case r");
          //loop until key is pressed
          while(true)
          {
          char opt = keyPad.getKey();
          //get only numbers
          if(isDigit(opt))
          {
            Serial.println(opt);
            //delete everything and get new password and fingerprint
            if (opt == '2')
             {
               Serial.println("(getpass 2)");
               Clear();
               delay(500);
               SetPass();
               for (int i = 0; i < 2; i++)
               {
                 Enroll();
               }
               i = 0;
               currentPass = "";
               break;
             }
             //set new password
             else if(opt == '3')
             {
               Serial.println("(getpass 3)");
               SetPass();
               break;
             }
             //add new finger print
             else if (opt == '4')
             {
               Serial.println("(getpass 4)");
               Enroll();
               break;
             }
           }
          delay(100);
        }
        break;
        }
      }
    }
    delay(100);
    Serial.println(currentPass);
  }
  SetLeds(5, false);
  unsigned int j = currentPass.toInt();
  return j;
}

bool ComparePass()
{
  uint16_t savedPass;
  Serial.println("Skriv in lösenord");
  uint16_t enteredPass = GetPass();
  //get password from memory as "savedPass"
  EEPROM.get(eepromPass, savedPass);
  delay(300);
  Serial.print("Pass is: ");
  Serial.println(savedPass);
  delay(300);
  //compare entered pass to the one stored in memory
  if (savedPass == enteredPass)
  {
    Serial.println("Pass correct");
    return true;
  }
  else
  {
    Serial.println("Pass not correct");
    return false;
  }

}


void SetPass()
{
    //get password by GetPass
    Serial.println("Enter password: ");
    uint16_t _password = GetPass();
    //check password again
    Serial.println("Enter password again: ");
    uint16_t checkIfCorrect = GetPass();

    if (_password == checkIfCorrect)
    {
    delay(100);
    //password is set so don't ask on startup again
    checkPass = true;
    //put entered pass into memory
    EEPROM.put(eepromPass, _password);
    delay(100);
    //put true into IsPass
    EEPROM.put(eepromIsPass, checkPass);
    Serial.println("Pass is set");
    }
}

// Enroll test
void Enroll()
{
  fps.SetLED(true);
	// find open enroll id
	int enrollid = 0;
	bool usedid = true;
	while (usedid == true)
	{
		usedid = fps.CheckEnrolled(enrollid);
		if (usedid==true) enrollid++;
	}
	fps.EnrollStart(enrollid);

	// enroll
	Serial.print("Press finger to Enroll #");
	Serial.println(enrollid);
	while(fps.IsPressFinger() == false) delay(100);
	bool bret = fps.CaptureFinger(true);
	int iret = 0;
	if (bret != false)
	{
		Serial.println("Remove finger");
		fps.Enroll1();
		while(fps.IsPressFinger() == true) delay(100);
		Serial.println("Press same finger again");
		while(fps.IsPressFinger() == false) delay(100);
		bret = fps.CaptureFinger(true);
		if (bret != false)
		{
			Serial.println("Remove finger");
			fps.Enroll2();
			while(fps.IsPressFinger() == true) delay(100);
			Serial.println("Press same finger yet again");
			while(fps.IsPressFinger() == false) delay(100);
			bret = fps.CaptureFinger(true);
			if (bret != false)
			{
				Serial.println("Remove finger");
				iret = fps.Enroll3();
				if (iret == 0)
				{
					Serial.println("Enrolling Successfull");
				}
				else
				{
					Serial.print("Enrolling Failed with error code:");
					Serial.println(iret);
				}
			}
			else Serial.println("Failed to capture third finger");
		}
		else Serial.println("Failed to capture second finger");
	}
	else Serial.println("Failed to capture first finger");
  fps.SetLED(false);
}

//deletes everything from memory
void Clear()
{
  for (size_t i = 0; i < EEPROM.length(); i++)
   {
     EEPROM.put(i, 0);
   }
   fps.DeleteAll();
}


/*******************************SETUP******************************************/
void setup()
{
  Serial.begin(9600);
  digitalWrite(enableServo, false);
  fps.SetLED(false);
  fps.Open();
  servo.attach(servPin);
  EEPROM.get(eepromIsPass, checkPass);
  //if pass is set dont ask for one
  if (!checkPass)
  {
    SetPass();
    //get 2 fingerprints
    for (int i = 0; i < 2; i++)
    {
      Enroll();
    }
  }
  delay(1000);
}

/*******************************LOOP******************************************/

void loop()
{
  if (ComparePass())
  {
    fps.SetLED(true);
    //loop until finger is placed
    while (!fps.IsPressFinger()) continue;

    if (fps.IsPressFinger())
  	{
      //capture finger print with low resolution
  		fps.CaptureFinger(false);
  		int id = fps.Identify1_N();
  		if (id <200)
  		{
  			Serial.print("Verified ID:");
  			Serial.println(id);
        //open door
        ServoController(0);
        fps.SetLED(false);
        //wait 6 seconds and close
        delay(6000);
        ServoController(100);

  		}
  		else
  		{
  			Serial.println("Finger not found");
  		}
  	}
  	else
  	{
  		Serial.println("Please press finger");
  	}
  	delay(100);
    fps.SetLED(false);
  }
  delay(200);
}
