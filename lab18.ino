//V18
//Fixed bug with setting duty cycle and timeout, added load calibration and load inside auto and manual modes.
//Working on changing the interupt intervals to 500ms
//Changed arrow prompts to numerical inside setup and main menu

#include <LiquidCrystal.h>
#include <SPI.h>
#include <Keypad.h> //Custom library at http://playground.arduino.cc/code/keypad

/* initialize the library with the numbers of the interface pins
 * LCD RS pin to digital pin 22
 * LCD Enable pin to digital pin 23
 * LCD D4 pin to digital pin 24
 * LCD D5 pin to digital pin 25
 * LCD D6 pin to digital pin 26
 * LCD D7 pin to digital pin 27
 * LCD R/W pin to ground     */
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
byte Up[8] = {//Create up arrow because there is no native one
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00000,
  0b00000
};
byte Down[8] = {//Create down arrow because there is no native one
  0b00000,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

////////////////Variables///////////////////////////////////////////
int pot; // A0 analog input for the actuator pot
int i;
int timeout = 30;//timeout variable (in seconds)
byte cursor=11;//cursor variable
int dutyCycle=25;//duty cycle variable
byte dutyCycleSelect=0;//Duty cycle selcet bit; If 0 then pause after each stroke if 1 then pause every cycle
long cycleTime = 0;//Duty cycle time in the interrupt(how long it takes for each cycle)
long dutyCyclePause=0;//duty cycle pause how long i need to puase after each cycle or stroke
float dutyCycleTimer=0;//the number showing how long it is paused for

byte setupFlag = 0; //checks if setup mode is used before using auto mode
double actualTimeout = 0; //(in seconds) to compare to timeout variable
double timeoutTimer = 0; //(in milliseconds)

byte limitSwitchFlag=1;//limit Switch Flag: 1=No limits///0=Limit switches///2=pot limits
byte limitSwitchFlagI=1;//limit Switch Flag input: 1=No limits///0=Limit switches///2=pot limits
byte auto_limitSwitchFlag=1;// auto mode limit Switch Flag: ///0=Limit switches///1=pot limits
float ext_clutch=0;// on after extend
float ret_clutch=0;// on after retract
byte load = 0;// manual mode load direction variable 0=Auto load(compression on extend and tension on retract) 1=no load;2=compression;3=tension
byte extLoadDirection = 1;// auto mode extend load direction variable 0=no load;1=compression;2=tension defaults to compression
byte retLoadDirection = 2;// auto mode retract load direction variable 0=no load;1=compression;2=tension defaults to retraction
int ext_load = 0;
int ret_load = 0;

byte load_man =0;//manual mode output variable 1=no load;2=compression;3=tension
byte manualSelect =0;//manual mode menu selector 0=load selection; 1=limit selection
byte setup_submenu = 0; //counter used to keep track of what setup menu is being used

int ext = 46; //pin for extend relay
int ret = 47; //pin for ret relay
int limitE = 48; //pin for extend limit switch
int limitR = 49; //pin for retract limit switch
int limitEstatus = 0; //checks if extend limit switch is pressed
int limitRstatus = 0; //checks if retract limit switch is pressed
int z = 0; //placeholder for array
int x; //used for array in number()

long ext_cycleTime =0;//extend duty cycle time
long ret_cycleTime = 0;//retract duty cycle time
int pot_ret =0;//retract pot limit setting
int pot_ext =10000;//extend pot limit setting
int startCycle = 0;// start cycle counter
int endCycle = 1;// end cycle counter
int totalCycles; 
int cycles=0;//cycle counter
byte dummy;//LOL dummy byte to be used 

int loadcell_value = 0; //initial load cell value read by the arduino
int outputValue = 0; //final load cell value after being mapped 
int loadcell_start = 0;
int loadcell_max = 0;
int testStandMax_Readout = 0;
float testStandMax_Load=0;
int ten = 40;
int com = 41;
int mVolts = 0;
int mcp;
int CS= 53;
float unitPound;
float startWeight;

/*
A0 = pot
A1 = load cell
*/

/////////////For keypad//////////////////////////
const byte rows = 4;
const byte cols = 4;

//Array to represent the key positions, used default numberpad for the keys used
char keys[rows][cols] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'},
};
/* 
 Uses 2 as the up arrow
 4 for the left arrow
 6 for the right arrow
 8 for the down arrow
 5 for select
 * for back
 A for extend
 B for retract
 */

byte rowPins[rows] = {9,8,7,6}; //Pin 9 is top row....
byte colPins[cols] = {5,4,3,2}; //Pin 5 is left column...

Keypad kp = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);
char key; //key being read from keypad
char holdKey; //key that is held being down
///////////////End of Variables/////////////////////////////////////////

///////////////////Setup MCU///////////////////////////////
void setup() {
  Serial.begin(9600);
  kp.setHoldTime(10); //used to simulate holding down a key as being a normal button
  //.................I/O setup..........................................
  pinMode(13, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(ext, OUTPUT);//Ext relay
  pinMode(ret, OUTPUT);//Ret relay
  pinMode(limitE, INPUT_PULLUP);
  pinMode(limitR, INPUT_PULLUP);
  
  pinMode(com, OUTPUT);
  pinMode(ten, OUTPUT);
  pinMode(CS, OUTPUT);
  SPI.begin(); 
  SPI.setClockDivider(SPI_CLOCK_DIV2); 
  //................end of I/O setup....................................* 


  // create a new characters
  lcd.createChar(0, Up);
  lcd.createChar(1, Down);
  // set up the LCD's number of columns and rows: 
  lcd.begin(20, 4);
  // initialize timer1 
  noInterrupts();           // disable all interrupts

    //set timer1 interrupt at 1kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1khz increments

  OCR1A = 31250;            // compare match register 16MHz/8/1kHz ; formula= { 16000000/8*1000)
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);    // 8 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts


    main_menu();
}

/////////////////Timer1 Int routine////////////////////////////
ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  cycleTime = cycleTime + 500 ;   // Add 500ms to the duty cycle counter. Interupt routine is set for 500ms
  timeoutTimer = timeoutTimer + 500;
}




/////////////////////////////////////////////////////////////

void main_menu() { 
  limitSwitchFlag =0;
  load = 0;
  manualSelect = 0; 
  lcd.clear();
  // Print a message to the LCD.
  lcd.print("Lab Test Stand V18");
  lcd.setCursor(0, 1);//start at col0 and row2
  //lcd.print((char)126);//display the right arrow 
  lcd.print("1.Manual Mode");
  lcd.setCursor(0, 2);//start at col0 and row3
  //lcd.print((char)127);//display the left arrow 
  lcd.print("2.Auto Mode");
  lcd.setCursor(0, 3);//start at col0 and row4
  //lcd.write(byte(0));//display the up arrow
  lcd.print("3.Setup Mode");


}

////////////////////////Main Loop//////////////////////////////
void loop() {

  delay(50);
  noTone(44);
  key = kp.getKey(); //wait for key to be pressed
  if (key == '1') {
    lcd.clear();
    lcd.print("    Manual Mode");
    lcd.setCursor(0, 1);//
    lcd.print("*");
    lcd.print((char)126);//display the right arrow 
    lcd.setCursor(4, 1);//
    lcd.print("Auto Load        ");
    lcd.setCursor(0, 2);//
    lcd.print(" ");
    lcd.print((char)127);//display the left arrow 
    lcd.print((char)126);//display the right arrow 
    lcd.setCursor(4, 2);//
    lcd.print("No Switches    ");
    load = 0; 
    limitSwitchFlag =1 ;
    manual();
  }
  if (key == '2') {
    lcd.clear();
    if(setupFlag == 0){
      lcd.print("Setup not completed.");
      lcd.setCursor(0,1);
      lcd.print("    Continue?");
      auto_mode();
      }
    else{
        lcd.print("     Auto Mode     ");
        lcd.setCursor(0, 1);//start at column 0 and row1
        lcd.print("  Select to start  ");
        auto_mode();
      }
  }
  if (key == '3') {
    setup_submenu = 0;
    lcd.clear();
    lcd.print("     Setup Mode");
    lcd.setCursor(0, 1);//start at column 0 and row2
    set_mode();
  }
  if(key == 'C'){
    cal_mode();
  }
}
//////////////////////////////////////////////////////////////


