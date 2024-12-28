// C++ code

#include <Adafruit_LiquidCrystal.h>

int balance = 0;
const int cost = 5;

// Pin and Button for coins, replacement for the coin slot
int coinPin_1 = 2;
int coinPin_5 = 3;
int coinPin_10 = 4;

// Pin and Button for water and foam function
int waterPin = 5;
int foamPin = 6;

// Pin for relay, to be connected to solenoid valve and solid state relay?
int relayPin_Motor = 9;
int relayPin_SolenoidValve = 10;


// Event flag for water and foam feature
bool isWatering = false;
bool isFoaming = false;
bool hasWaterTime = false;
bool hasFoamTime = false;

// Variables for toggle
int buttonState;
int prevButtonState = 0;
unsigned long toggleTime = 0;
unsigned long debounce = 200UL;

// Millis() variables
unsigned long currentMillis;
unsigned long startWaterMillis;
unsigned long startFoamMillis;

const unsigned long second = 1000UL;
const unsigned long minute = second * 60;
const unsigned long waterTime = minute * 2;
const unsigned long foamTime = minute;  
unsigned long elapsedWaterTime;
unsigned long elapsedFoamTime;
unsigned long waterTimeLeft = 0;
unsigned long foamTimeLeft = 0;
long displayWaterTime = 0;
long displayFoamTime = 0;

// Enum variable for the machine state
enum MachineState {IDLE, SELECTION, WATER, FOAM};
MachineState state;

Adafruit_LiquidCrystal lcd(0);

void setup()
{
  // Serial
  Serial.begin(115200);
  //Intialize the LCD
  lcd.begin(16, 2);
  
  //Set the pin of coinButtons
  pinMode(coinPin_1, INPUT);
  pinMode(coinPin_5, INPUT);
  pinMode(coinPin_10, INPUT);
  // Set the pin of water and foam button
  pinMode(waterPin, INPUT);
  pinMode(foamPin, INPUT);
  
  // Set the pin of relay
  pinMode(relayPin_Motor, OUTPUT);
  pinMode(relayPin_SolenoidValve, OUTPUT);
  
  // Set the default state of the machine
  state = IDLE;
}

void loop()
{
  currentMillis = millis();
  balance = AddCoinTo(balance);
  ReadWaterButton(waterPin);
  HandleMachineState();
}

// Handle the states and transitions of the machine
void HandleMachineState()
{
  switch(state)
  {
    case IDLE:
    // Transition to SELECTION state if balance is greater than or equal to the cost of water or foam
    if(balance >= cost)
    {
      state = SELECTION;
    }
    else if(balance < cost)
    {
      if(hasWaterTime || hasFoamTime)
      {
        // Transition to SELECTION state if either water or foam has time left
        state = SELECTION;
      }
      else
      {
        // Display insert coins instruction
        // Display the current balance
        DisplayInsert();
        DisplayBalance(balance);
      }
    }
    break;
    
    case SELECTION:
    // Display press a button instruction
    // Display the current balance
    if(!hasWaterTime && !hasFoamTime)
    {
      DisplayPress();
      DisplayBalance(balance);
    }
    else
    {
      // Display the time left of the water and foam if either has any
      // Display the current balance
      DisplayMinSec(0, 0, "WTR", displayWaterTime);
      DisplayMinSec(9, 0, "FM", displayFoamTime);
      DisplayBalance(balance);
    }
    break;
    
    // If condition?
    case WATER:
    PumpWater();
    DisplayBalance(balance);
    break;
    
    case FOAM:
    Serial.println(F("FOAM"));
    break;
  }
}

// Read and Add inserted coins to the balance
int AddCoinTo(int bal)
{
  if(digitalRead(coinPin_1) == 1)
  {
    bal += 1;
    Serial.print(F("BALANCE: P"));
    Serial.println(bal);
  }
  if(digitalRead(coinPin_5) == 1)
  {
    bal += 5;
    Serial.print(F("BALANCE: P"));
    Serial.println(bal);
  }
  if(digitalRead(coinPin_10) == 1)
  {
    bal += 10;
    Serial.print(F("BALANCE: P"));
    Serial.println(bal);
  }
  return bal;
}

