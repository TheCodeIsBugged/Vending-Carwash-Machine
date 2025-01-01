// C++ code

# include <LiquidCrystal_I2C.h>

# define I2C_ADDR    0x27
# define LCD_COLUMNS 16
# define LCD_LINES   4

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
byte buttonPins[5] = {2, 3, 4, 5, 6};
byte buttonStates[5];
byte prevButtonStates[5] = {0, 0, 0, 0, 0};
unsigned long debounceStart[5] = {0, 0, 0, 0, 0};
const unsigned long debounceDelay = 50;

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

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

void setup()
{
  // Serial
  Serial.begin(115200);

  //Intialize the LCD
  lcd.init();

  //Set the pin of coinButtons
  // Set the pin of water and foam button
  for(int buttonPin: buttonPins)
  {
    pinMode(buttonPin, INPUT);
  }
  
  // Set the pin of relay
  pinMode(relayPin_Motor, OUTPUT);
  pinMode(relayPin_SolenoidValve, OUTPUT);
  
  // Set the default state of the machine
  state = IDLE;
}

void loop()
{
  currentMillis = millis();
  ReadDebounceButton();
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
void ReadDebounceButton(){
  for(int currentButton = 0; currentButton < sizeof(buttonPins); currentButton++)
  {
    int reading = digitalRead(buttonPins[currentButton]);

    if (reading != prevButtonStates[currentButton]) {
      debounceStart[currentButton] = currentMillis;
    }

    if ((currentMillis - debounceStart[currentButton]) > debounceDelay) {
      if (reading != buttonStates[currentButton]) {
        buttonStates[currentButton] = reading;
        if (buttonStates[currentButton] == 1) {
          lcd.clear();
          switch(buttonPins[currentButton])
          {
            case 2:
            Serial.println(F("1-peso Button Pressed"));
            balance += 1;
            break;
            
            case 3:
            Serial.println(F("5-pesos Button Pressed"));
            balance += 5;
            break;

            case 4:
            Serial.println(F("10-pesos Button Pressed"));
            balance += 10;
            break;
            
            case 5:
            Serial.println(F("Water Button Pressed"));
            WaterButtonOutputs();
            break;
            
            case 6:
            Serial.println(F("Foam Button Pressed"));
            break;
            
            default:
            Serial.println(F("No button was read"));
          }
        }
      }
    }
    prevButtonStates[currentButton] = reading;
  }
} 

void WaterButtonOutputs()
{
  switch(state)
  {
    case IDLE:
    DisplayInsufficient();
    // TO-DO: Search for better solution instead of using delay in lcd displa, use millis() instead?
    delay(second);
    break;

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

    case WATER:
    TogglePumpWater();
    state = SELECTION;
    break;

    case FOAM:
    // state = WATER;
    // TogglePumpFoam();
    break;

    default:
    state = IDLE;
  }
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