////////////////////////MANUAL MODE////////////////////////////////////////////
void manual() {

  key = kp.getKey();
  if(key)
  {
    holdKey = key;
  }
  

  // manual mode selection: up arrow = load  down arrow =limit switches
  if (key == '2'){
    if (manualSelect ==1){
      manualSelect = manualSelect - 1;
      switch (load) {
      case 0:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        break;
      case 1:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        break;
      case 2:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        break;
      case 3:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print("  ");
        break;

      }

    }
    lcd.setCursor(0, 2);//
    lcd.print(" ");
  }

  if (key == '8'){
    if (manualSelect ==0){
      manualSelect = manualSelect + 1;
      switch (limitSwitchFlag) {
      case 0:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)126);//display the right arrow 
        lcd.print(" ");
        break;
      case 1:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        break;
      case 2:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print(" ");
        break;

      }

    }
    lcd.setCursor(0, 1);//
    lcd.print(" "); 

  }
  //..............................................................................*
  //---------------------------------------------  
  if (key == '6'){
    if (manualSelect ==0) {//If manual select is in load position then change load menu else change the limit switches menu
      switch (load) {//load direction variable 1=no load;0=compression;2=tension
      case 0:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 1);//
        lcd.print("No Load         ");
        load = load + 1;
        break;
      case 1:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Compression      ");
        load = load + 1;
        break;
      case 2:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Tension          ");
        load = load + 1;
        break;
      }
    }
    else{ 
      switch (limitSwitchFlag) {
      case 0:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 2);//
        lcd.print("No Switches     ");
        limitSwitchFlag = limitSwitchFlag + 1;
        break;
      case 1:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print("  ");
        lcd.setCursor(4, 2);//
        lcd.print("Pot Switches    ");
        limitSwitchFlag = limitSwitchFlag + 1;
        break;
      }
    }   

  }
  //-------------------------------------------*

  //--------------------------------------------
  if (key == '4'){
    if (manualSelect ==0) {
      switch (load) {
      case 3:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 1);//
        lcd.print("Compression      ");
        load = load - 1;
        break;
      case 2:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 1);//
        lcd.print("No Load         ");
        load = load - 1;
        break;
      case 1:
        lcd.setCursor(0, 1);//
        lcd.print("*");
        lcd.print((char)126);//display the left arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Auto Load        ");
        load = load - 1;
        break;
      }
    }
    else{ 
      switch (limitSwitchFlag) {
      case 2:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 2);//
        lcd.print("No Switches     ");
        limitSwitchFlag = limitSwitchFlag - 1;
        break;
      case 1:
        lcd.setCursor(0, 2);//
        lcd.print("*");
        lcd.print((char)126);//display the left arrow 
        lcd.print(" ");//clear the right arrow
        lcd.setCursor(4, 2);//
        lcd.print("LS Switches     ");
        limitSwitchFlag = limitSwitchFlag - 1;
        break;
      }
    }   
  } 
  //----------------------------------------------------------------------------------------*  

  //While a button is being held down, checks to see if it is the extend or retract button
  /////////////////////////Extend/Retract Sub routine////////////////////////////////////////////////  
  while(kp.getState() == HOLD) {
    if(holdKey == 'A'){
      switch (load_man) {//apply the coresponding load 0=Auto load(compression on extend and tension on retract) 1=no load;2=compression;3=tension
      case 0:// auto load on extend is compression
        if(ext_load == 0){
          lcd.setCursor(0, 3);//
          lcd.print("ext no Load set     ");
          break;
        }      
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        
        lcd.setCursor(0, 3);//
        lcd.print("ext Compression     ");
        break;
      case 1:// No Load
        lcd.setCursor(0, 3);//
        lcd.print("ext no Load         ");
        break;
      case 2:// compression load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        
        lcd.setCursor(0, 3);//
        lcd.print("ext Compression     ");
        break;
      case 3:// Tension load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
        outputValue = abs(outputValue);  
        digitalWrite(13, HIGH); 
        digitalWrite(ten, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
          outputValue = abs(outputValue);  
        }
        lcd.setCursor(0, 3);//
        lcd.print("ext Tension         ");
        break;
      }
      
      key = kp.getKey();
      pot = analogRead(A0);
      pot = map(pot,0,1023,0,10000);
      limitEstatus = digitalRead(limitE);
      
      switch (limitSwitchFlagI) {//1=No limits///0=Limit switches///2=pot limits
      case 0: // Limit switches
        if(limitEstatus == HIGH){
        digitalWrite(13, HIGH);    // sets the LED on
        digitalWrite(ext, HIGH);    // turn on ext relay
        }
        else{
          digitalWrite(13, LOW);    // sets the LED off
          digitalWrite(ext, LOW);    //turn off ext relay
        }
        break;
      case 1:// //if no limts is selected then just extend 
        digitalWrite(13, HIGH);    // sets the LED on
        digitalWrite(ext, HIGH);    // turn on ext relay
        break;
      case 2:// if pot limits the compare actuator current position with the teach mode selected position(pot_ext)
        if ( pot < pot_ext){
          digitalWrite(13, HIGH);    // sets the LED on
          digitalWrite(ext, HIGH);    // turn on ext relay
        }
        else{
          digitalWrite(13, LOW);    // sets the LED off
          digitalWrite(ext, LOW);    //turn off ext relay
        }
      }
    }
    
    else if (holdKey == 'B') {  //While the retract button is pressed turn on the retract relay
      switch (load_man) {//apply the coresponding load 0=Auto load(compression on extend and tension on retract) 1=no load;2=compression;3=tension
      case 0:// auto load on extend is compression
      if(ret_load == 0){
        lcd.setCursor(0, 3);//
        lcd.print("ret no Load set     ");
        break;
      }
       loadcell_value = analogRead(A1);
       outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight); 
       outputValue = abs(outputValue); 
       digitalWrite(13, HIGH); 
       digitalWrite(ten, HIGH);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight); 
          outputValue = abs(outputValue); 
        }
        lcd.setCursor(0, 3);//
        lcd.print("ret Tension         ");
        break;
      case 1:// No Load
        lcd.setCursor(0, 3);//
        lcd.print("ret no Load         ");
        break;
      case 2:// compression load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load); 
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        lcd.setCursor(0, 3);//
        lcd.print("ret Compression     ");
        break;
      case 3:// Tension load
        loadcell_value = analogRead(A1);
         outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);    
        digitalWrite(13, HIGH); 
        digitalWrite(ten, HIGH);
        outputValue = abs(outputValue);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
          outputValue = abs(outputValue);   
        }
        lcd.setCursor(0, 3);//
        lcd.print("ret Tension         ");
        break;
      }
      key = kp.getKey();
      pot = analogRead(A0);
      pot = map(pot,0,1023,0,10000);
      limitRstatus = digitalRead(limitR);
      
      switch (limitSwitchFlagI) {//1=No limits///0=Limit switches///2=pot limits
      case 0:
        if(limitRstatus == HIGH){
        digitalWrite(13, HIGH);    // sets the LED on
        digitalWrite(ret, HIGH);    // turn on ret relay
        }
        else{
          digitalWrite(13, LOW);    // sets the LED off
          digitalWrite(ret, LOW);    //turn off ext relay
        }
        break;
      case 1:// //if no limts is selected then just retract 
        digitalWrite(13, HIGH);    // sets the LED on
        digitalWrite(ret, HIGH);    // turn on ret relay
        break;
      case 2:
        if ( pot > pot_ret){
          digitalWrite(13, HIGH);    // sets the LED on
          digitalWrite(ret, HIGH);    // turn on ret relay
        }
        else{
          digitalWrite(13, LOW);    // sets the LED off
          digitalWrite(ret, LOW);    //turn off ret relay
        }
      }
    }  
    else
    {
      break;
    }
  }
  //////////////////////////End of extend/retract sub routine////////////////////////////////////////////////////////


  if (key == '5'){//if select button is pressed
    if (limitSwitchFlag==2){// if pot switches is selected then goto teach mode
      teach_mode();
    }
    limitSwitchFlagI = limitSwitchFlag; // load limitSwitchFlagI with limitSwitchFlag
    load_man = load;
  }
  if (key == '*'){
    main_menu(); //if back switch is pressed then go back to the main menu
    return;

  }

  digitalWrite(13, LOW);    // sets the LED off
  digitalWrite(ext, LOW);    //turn off ext relay
  digitalWrite(ret, LOW);    //turn off ret relay
  
  digitalWrite(com, LOW);    
  digitalWrite(ten, LOW);
  mVolts = 0;
  DAC();
  
  lcd.setCursor(0, 3);//
  lcd.print("                    ");
  lcd.setCursor(17, 1);//
  lcd.print(load);
  lcd.print(":");
  lcd.print(load_man);
  lcd.setCursor(17, 2);//
  lcd.print(limitSwitchFlag);
  lcd.print(":");
  lcd.print(limitSwitchFlagI);
  manual();

}
///////////////////////////////////////////////////////////////////////////