// Read and debounce the water button
// TO-DO: Convert this method to be reused with different buttons in the circuit
// As buttons results to different output, converting this to reusable debounce button method is a challenge
// May abstract the switch case?
void ReadWaterButton(int pin)
{
  int reading = digitalRead(pin);
  if(reading != prevButtonState)
  {
    toggleTime = currentMillis;
  }
  
  if(currentMillis - toggleTime > debounce)
  {
    if(reading != buttonState)
    {
      buttonState = reading;
      if(buttonState == 1)
      {
        // lcd.clear causes the togglepumpwater to fail if there's no delay
        // TO-DO: Research on how to solve the lcd.clear without using a delay in the sketch
        lcd.clear();
        delay(50);
        switch(state)
        {
          // Display the insufficient balance instruction, balance < 0
          case IDLE:
          DisplayInsufficient();
          break;
          
          // Initialize the pump water function if there's no water time left
          // Toggle the pump water, on and off, if there's still water time left
          case SELECTION:
          if(!hasWaterTime)
          {
            balance -= cost;
            waterTimeLeft = waterTime;
            startWaterMillis = currentMillis;
            isWatering = true;
            hasWaterTime = true;
            state = WATER;
          }
          else if(hasWaterTime)
          {
            TogglePumpWater();
            state = WATER;
          }
          break;
          
          // Put an if statement? hasWaterTime?
          case WATER:
          TogglePumpWater();
          state = SELECTION;
          break;
          
          // Transition to WATER state. As well as, pause the pump foam
          // TO-DO: Toggle the pump foam function
          case FOAM:
          // state = WATER;
          // TogglePumpFoam();
          break;
          
          default:
          state = IDLE;
        }
        Serial.println(balance);
      }
    }
  }
  prevButtonState = reading;
}

// Method to pump water
void PumpWater()
{
  // long displayWaterTime; Global variable?
  if(isWatering)
  {
    elapsedWaterTime = currentMillis - startWaterMillis;
    if(elapsedWaterTime > waterTimeLeft)
    { 
      // Turn off the relay
      if(digitalRead(relayPin_Motor) == 1)
      {
        digitalWrite(relayPin_Motor, 0);
      }
      // Reset the water event flags
      // Transition to the IDLE state after the countdown
      isWatering = false;
      hasWaterTime = false;
      state == IDLE;
    }
    else if(elapsedWaterTime <= waterTimeLeft)
    {
      // Turn on the relay
      if(digitalRead(relayPin_Motor) == 0)
      {
        digitalWrite(relayPin_Motor, 1);
      }
      displayWaterTime = waterTimeLeft - elapsedWaterTime;
    }
  }
  else if(!isWatering)
  {
    // Turn off the relay
    if(digitalRead(relayPin_Motor) == 1)
    {
      digitalWrite(relayPin_Motor, 0);
    }
    // Update the displayWaterTime based on waterTimeLeft;
    if(hasWaterTime)
    {
      displayWaterTime = waterTimeLeft;
    }
  }
  
  // Display water time left
  // Display the foam time as well?
  if(hasWaterTime)
  {
    DisplayMinSec(0, 0, "WTR", displayWaterTime);
  }
  else if(!hasWaterTime)
  {
    // Reset the waterTimeLeft and displayWaterTime after the countdown
    waterTimeLeft = 0;
    displayWaterTime = 0;
  }
}

// Play/Pause toggle of the pump water and water time countdown:
void TogglePumpWater()
{
  isWatering = !isWatering;
  if(isWatering)
  {
    // Update the start millis of water countdown
    startWaterMillis = currentMillis;
    Serial.println(F("WATER IS RUNNING"));
  }
  else if(!isWatering)
  {
    // Update the water time left
    waterTimeLeft -= elapsedWaterTime;
    Serial.println(F("WATER HAS STOPPED"));
  }
}

// Method to pump foam
void PumpFoam(){}
// Play/Pause toggle of the pump foam and foam time countdown:
void TogglePumpFoam(){}

// Display the milliseconds in minutes:seconds format
void DisplayMinSec(int h, int v, String s, long time)
{
  lcd.setCursor(h, v);
  lcd.print(s);
  int countdown_minute = ((time / 1000) / 60) % 60;
  int countdown_sec = (time / 1000) % 60;
    
  if(countdown_minute < 10)
  {
    lcd.print(' ');
  }
  lcd.print(countdown_minute);
  lcd.print(':');
  if(countdown_sec < 10)
  {
    lcd.print('0');
  }
  lcd.print(countdown_sec);
}

// Display the current balance, parameter?
void DisplayBalance(int bal)
{
  lcd.setCursor(0, 1);
  lcd.print("  BALANCE: P");
  lcd.print(bal);
}

// Display the coin insert instruction
void DisplayInsert()
{
  lcd.home();
  lcd.print("INSERT:P1,P5,P10");
}

// Display the press a button instruction
void DisplayPress()
{
  lcd.home();
  lcd.print(" PRESS A BUTTON ");
}

// Display the insufficient balance instruction
void DisplayInsufficient()
{
  lcd.home();
  lcd.print(" INSUFF BALANCE ");
}