////////////////////////Auto MODE/////////////////////////////////////////////////////////////////////////////////////

//.................................................................
void auto_mode() {
  cycleTime = 0;
  timeoutTimer = 0;
  key = kp.getKey();
  
  if (key == '*'){
    main_menu();
    return;
  }
  if (key == '5'){
    run_mode();
  }
  auto_mode();
  
}
//..................................................................



//////////////////////////////Run mode routine/////////////////////////////////////////////////////////
void run_mode(){//start auto run mode
  lcd.clear();
  lcd.print("    Run Mode");
  // full retract 
  limitRstatus = digitalRead(limitR); //Checks the pin of the retract limit switch
  
  if(auto_limitSwitchFlag == 0){
    while (limitRstatus == HIGH) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ret, HIGH);    // turn on ret relay
    limitRstatus = digitalRead(limitR);
    }
  }
  
  if(auto_limitSwitchFlag == 1){  
  while (pot > pot_ret) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ret, HIGH);    // turn on ret relay
    pot = analogRead(A0);
    pot = map(pot,0,1023,0,10000);
    }
  }
  
  digitalWrite(13, LOW);    // sets the LED off
  digitalWrite(ext, LOW);    //turn off ext relay
  digitalWrite(ret, LOW);    //turn off ret relay

run0:
///////////////////////Setting Extend Load///////////////////////////////
  mVolts = 0;
  DAC();
  switch (extLoadDirection) {//apply the corresponding load 0=Auto load(compression on extend and tension on retract) 1=no load;2=compression;3=tension
      case 0:// auto load on extend is compression
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        break;
      case 1:// No Load
        break;
      case 2:// compression load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        break;
      case 3:// Tension load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
        outputValue = abs(outputValue);  
        digitalWrite(13, HIGH); 
        digitalWrite(ten, HIGH);
        while(outputValue < ext_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
          outputValue = abs(outputValue);  
        }
        break;
  }
  //....................Run mode Extend cycle................................
  lcd.setCursor(0, 1);
  lcd.print("    Extending   ");
  cycleTime = 0;
  timeoutTimer = 0;
  limitEstatus = digitalRead(limitE); //Checks the pin of the extend limit switch
  //pot switches
  if(auto_limitSwitchFlag == 1){
  while (pot < pot_ext) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ext, HIGH);    // turn on ext relay
    pot = analogRead(A0);
    pot = map(pot,0,1023,0,10000);



    //TIMEOUT SEQUENCE
    actualTimeout = timeoutTimer/1000.00; //converts milliseconds to seconds
    key = kp.getKey();
    if (key == '*')
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();      
      noTone(44);
      cycles = 0;
      return;
    }
    if (actualTimeout >= timeout) //if timer is greater than timeout value, produce a timeout error
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();
      tone(44,1000);
      lcd.clear();
      lcd.print("ERROR:TIMEOUT");
      lcd.setCursor(0,1);
      lcd.print(cycles);
      lcd.print(" Cycles Completed");
      lcd.setCursor(0,2);
      lcd.print("Press B to return");
      cycles = 0;
      key = kp.getKey();
      if (key == '*')
      {
        return;
      }
      return;
    }


  }
 }
 
 //Limit switches, will continue to run until the Extend limit switch is pulled low
 if(auto_limitSwitchFlag == 0){
  while (limitEstatus == HIGH) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ext, HIGH);    // turn on ext relay



    //TIMEOUT SEQUENCE
    actualTimeout = timeoutTimer/1000.000; //converts milliseconds to seconds
    key = kp.getKey();
    if (key == '*')
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();      
      cycles = 0;
      return;
    }
    if (actualTimeout >= timeout) //if timer is greater than timeout value, produce a timeout error
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();      
      tone(44,1000);
      lcd.clear();
      lcd.print("ERROR:TIMEOUT");
      lcd.setCursor(0,1);
      lcd.print(cycles);
      lcd.print(" Cycles Completed");
      lcd.setCursor(0,2);
      lcd.print("Press B to return");
      cycles = 0;
      key = kp.getKey();
      if (key == '*')
      {
        return;
      }
      return;
    }
    
    limitEstatus = digitalRead(limitE);
  }
 }
 
 
 
  digitalWrite(13, LOW);    // sets the LED off
  digitalWrite(ext, LOW);    //turn off ext relay
  digitalWrite(ret, LOW);    //turn off ret relay
  digitalWrite(com, LOW);    
  digitalWrite(ten, LOW);
  mVolts = 0;
  DAC();
  

  // check on after extend if ext_clutch > 0 then delay that number of seconds
  if(ext_clutch >0)
  {
    delay(ext_clutch*1000);
  }
  ext_cycleTime = cycleTime;
  lcd.setCursor(0, 1);
  lcd.print("                 ");

  if (dutyCycleSelect==0){
    dutyCyclePause = (100/ dutyCycle) * ext_cycleTime;
    lcd.setCursor(0, 2);
    lcd.print("PAUSE:");

    while (dutyCyclePause > 0) {
      key = kp.getKey();
      if (key == '*'){
      cycles = 0;
      return;
      }
      delay(10);
      dutyCyclePause = dutyCyclePause -10;
      dutyCycleTimer = (dutyCyclePause/1000) + 1;
      lcd.setCursor(6, 2);
      lcd.print(dutyCycleTimer);
      lcd.print("  ");
    }
    lcd.setCursor(0, 2);
    lcd.print("                  ");
  }
  


  //.....................End of Run extend cycle................................
///////////////////////Setting Retract Load///////////////////////////////
  mVolts = 0;
  DAC();
  switch (retLoadDirection) {//apply the coresponding load 0=Auto load(compression on extend and tension on retract) 1=no load;2=compression;3=tension
       case 0:// auto load on retract is tension
       loadcell_value = analogRead(A1);
       outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
       outputValue = abs(outputValue);  
       digitalWrite(13, HIGH); 
       digitalWrite(ten, HIGH);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
          outputValue = abs(outputValue);  
        }
        break;
      case 1:// No Load
        break;
      case 2:// compression load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load); 
        digitalWrite(13, HIGH); 
        digitalWrite(com, HIGH);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, loadcell_start, testStandMax_Readout,startWeight, testStandMax_Load);
        }
        break;
      case 3:// Tension load
        loadcell_value = analogRead(A1);
        outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
        outputValue = abs(outputValue);    
        digitalWrite(13, HIGH); 
        digitalWrite(ten, HIGH);
        while(outputValue < ret_load){
          mVolts += 1;
          DAC();
          loadcell_value = analogRead(A1);
          outputValue = map(loadcell_value, testStandMax_Readout,loadcell_start, testStandMax_Load, startWeight);
          outputValue = abs(outputValue);   
        }
        break;
  }
  //......................Run mode retract cycle...............................
  lcd.setCursor(0, 1);
  lcd.print("    Retracting   ");
  cycleTime = 0;
  timeoutTimer = 0;
  limitRstatus = digitalRead(limitR); //Checks the pin of the retract limit switch
  
  //pot switches
  if(auto_limitSwitchFlag==1){
  while (pot > pot_ret) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ret, HIGH);    // turn on ret relay
    pot = analogRead(A0);
    pot = map(pot,0,1023,0,10000);



    //TIMEOUT SEQUENCE
    actualTimeout = timeoutTimer/1000.000; //converts timeout timer from milliseconds to seconds
    key = kp.getKey();
    if (key == '*'){
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();
      cycles = 0;
      return;
    }
    if (actualTimeout >= timeout){
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();
      tone(44,1000);
      lcd.clear();
      lcd.print("ERROR:TIMEOUT");
      lcd.setCursor(0,1);
      lcd.print(cycles);
      lcd.print(" Cycles Done");
      lcd.setCursor(0,2);
      lcd.print("Press B to return");
      cycles = 0;
      key = kp.getKey();
      if (key == '*')
      {
        return;
      }
      return;
    }



    }
  }
  //limit switches, will continue to run until the Retract limit switch is pulled low
  if(auto_limitSwitchFlag == 0){
  while (limitRstatus == HIGH) {
    digitalWrite(13, HIGH);    // sets the LED on
    digitalWrite(ret, HIGH);    // turn on ret relay



    //TIMEOUT SEQUENCE
    actualTimeout = timeoutTimer/1000.000;
    key = kp.getKey();
    if (key == '*')
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret,LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();
      cycles = 0;
      return;
    }
    if (actualTimeout >= timeout)
    {
      digitalWrite(13, LOW);
      digitalWrite(ext,LOW);
      digitalWrite(ret, LOW);
      digitalWrite(com, LOW);    
      digitalWrite(ten, LOW);
      mVolts = 0;
      DAC();      
      tone(44,1000);
      lcd.clear();
      lcd.print("ERROR:TIMEOUT");
      lcd.setCursor(0,1);
      lcd.print(cycles);
      lcd.print(" Cycles Completed");
      lcd.setCursor(0,2);
      lcd.print("Press B to return");
      cycles = 0;
      key = kp.getKey();
      if (key == '*')
      {
        return;
      }
      return;
    }
    
    limitRstatus = digitalRead(limitR);
  }
  }
  
  digitalWrite(13, LOW);    // sets the LED off
  digitalWrite(ext, LOW);    //turn off ext relay
  digitalWrite(ret, LOW);    //turn off ret relay
  digitalWrite(com, LOW);    
  digitalWrite(ten, LOW);
  mVolts = 0;
  DAC();

 // check on after retract if ret_clutch > 0 then delay that number of seconds
  if(ret_clutch > 0)
  {
    delay(ret_clutch*1000);
  }


  ret_cycleTime = cycleTime;
  lcd.setCursor(0, 1);
  lcd.print("                 ");
  cycles=cycles+1;
  totalCycles = endCycle - startCycle;

  //checks if the number of completed cycles = the number of desired cycles
  if(cycles == totalCycles)
  {
    key = kp.getKey();
    lcd.setCursor(0,0);
    lcd.print(cycles);
    lcd.print(" Cycles Completed");
    lcd.setCursor(0,1);
    lcd.print("Press B to return");
    lcd.setCursor(0,2);
    lcd.print("S cycle= ");
    lcd.print(startCycle);
    lcd.setCursor(0,3);
    lcd.print("E cycle= ");
    lcd.print(endCycle);
    cycles = 0;
    key = kp.getKey();
    mVolts = 0;
    DAC();
    digitalWrite(com, LOW);    
    digitalWrite(ten, LOW);
    if (key == '*')
    {
      return;
    }
    return;
  }

  lcd.setCursor(0, 3);
  lcd.print(cycles);
  lcd.print(" cycles");
  if (dutyCycleSelect==0){
    dutyCyclePause = (100.00/ dutyCycle) * ret_cycleTime;
    lcd.setCursor(0, 2);
    lcd.print("PAUSE:");

    while (dutyCyclePause > 0) {
      
      key = kp.getKey();
      if (key == '*'){
      cycles = 0;
      return;
      } 
      
      delay(10);
      dutyCyclePause = dutyCyclePause - 10.00;
      dutyCycleTimer = (dutyCyclePause/1000.00) + 1;
      lcd.setCursor(6, 2);
      lcd.print(dutyCycleTimer);
      lcd.print("   ");
    }
    lcd.setCursor(0, 2);
    lcd.print("                  ");
  }
  else{
    dutyCyclePause = (100.00/ dutyCycle) *(ret_cycleTime + ext_cycleTime);
    lcd.setCursor(0, 2);
    lcd.print("PAUSE:");
    while (dutyCyclePause > 0) {
      
      key = kp.getKey();
      if (key == '*'){
      cycles = 0;
      return;
      }  
      
      delay(10);
      dutyCyclePause = (dutyCyclePause - 10.00);
      dutyCycleTimer = (dutyCyclePause/1000.00) + 1;
      lcd.setCursor(6, 2);
      lcd.print(dutyCycleTimer);
      lcd.print("  ");
    }
    lcd.setCursor(0, 2);
    lcd.print("                  ");
  } 




  //......................End of Run mode retract cycle...............................




  key = kp.getKey();
  if (key == '*'){
    cycles = 0;
    return;
  }

  goto run0;



}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////Calibration MODE/////////////////////////////////////////////////////////////////////////////////////
void cal_mode(){
      
    lcd.clear();
    mVolts = 0;
    DAC();
    lcd.print("Enter start weight");
    startWeight = numberEntry(0);
    while(key != '5'){
      key = kp.getKey();
    }
    loadcell_start = analogRead(A1);
    
    //Put 10V in DAC, put into tension, and read the load
    mVolts = 4095;
    DAC();
    digitalWrite(ten, HIGH);
    digitalWrite(13, HIGH);
    lcd.clear();
    lcd.print("Enter max test load:");
    testStandMax_Load = numberEntry(0);
    while(key != '5'){
      key = kp.getKey();
    }
    testStandMax_Readout = analogRead(A1);
    
    //unitPound = 4096/testStandMax_Load;
    mVolts = 0;
    DAC();
    
    digitalWrite(13, LOW);
    digitalWrite(com, LOW);    
    digitalWrite(ten, LOW); 
    
    main_menu();
    return;
}
////////////////////////Setup MODE/////////////////////////////
void set_mode() {
  switch (setup_submenu){
  case 0:
    ls_menu();//limits menu
    break;
  case 1:
    dutyCycle_menu();//duty cycle menu(0-100%)
    break;
  case 2:
    dutyCycleSelect_menu();// duty cycle type menu pause after each cycle or after each end of the stroke
    break;
  case 3:
    ext_clutch_menu();//on after ext menu
    break;
  case 4: 
    ret_clutch_menu();//on after ret menu
    break;
  case 5:
    extLoad_direction();//Extend load direction menu : no load,compression,tension
    break;
  case 6:
    ext_load_menu(); //set Extention Load
    break;
  case 7:
    retLoad_direction();//Retract load direction menu : no load,compression,tension
    break;
  case 8:
    ret_load_menu(); //Set Retract Load
    break;
  case 9:
    stroke_timeout();//max stroke time allowed sub
    break;
  case 10:
    start_cycle();//goto start cycle counter sub
    break;
  case 11:
    end_cycle();//goto end cycle counter sub
    break; 
  case 12:
    setupFlag = 1; //Setup completed
    main_menu();
    return;
    break;
  }


  set_mode();

}
//////////////////////////////////////////////////////////////
////////////////////////Limit Switch Menus/////////////////////////////
void ls_menu() {
  auto_limitSwitchFlag = 1; //Default is pot limits
  lcd.setCursor(0, 1);
  //lcd.print((char)127);
  lcd.print("1.Pot Limits ");
  lcd.setCursor(0, 2);
  //lcd.print((char)126); 
  lcd.print("2.Limit Switches ");
limitSelect_sub:

  key = kp.getKey();

  if (key == '1') {
    setup_submenu = setup_submenu +1; 
    auto_limitSwitchFlag =1;
    return;
  }

  if (key == '2'){
    setup_submenu = setup_submenu +1; 
    auto_limitSwitchFlag = 0;
    return; 
  } 
  if (key == '*'){
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  } 

  goto limitSelect_sub;  
}  

//////////////////////////////////////////////////////////////


///////////////////////Duty Cycle Sub Menu//////////////////

void dutyCycle_menu(){
  lcd.clear();
  lcd.print("     Setup Mode");
  cursor = 11;
  lcd.setCursor(0, 1);
  lcd.print("Duty Cycle:");
  lcd.setCursor(cursor, 1);
  lcd.print(dutyCycle);
  lcd.print("%       ");  
  lcd.setCursor(0, 2);
  lcd.print("Press # to continue ");
  int dutyCycleArray[3] = {0,0,0}; //Used to hold the timeout timer numbers
  z = 0; //Used to determine how many numbers are being used


dutyCycle_sub:
  lcd.setCursor(cursor, 1);
  lcd.cursor();
  delay(30);
  key = kp.getKey();
  
  if(key != NO_KEY)
  {
    x = key - '0';
    if(x >= 0 && x<=9){
     lcd.print("       ");
     lcd.setCursor(cursor,1);
     switch (z){
       case 0:
         dutyCycleArray[2]=x;
         lcd.print(x);
         lcd.print("%");
         z++;
         break;
       case 1:
         dutyCycleArray[1] = dutyCycleArray[2]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         dutyCycleArray[2] = x;
         lcd.setCursor(cursor, 1);
         for(i = 1; i < 3; i++)
         {
          lcd.print(dutyCycleArray[i]);
         }
         lcd.print("%");
         z++;
         break;
       case 2:
         dutyCycleArray[0] = dutyCycleArray[1]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         dutyCycleArray[1] = dutyCycleArray[2];
         dutyCycleArray[2] = x;
         for(i = 0; i<3; i++)
         {
          lcd.print(dutyCycleArray[i]);
         }
         lcd.print("%");
         z++;
         break;
       case 3:
         dutyCycleArray[0] = dutyCycleArray[1]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         dutyCycleArray[1] = dutyCycleArray[2];
         dutyCycleArray[2] = x;
         for(i = 0; i<3; i++)
         {
          lcd.print(dutyCycleArray[i]);
         }
         lcd.print("%");
         break;
       }
    }
    
    
    if (key == '#'){
      lcd.noCursor();
      dutyCycle = dutyCycleArray[0]* 100 + dutyCycleArray[1] * 10 + dutyCycleArray[2];
      if(dutyCycle == 0){
      dutyCycle = 25;  
      setup_submenu = setup_submenu +1;
        return;   
      }
      else{
      if(dutyCycle > 100){
        lcd.setCursor(0,2);
        lcd.print("  Duty Cycle > 100%");
        lcd.setCursor(0,3);
        lcd.print("  Try again.");
        z=0; 
        for(i = 0; i<3; i++)
        {
          dutyCycleArray[i] = 0; //Reset duty cycle array
        }
        lcd.setCursor(cursor, 1);
        lcd.print("        ");
        lcd.setCursor(cursor, 1);
        lcd.print(" %");
      }
      else {
        setup_submenu = setup_submenu +1;
        return; 
      }
     }
    }
    
    
    if (key == '*'){
      lcd.clear();
      lcd.print("     Setup Mode");
      for(i = 0; i<3; i++)
      {
        dutyCycleArray[i] = 0; //Reset duty cycle array
      }
      dutyCycle = 25; //Changes duty cycle back to default
      z = 0;
      lcd.noCursor();
      setup_submenu = setup_submenu -1;
      set_mode(); 
      return;
    }
    
    
    if(key == 'C'){
      lcd.print("       ");
      lcd.setCursor(cursor,1);
      switch(z){
        case 3:
         dutyCycleArray[2] = dutyCycleArray[1]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         dutyCycleArray[1] = dutyCycleArray[0];
         dutyCycleArray[0] = 0;
         for(i = 1; i < 3; i++)
         {
          lcd.print(dutyCycleArray[i]);
         }
         lcd.print("%");
         z--;
         break;
        case 2:
          dutyCycleArray[2] = dutyCycleArray[1]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
          dutyCycleArray[1] = 0;
          lcd.print(dutyCycleArray[2]);
          lcd.print("%");
          z--;
          break;
        case 1:
          dutyCycleArray[2] = 0; 
          lcd.print("%");
          z--;
          break;
        case 0:
          lcd.print("%");
          break;
      }
    }
  }

  lcd.noCursor();


  goto dutyCycle_sub;  
}
//////////////////////////////////////////////////////////////////////////////////////////


///////////////Duty Cycle Select menu//////////////////////////////////////////////////

void dutyCycleSelect_menu(){
  dutyCycleSelect = 0;
  lcd.setCursor(0, 1);
  lcd.print(" Duty Cycle Mode: ");  
  lcd.setCursor(0, 2);
  //lcd.print((char)127);
  lcd.print("1.After each Cycle ");
  lcd.setCursor(0, 3);
  //lcd.print((char)126); 
  lcd.print("2.After each Stroke ");
dutyCycleSelect_sub:

  key = kp.getKey();

  if (key == '1') {
    setup_submenu = setup_submenu +1; 
    dutyCycleSelect =1;
    return;
  }

  if (key == '2'){
    setup_submenu = setup_submenu +1; 
    dutyCycleSelect =0  ;
    return; 
  } 
  if (key == '*'){
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  } 

  goto dutyCycleSelect_sub;  
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////Extend on timer after limits///////////////////////////////////////////////////
// 0 to 255 seconds
void ext_clutch_menu(){
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("On after ext:");
  lcd.print(ext_clutch, 1);
  lcd.print(" Sec");
ext_sub:

  key = kp.getKey();
  if (key == '2') {
    ext_clutch = ext_clutch + 0.1;
  }

  if (key == '8'){
    if (ext_clutch >= 0.1){
      ext_clutch = ext_clutch - 0.1; 
    }
  }  
  if (key == '5'){
    setup_submenu = setup_submenu +1; 
    return; 
  }
  if (key == '*'){
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }
  lcd.setCursor(13, 1);
  lcd.print(ext_clutch, 1);
  lcd.print(" Sec ");
  goto ext_sub; 

}
///////////////////////////End of Extend timer sub//////////////////////////////////////////////////


/////////////////Retract on timer after limits///////////////////////////////////////////////////
// 0 to 255 seconds
void ret_clutch_menu(){
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("On after ret:");
  lcd.print(ret_clutch, 1);
  lcd.print(" Sec");
ret_sub:
  delay(50);
  key = kp.getKey();
  if (key == '2') {
    ret_clutch = ret_clutch + 0.1;
  }

  if (key == '8'){
    if (ret_clutch >= 0.1){
      ret_clutch = ret_clutch - 0.1; 
    }
  }  
  if (key == '5'){
    setup_submenu = setup_submenu +1; 
    return; 
  }
  if (key == '*'){
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }
  lcd.setCursor(13, 1);
  lcd.print(ret_clutch,1);
  lcd.print(" Sec ");
  goto ret_sub; 

}
///////////////////////////End of Retract timer sub//////////////////////////////////////////////////

//=====================================================================
/////////////////////////Extend Load direction menu////////////////////////////////////////////////////////
void extLoad_direction() {
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Ext Load Direction  ");
  lcd.setCursor(0, 2);//
  lcd.print((char)127);//display the left arrow 
  lcd.print((char)126);//display the right arrow 
  lcd.setCursor(4, 2);//
  lcd.print("Compression     ");
  extLoadDirection = 1;
ext_sub:
  delay(50);
  key = kp.getKey();
  if (key == '6'){

    switch (extLoadDirection) {//load direction variable 0=no load;1=compression;2=tension
    case 0:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print((char)126);//display the right arrow 
      lcd.setCursor(4, 2);//
      lcd.print("Compression     ");
      extLoadDirection = extLoadDirection + 1;
      break;
    case 1:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print(" ");
      lcd.setCursor(4, 2);//
      lcd.print("Tension         ");
      extLoadDirection = extLoadDirection + 1;
      break;
    }
  }

  //-------------------------------------------*

  //--------------------------------------------
  if (key == '4'){

    switch (extLoadDirection) {
    case 2:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print((char)126);//display the right arrow 
      lcd.setCursor(4, 2);//
      lcd.print("Compression     ");
      extLoadDirection = extLoadDirection - 1;
      break;
    case 1:
      lcd.setCursor(0, 2);//
      lcd.print((char)126);//display the left arrow 
      lcd.print(" ");
      lcd.setCursor(4, 2);//
      lcd.print("No Load         ");
      extLoadDirection = extLoadDirection - 1;
      break;
    }
  } 
  //--------------------------------------------*  
  if (key == '5'){
    if(extLoadDirection == 0){
      setup_submenu = setup_submenu +2; //skip setting extend load if there is no load
      return;
    }
    else{
    setup_submenu = setup_submenu +1; 
    return;
    }
  }
  if (key == '*'){
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }
  goto ext_sub;
}
////////////////////////////End of extend load direction menu/////////////////////////////////////////// 

//==============================================================================================================
/////////////////////////////////Extend load menu/////////////////////////////////////////////
void ext_load_menu(){
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Extend Load:");
  lcd.setCursor(0, 2);
  cursor = 12;
  lcd.cursor();
  ext_load = numberEntry(cursor);
extload0:
  key = kp.getKey();
  if (key == '5'){
    setup_submenu = setup_submenu +1; 
    lcd.noCursor();
    return; 
  }
  if (key == '*'){
    lcd.noCursor();
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }

  goto extload0;

}
////////////////////////////////End of Extract Load menu/////////////////////////////////////////////

/////////////////////////retract Load direction menu////////////////////////////////////////////////////////
void retLoad_direction() {
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Ret Load Direction  ");
  lcd.setCursor(0, 2);//
  lcd.print((char)127);//display the left arrow 
  lcd.print(" ");
  lcd.setCursor(4, 2);//
  lcd.print("Tension         ");
  retLoadDirection = 2;
ret_sub:
  delay(50);
  key = kp.getKey();
  if (key == '6'){

    switch (retLoadDirection) {//load direction variable 0=no load;1=compression;2=tension
    case 0:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print((char)126);//display the right arrow 
      lcd.setCursor(4, 2);//
      lcd.print("Compression     ");
      retLoadDirection = retLoadDirection + 1;
      break;
    case 1:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print(" ");
      lcd.setCursor(4, 2);//
      lcd.print("Tension         ");
      retLoadDirection = retLoadDirection + 1;
      break;
    }
  }

  //-------------------------------------------*

  //--------------------------------------------
  if (key == '4'){

    switch (retLoadDirection) {
    case 2:
      lcd.setCursor(0, 2);//
      lcd.print((char)127);//display the left arrow 
      lcd.print((char)126);//display the right arrow 
      lcd.setCursor(4, 2);//
      lcd.print("Compression     ");
      retLoadDirection = retLoadDirection - 1;
      break;
    case 1:
      lcd.setCursor(0, 2);//
      lcd.print((char)126);//display the left arrow 
      lcd.print(" ");
      lcd.setCursor(4, 2);//
      lcd.print("No Load         ");
      retLoadDirection = retLoadDirection - 1;
      break;
    }
  } 
  //--------------------------------------------*  
  if (key == '5'){
    if(retLoadDirection == 0){
      setup_submenu = setup_submenu +2; //skip setting retention load if there is no load
      return;
    }
    else{
    setup_submenu = setup_submenu +1; 
    return;
    }
  }
  if (key == '*'){
    if(extLoadDirection == 0){
      lcd.clear();
      lcd.print("     Setup Mode");
      setup_submenu = setup_submenu -2; //go back to setting extention load direction
      set_mode(); 
      return;
    }
    else{
    lcd.clear();
    lcd.print("     Setup Mode");
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
    }
  }
  goto ret_sub;
}
////////////////////////////End of retract load direction menu/////////////////////////////////////////// 
//==============================================================================================================
/////////////////////////////////Retract load menu/////////////////////////////////////////////
void ret_load_menu(){
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Retract Load:");
  lcd.setCursor(0, 2);
  cursor = 13;
  lcd.cursor();
  ret_load = numberEntry(cursor); 
retload0:
  key = kp.getKey();


  if (key == '5'){
    setup_submenu = setup_submenu +1; 
    lcd.noCursor();
    return; 
  }
  if (key == '*'){
    lcd.noCursor();
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }

  goto retload0;
}
////////////////////////////////End of Retract Load menu/////////////////////////////////////////////

///////////////////////////Maiximum stroke time allowed(timeout)//////////////////////////////////

void stroke_timeout(){

  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Time out:");
  lcd.print(timeout);
  lcd.print(" Sec"); 
  lcd.setCursor(0, 2);
  lcd.print("Press # to continue ");
  int timeoutArray[3] = {0,0,0}; //Used to hold the timeout timer numbers
  z = 0; //Used to determine how many numbers are being used
  cursor = 9;

timeout_sub:
  lcd.setCursor(cursor, 1);
  lcd.cursor();
  delay(30); //To keep cursor blinking
  key = kp.getKey();
  
  if(key != NO_KEY)
  {
    x = key - '0'; //converts key into an int
    if(x >= 0 && x<=9){
     lcd.print("       ");
     lcd.setCursor(cursor,1);
     switch (z){
       case 0:
         timeoutArray[2]=x;
         lcd.print(x);
         lcd.print(" Sec");
         z++;
         break;
       case 1:
         timeoutArray[1] = timeoutArray[2];  //Will push the oldest number to the left, and add newest number on the end like a calculator
         timeoutArray[2] = x;
         lcd.setCursor(cursor, 1);
         for(i = 1; i < 3; i++)
         {
          lcd.print(timeoutArray[i]);
         }
         lcd.print(" Sec");
         z++;
         break;
       case 2:
         timeoutArray[0] = timeoutArray[1]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         timeoutArray[1] = timeoutArray[2];
         timeoutArray[2] = x;
         for(i = 0; i<3; i++)
         {
          lcd.print(timeoutArray[i]);
         }
         lcd.print(" Sec");
         z++;
         break;
       case 3:
         timeoutArray[0] = timeoutArray[1]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         timeoutArray[1] = timeoutArray[2];
         timeoutArray[2] = x;
         for(i = 0; i<3; i++)
         {
          lcd.print(timeoutArray[i]);
         }
         lcd.print(" Sec");
         break;
       }
    }
    
    
    if (key == '#'){
      timeout = timeoutArray[0]* 100 + timeoutArray[1] * 10 + timeoutArray[2];
      if(timeout == 0){
        timeout = 30;
        lcd.noCursor();
        setup_submenu = setup_submenu +1;
        return; 
      }
      else{
      lcd.noCursor();
      setup_submenu = setup_submenu +1;
      return; 
      }
    }
    
    
    if (key == '*'){
      lcd.clear();
      lcd.print("     Setup Mode");
      for(i = 0; i<3; i++)
      {
        timeoutArray[i] = 0; //Reset timeout array
      }
      timeout = 30; //Changes timeout back to default
      z = 0;
      lcd.noCursor();
      if(ret_load == 0){
        setup_submenu = setup_submenu - 2;
        set_mode();
        return;
      }
      else{
      setup_submenu = setup_submenu -1;
      set_mode(); 
      return;
      }
    }
    
    
    if(key == 'C'){
      lcd.print("       ");
      lcd.setCursor(cursor,1);
      switch(z){
        case 3:
         timeoutArray[2] = timeoutArray[1];  //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         timeoutArray[1] = timeoutArray[0];
         timeoutArray[0] = 0;
         for(i = 1; i < 3; i++)
         {
          lcd.print(timeoutArray[i]);
         }
         lcd.print(" Sec");
         z--;
         break;
        case 2:
          timeoutArray[2] = timeoutArray[1]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
          timeoutArray[1] = 0;
          lcd.print(timeoutArray[2]);
          lcd.print(" Sec");
          z--;
          break;
        case 1:
          timeoutArray[2] = 0;
          lcd.print(" Sec");
          z--;
          break;
        case 0:
          lcd.print(" Sec");
          break;
      }
    }
  }

  lcd.noCursor();


  goto timeout_sub;  
}  
/////////////////////////////////Time out end///////////////////////////////////////////////////////////////

/////////////////////////////////Start cycle counter menu///////////////////////////////////////////////////
void start_cycle (){
  cursor = 8;
  dummy =0;
  lcd.clear();
  lcd.print("     Setup Mode");
  lcd.setCursor(0, 1);
  lcd.print("Start Cycle:");
  lcd.setCursor(cursor, 2);
  cursor = 12;
  lcd.cursor();
  startCycle = numberEntry(cursor); //gets # of cycles using numberEntry()
start0:
  key = kp.getKey();


  if (key == '5'){
    setup_submenu = setup_submenu +1; 
    lcd.noCursor();
    return; 
  }
  if (key == '*'){
    lcd.noCursor();
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }

  goto start0;

}
////////////////////////////////End of start cycle counter menu/////////////////////////////////////////////

//==============================================================================================================
/////////////////////////////////End cycle counter menu///////////////////////////////////////////////////
void end_cycle (){
  lcd.clear();
  lcd.print("     Setup Mode");
  cursor = 8;
  lcd.setCursor(0, 1);
  lcd.print("End Cycle:");
  
  cursor = 10;
  lcd.cursor();
  lcd.setCursor(cursor, 1);
  endCycle = numberEntry(cursor); //gets # of cycles using numberEntry()
end0:
  key = kp.getKey();


  if (key == '5'){
     if (startCycle > endCycle) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(" ERROR:");
        lcd.setCursor(0,1);
        lcd.print("Start cycle is");
        lcd.setCursor(0,2);
        lcd.print("larger");
        lcd.setCursor(0,3);
        lcd.print("Press B to return");
        key = kp.getKey();
        if(key == '*'){
          setup_submenu = setup_submenu -1;
          set_mode(); 
          return;
          lcd.clear();
        }
     }
     else if (startCycle == endCycle) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(" ERROR:");
        lcd.setCursor(0,1);
        lcd.print("Cycles are equal");
        lcd.setCursor(0,2);
        lcd.print("Press B to return");
        key = kp.getKey();
        if(key == '*'){
          setup_submenu = setup_submenu -1;
          set_mode(); 
          return;
          lcd.clear();
        }
      }
    else{
    setup_submenu = setup_submenu +1; 
    lcd.noCursor();
    return;
    }
  }

  if (key == '*'){
    setup_submenu = setup_submenu -1;
    set_mode(); 
    return;
  }

  goto end0;
}
////////////////////////////////End of End cycle counter menu/////////////////////////////////////////////
//==============================================================================================================
////////////////////////////////////////Teach Menu////////////////////////////////////////
void teach_mode() {
  limitSwitchFlagI = limitSwitchFlag; // load limitSwitchFlagI with limitSwitchFlag
  load_man = load;  
  lcd.clear();
  lcd.print("    Teach Mode ");
  lcd.setCursor(0, 1);
  lcd.print("Max Ext ");
  lcd.write(byte(0));
  lcd.print(":");
  lcd.print("     ");
  lcd.setCursor(10, 1);
  lcd.print(pot_ext);
  lcd.print(char(244));
  lcd.setCursor(0, 2);
  lcd.print("A: ");
  lcd.setCursor(0, 3);
  lcd.print("Max Ret ");
  lcd.write(byte(1));
  lcd.print(":");
  lcd.print(pot_ret);
  lcd.print(char(244));

teach0:
  key = kp.getKey();
  pot = analogRead(A0);
  pot = map(pot,0,1023,0,10000);
  lcd.setCursor(3, 2);
  lcd.print(pot);
  lcd.print(char(244));
  lcd.print("      ");

  //        if select button is pressed/////////////////////////
  if (key == '5'){
    lcd.clear();
    lcd.print("    Manual Mode");
    if (manualSelect ==0){//if in load then reset the lcd line 1
      lcd.setCursor(0, 2);//
      lcd.print(" ");
      lcd.setCursor(0, 1);//
      lcd.print("*");
      switch (load_man) {
      case 0:
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 1);//
        lcd.print("Auto load       ");
        break;
      case 1:
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("No Load         ");
        break;
      case 2:

        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Compression      ");
        break;
      case 3:
        lcd.print((char)127);//display the left arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Tension              ");
        break;
      }
      lcd.setCursor(0, 2);//
      lcd.print(" ");
      lcd.print((char)127);//display the left arrow 
      lcd.print("  ");
      lcd.setCursor(4, 2);//
      lcd.print("Pot Switches    ");
    }

    else {

      lcd.setCursor(0, 2);//
      lcd.print("*");
      lcd.print((char)127);//display the left arrow 
      lcd.print("  ");
      lcd.setCursor(4, 2);//
      lcd.print("Pot Switches    ");
      lcd.setCursor(0, 1);//
      lcd.print(" ");
      switch (load_man) {

      case 0:
        lcd.print((char)126);//display the right arrow 
        lcd.setCursor(4, 1);//
        lcd.print("Auto load       ");
        break;
      case 1:
        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("No Load         ");
        break;
      case 2:

        lcd.print((char)127);//display the left arrow 
        lcd.print((char)126);//display the right arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Compression      ");
        break;
      case 3:
        lcd.print((char)127);//display the left arrow 
        lcd.print("  ");
        lcd.setCursor(4, 1);//
        lcd.print("Tension              ");
        break;
      }
    }


    return; 
  }
  ////////////////////////////End of select////////////////////////////
  if(key){
    holdKey = key;
  }


  while (kp.getState() == HOLD) {
    if(holdKey== 'A'){ //While the extend button is pressed turn on the extend relay
      digitalWrite(13, HIGH);    // sets the LED on
      digitalWrite(ext, HIGH);    // turn on ext relay
      key = kp.getKey();
    }
    else if(holdKey == 'B') {  //While the extend button is pressed turn on the retract relay
      digitalWrite(13, HIGH);    // sets the LED on
      digitalWrite(ret, HIGH);    // turn on ret relay
      key = kp.getKey();
    }
    else{
      break;
    }
  }
  
  if (key == '2') {
    pot_ext  = pot;
    lcd.setCursor(10, 1);
    lcd.print("         ");
    lcd.setCursor(10, 1);
    lcd.print(pot_ext);
    lcd.print(char(244));
  }
  if (key == '8') {
    pot_ret  = pot;
    lcd.setCursor(10, 3);
    lcd.print("         ");
    lcd.setCursor(10, 3);
    lcd.print(pot_ret);
    lcd.print(char(244));
  }
  digitalWrite(13, LOW);    // sets the LED on
  digitalWrite(ret, LOW);    // turn on ret relay
  digitalWrite(ext, LOW);    // turn on ext relay
  goto teach0;
}

//////////////////////END OF TEACH MODE///////////////////////////////////////////////////////



///////Method used to enter a number with up to 7 digits with the keypad, input is starting cursor location on the 2nd row
float numberEntry(int numbercsr)
{
  long entry;
  lcd.setCursor(0,2);
  lcd.print("NUMBER ENTRY");
  lcd.setCursor(0,3);
  lcd.print("PRESS # TO CONFIRM");
  int counter[7] = {0,0,0,0,0,0,0}; //Array to hold numbers entered
  z = 0; //Used to determine how many numbers are currently being used


entryloop:
  lcd.setCursor(numbercsr, 1);
  lcd.cursor();
  delay(30); //To keep cursor blinking
  key = kp.getKey();
  
  // checks if key is pressed
  if(key != NO_KEY){
    x = key - '0'; //converts key pressed into an int
    if(x >= 0 && x<=9){
     lcd.setCursor(numbercsr,1);
     switch (z){
       case 0:
         counter[6]=x;
         lcd.print(x);
         z++; 
         break;
       case 1:
         counter[5] = counter[6]; //Will push the oldest number to the left, and add newest number on the end like a calculator
         counter[6] = x;
         for(i = 5; i < 7; i++)
         {
          lcd.print(counter[i]);
         }
         z++;
         break;
       case 2:
         counter[4] = counter[5];  //Will push the oldest number to the left, and add newest number on the end
         counter[5] = counter[6];
         counter[6] = x;
         for(i = 4; i<7; i++)
         {
          lcd.print(counter[i]);
         }
         z++;
         break;
       case 3:
        counter[3] = counter[4]; //Will push the oldest number to the left, and add newest number on the end
        counter[4] = counter[5];
        counter[5] = counter[6];
        counter[6] = x;
         for(i = 3; i<7; i++)
         {
          lcd.print(counter[i]);
         }
         z++;
         break;
      case 4:
        counter[2] = counter[3]; //Will push the oldest number to the left, and add newest number on the end
        counter[3] = counter[4];
        counter[4] = counter[5];
        counter[5] = counter[6];
        counter[6] = x;
         for(i = 2; i<7; i++){
          lcd.print(counter[i]);
         }
         z++;
         break;
       case 5:
       counter[1] = counter[2]; //Will push the oldest number to the left, and add newest number on the end
       counter[2] = counter[3];
       counter[3] = counter[4];
       counter[4] = counter[5];
       counter[5] = counter[6];
       counter[6] = x;
         for(i = 1; i<7; i++)
         {
          lcd.print(counter[i]);
         }
         z++;
         break;
      case 6:
       counter[0] = counter[1]; //Will push the oldest number to the left, and add newest number on the end
       counter[1] = counter[2];
       counter[2] = counter[3];
       counter[3] = counter[4];
       counter[4] = counter[5];
       counter[5] = counter[6];
       counter[6] = x;
         for(i = 0; i<7; i++)
         {
          lcd.print(counter[i]);
         }
         z++;
         break;
     case 7:
       counter[0] = counter[1]; //Will push the oldest number to the left, and add newest number on the end
       counter[1] = counter[2];
       counter[2] = counter[3];
       counter[3] = counter[4];
       counter[4] = counter[5];
       counter[5] = counter[6];
       counter[6] = x;
         for(i = 0; i<7; i++)
         {
          lcd.print(counter[i]);
         }
         break;
       }
      }
    }
    
    //Confirm the number
    if (key == '#'){
      lcd.noCursor();
      entry = (counter[0]*1000000) + (counter[1]*100000) + (counter[2]*10000) + (counter[3]*1000) + (counter[4]*100) + (counter[5]*10) + (counter[6]);
      
      lcd.setCursor(0,2);
      lcd.print("            "); //Clear the line to avoid overlapping letters
     
      lcd.setCursor(0,2);
      lcd.print("NUMBER ACCEPTED");
      
      lcd.setCursor(0,3);
      lcd.print(" "); //Clear the line to avoid overlapping letters
      
      lcd.setCursor(0,3);
      lcd.print("PRESS S TO CONTINUE");
      return entry; 
    }
    
    //Leave the number menu
    if (key == '*'){
      lcd.clear();
      lcd.print("     Setup Mode");
      lcd.setCursor(0,2);
      lcd.print("RETURNING ZERO");
      lcd.setCursor(0,3);
      lcd.print("PRESS B TO RETURN");
      return 0;
    }
    
    
    if(key == 'C'){
      lcd.print("       ");
      lcd.setCursor(numbercsr,1);
      switch(z){
        case 7:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = counter[4];
         counter[4] = counter[3];
         counter[3] = counter[2];
         counter[2] = counter[1];
         counter[1] = counter[0];
         counter[0] = 0;
         for(i = 1; i < 7; i++)
         {
          lcd.print(counter[i]); //Prints remaining keys
         }
         z--;
         break;
        case 6:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = counter[4];
         counter[4] = counter[3];
         counter[3] = counter[2];
         counter[2] = counter[1];
         counter[1] = 0;
         for(i = 2; i < 7; i++)
         {
          lcd.print(counter[i]);
         }
         z--;
         break;
        case 5:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = counter[4];
         counter[4] = counter[3];
         counter[3] = counter[2];
         counter[2] = 0;
         for(i = 3; i < 7; i++)
         {
          lcd.print(counter[i]);
         }
         z--;
         break;
        case 4:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = counter[4];
         counter[4] = counter[3];
         counter[3] = 0;
         for(i = 4; i < 7; i++)
         {
          lcd.print(counter[i]);
         }
         z--;
         break;
        case 3:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = counter[4];
         counter[4] = 0;
         for(i = 5; i < 7; i++)
         {
          lcd.print(counter[i]);
         }
         z--;
         break;
        case 2:
         counter[6] = counter[5]; //Deletes the newest key and pushes everything back to the right, to emulate a calculator
         counter[5] = 0;
          lcd.print(counter[6]);
          z--;
          break;
        case 1:
          counter[6] = 0;
          z--;
          break;
        case 0:
          counter[6] = 0;
          break;
      }
    }

  lcd.noCursor();


  goto entryloop;  
}


//Converts mVolts into an analog number that the air valve can use to adjust the load
void DAC() {
   digitalWrite(CS,LOW);
   byte lowByte = mVolts & 0xff;
   byte highByte = ((mVolts >> 8) & 0xff) | 0x10;
   SPI.transfer(highByte);
   SPI.transfer(lowByte);
   digitalWrite(CS,HIGH);
 
   return;
}
