

//#define DEBUG
// #include <TinyWireM.h>                  // I2C Master lib for ATTinys which use USI - comment this out to use with standard arduinos
#include <Wire.h>
#include <LiquidCrystal_I2C.h>          // for LCD w/ GPIO MODIFIED for the ATtiny85
#include <DmxSimple.h>
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>


#define GPIO_ADDR     0x27             // (PCA8574A A0-A2 @5V) typ. A0-A3 Gnd 0x20 / 0x38 for A - 0x27 is the address of the Digispark LCD modules.

// All of the lights
#define VIZI1  60
#define VIZI2  74
#define ELIM1  44
#define ELIM2  52
#define MIN1   88
#define MIN2   101
#define SC25   35  // FUTURE IMPLEMENTATION
#define AVG    40
#define PAR4   1


#define BUTTON_UP 10
#define BUTTON_DOWN 9
#define BUTTON_OK 8

// Pattern names          1234567890123456
// Use PROGMEM to save RAM
const char text00[] PROGMEM = {"1. Party in haus"};
const char text01[] PROGMEM = {"2. White Mirror"}; 
const char text02[] PROGMEM = {"3. Mirror Colors"};
const char text03[] PROGMEM = {"4. Slow Feeling"};
const char text04[] PROGMEM = {"5. Purple World"};
const char text05[] PROGMEM = {"6. Moose Factory"};
const char text06[] PROGMEM = {"7. Smothermoth"};
const char text07[] PROGMEM = {"8. Full Retard!"};
const char text08[] PROGMEM = {"9. Splines"};

const char text09[] PROGMEM = {"10. AUTO CYCLE"};
PGM_P const string_table[] PROGMEM = {
    text00, text01, text02, text03, text04, text05, text06, text07, text08, text09};

char buffer[30];
byte lastButtonUp;
byte lastButtonOk;
byte lastSeqNum; // The last sequence number that was run.

byte LAMP_POWER = 0; // 1 if the lamps are on

byte MenuMode = 0; // Which menu are we in
byte MenuItem = 0; // Current item in the menu
byte CurrentSequence = 9; // The current sequence that is playing
byte pp = 0; // For incremental pattern effects
long lastButton = 0;
byte currBeat = 0; // Used for beat display on screen
byte lastBeat = 0; // Last beat shown on screen
byte colorOverride = 0; // 0 means NO OVERRIDE
long lastAutoChange = 0;
byte CurrentAutoSequence = 0;

LiquidCrystal_I2C lcd(GPIO_ADDR,16,2);  // set address & 16 chars / 2 lines

long nextBeat = 0; // When the next beat should run, based on Millis();
byte SeqStep = 0;

byte BACKLIGHT_ON = 1;

SoftwareSerial mySerial(6, 7); // RX, TX


void setup(){
  lcd.begin();                           // initialize the lcd 
  lcd.backlight();                      // Print a message to the LCD.
  lcd.print("Startup...");

  DmxSimple.maxChannel(200);
  
  
  Serial.begin(9600);
  mySerial.begin(9600);
  
  // Set up the buttons
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_OK, INPUT);
  digitalWrite(BUTTON_UP, HIGH); // enable pullup resistor
  digitalWrite(BUTTON_OK, HIGH); // enable pullup resistor
  
  ViziSetup(VIZI1);
  ViziSetup(VIZI2);
  IntimSetup(ELIM1);
  IntimSetup(ELIM2);
  MinSetup(MIN1);
  MinSetup(MIN2);
  
  
  
  updateScreen();
}


void loop(){

  // See if a button is being pressed.
  checkButtons();
  
  if (millis() > nextBeat) runNextBeat();
  
  // Time to switch back to the main screen?
  if ((millis() > lastButton + 30000 || millis() < lastButton) && MenuMode > 0)
  {
    MenuMode = 0;
    updateScreen(); 
  }
  
  if ((millis() > lastButton + 30000 || millis() < lastButton) && BACKLIGHT_ON == 1)
  {
   BACKLIGHT_ON = 0;
   lcd.noBacklight(); 
  }
  
  if (MenuMode == 0 && lastBeat != SeqStep) // Display current beat on the LCD screen
  {
    lastBeat = SeqStep;
    lcd.setCursor(15, 0);
    lcd.print(SeqStep + 1);
  }
  
}

void runNextBeat()
{
  float stepLen = 2; // By default, a step is two beats.  
  
  byte useSeq = CurrentSequence;
  
  if (useSeq == 9) // AUTO CYCLE
  { 
    useSeq = CurrentAutoSequence;
    // Automatically go to the next sequence after 30 seconds.
    if (millis() - lastAutoChange > 30000)
    {
       CurrentAutoSequence++;
       if (CurrentAutoSequence > 8) CurrentAutoSequence = 0;
       lastAutoChange = millis();
       updateScreen();
    }
  }
  
  
  if (useSeq != lastSeqNum)
  {
    lastSeqNum = CurrentSequence;
    resetFixtures(); 
  }
  
  
  switch (useSeq)
  {
    case 0: // Medium speed fun pattern

      // Step number
      switch (SeqStep)
      {
        case 0:
          MinPos(MIN1, 0, 127);
          MinPos(MIN2, 90, 0);  
          MinColor(MIN1, 255, 0, 0);
          MinColor(MIN2, 0, 0, 255);
          ViziPos(VIZI1, 0, 127);
          ViziPos(VIZI2, 90, 0);  
          ViziColor(VIZI1, 'r');
          ViziColor(VIZI2, 'b');
          IntimColor(ELIM1, 'r');
          IntimColor(ELIM2, 'b');
          IntimPos(ELIM1, 127, 127);
          IntimPos(ELIM2, 127, 127);
          break;
        case 1: 
          MinColor(MIN1, 0, 255, 0);
          MinColor(MIN2, 255, 255, 0);
          MinPattern(MIN1, '1');
          MinPattern(MIN2, '2');
          ViziColor(VIZI1, 'g');
          ViziColor(VIZI2, 'y');
          IntimColor(ELIM1, 'g');
          IntimColor(ELIM2, 'y');
          IntimPos(ELIM1, 0, 127);
          IntimPos(ELIM2, 0, 127);
          break;
        case 2:
          MinColor(MIN1, 0,0,255);
          MinColor(MIN2, 255,255,255);
          MinPos(MIN1, 180, 34);
          MinPos(MIN2, 45, 200);  
          ViziColor(VIZI1, 'b');
          ViziColor(VIZI2, 'w');
          IntimColor(ELIM1, 'b');
          IntimColor(ELIM2, 'w');
          ViziPos(VIZI1, 180, 34);
          ViziPos(VIZI2, 45, 200);  
          IntimPos(ELIM1, 255, 127);
          IntimPos(ELIM2, 255, 127);
          break;
        case 3: 
          MinColor(MIN1, 255,128,128);
          MinColor(MIN2, 255,0,255);
          MinPattern(MIN1, '3');
          MinPattern(MIN2, '4');
          ViziColor(VIZI1, 'k');
          ViziColor(VIZI2, 'o');
          IntimColor(ELIM1, 'k');
          IntimColor(ELIM2, 'o');
          IntimPos(ELIM1, 0, 0);
          IntimPos(ELIM2, 0, 0);
          break;
      }
      SeqStep++;
      if (SeqStep > 3) SeqStep = 0; // 4 steps in this sequence.
      break; 

    case 1: // Point to Mirror Ball.
      
      ViziColor(VIZI1, 'w');
      ViziColor(VIZI2, 'w');
      IntimColor(ELIM1, 'w');
          MinPos(MIN1, pp, 0);
          MinPos(MIN2, pp, 0);  

          MinColor(MIN1, 255, 0, 0);
          MinColor(MIN2, 0, 0, 255);

          MinPattern(MIN1, '6');
          MinPattern(MIN2, '7');
      IntimColor(ELIM2, 'w');
      ViziPos(VIZI1, 5, 225); // Mirror Ball
      ViziPos(VIZI2, 5, 30);
      IntimPos(ELIM1, 127, 127);
      IntimPos(ELIM2, 127, 127);
      break;

    case 2: // Point to Mirror Ball.
      ViziPattern(VIZI1, '1', 230);
      ViziPattern(VIZI2, '1', 230);

      ViziPos(VIZI1, 5, 225);
      ViziPos(VIZI2, 5, 30);
      IntimPos(ELIM1, 127, 127);
      IntimPos(ELIM2, 127, 127);
            
      ViziColor(VIZI1, '0'); // Color spin
      ViziColor(VIZI2, '0');
      IntimColor(ELIM1, '0');
      IntimColor(ELIM2, '0');
      break;

    case 3: // Slow blue 
      stepLen = 0.3;
     
      switch (SeqStep)
      {
        case 0:  
          MinPattern(MIN1, '5');
          MinPattern(MIN2, '4');

          MinColor(MIN1, 0,0,255);
          MinColor(MIN2, 0,0,255);
          
          MinPos(MIN1, pp, pp - 128);
          MinPos(MIN2, 255 - pp, pp - 160);
          ViziPattern(VIZI1, '5', 230);
          ViziPattern(VIZI2, '4', 230);
          ViziPrism(VIZI1, 'i', 150);
          ViziPrism(VIZI2, 'i', 150);

          ViziColor(VIZI1, 'b');
          ViziColor(VIZI2, 'b');
          IntimColor(ELIM1, 'b');
          IntimColor(ELIM2, 'b');
          
          ViziPos(VIZI1, pp, 180);
          ViziPos(VIZI2, 255 - pp, 180);
          IntimPos(ELIM1, pp, 127);
          IntimPos(ELIM2, pp, 127);
          
          pp++;
          if (pp > 250) SeqStep++;
          break;
        case 1:  
          MinPattern(MIN1, '3');
          MinPattern(MIN2, '2');
        
          MinColor(MIN1, 255,0,0);
          MinColor(MIN2, 255,0,0);
          
          MinPos(MIN1, pp, pp - 128);
          MinPos(MIN2, 255 - pp, pp - 160);
          ViziPattern(VIZI1, '3', 230);
          ViziPattern(VIZI2, '2', 230);
        
          ViziPrism(VIZI1, '3', 160);
          ViziPrism(VIZI2, '3', 160);
        
          ViziColor(VIZI1, 'r');
          ViziColor(VIZI2, 'r');
          IntimColor(ELIM1, 'r');
          IntimColor(ELIM2, 'r');
          
          ViziPos(VIZI1, pp, 180);
          ViziPos(VIZI2, 255 - pp, 180);
          IntimPos(ELIM1, pp, 127);
          IntimPos(ELIM2, pp, 127);
          
          pp--;
          if (pp < 3) SeqStep++;
          break;
        default:
          SeqStep = 0;
          break;
      }
      break;
    case 4: // UV

      stepLen = 2;
     
      switch (SeqStep)
      {
        case 0:
          IntimPos(ELIM1, 115,134);
          IntimColor(ELIM1, 'r');
          IntimPattern(ELIM1, '3');
          IntimPos(ELIM2, 130, 122);
          IntimColor(ELIM1, 'r');
          IntimPattern(ELIM1, '3');
          
          ViziPos(VIZI1, 89, 98);
          ViziColor(VIZI1, 'r');
          ViziPattern(VIZI1, '4', 0);
          ViziPos(VIZI2, 75, 232);
          ViziColor(VIZI2, 'r');
          ViziPattern(VIZI2, '4', 0);

          MinPos(MIN1, 193, 69);
          MinColor(MIN1, 255,0,0);
          MinPattern(MIN1, '4');
          MinPos(MIN2, 145, 183);
          MinColor(MIN2, 255,0,0);
          MinPattern(MIN2, '4');

          break;
        case 1:  
          IntimPos(ELIM1, 116, 143);
          IntimColor(ELIM1, 'g');
          IntimPattern(ELIM1, '3');
          IntimPos(ELIM2, 136, 153);
          IntimColor(ELIM1, 'g');
          IntimPattern(ELIM1, '3');
          
          ViziPos(VIZI1, 104, 98);
          ViziColor(VIZI1, 'g');
          ViziPattern(VIZI1, '4', 0);
          ViziPos(VIZI2, 65, 160);
          ViziColor(VIZI2, 'g');
          ViziPattern(VIZI2, '4', 0);

          MinPos(MIN1, 220, 8);
          MinColor(MIN1, 0, 255,0);
          MinPattern(MIN1, '4');
          MinPos(MIN2, 145, 214);
          MinColor(MIN2, 0,255,0);
          MinPattern(MIN2, '4');

          break;
        case 2:  
          IntimPos(ELIM1, 116, 137);
          IntimColor(ELIM1, 'b');
          IntimPattern(ELIM1, '3');
          IntimPos(ELIM2, 136, 123);
          IntimColor(ELIM1, 'b');
          IntimPattern(ELIM1, '3');
          
          ViziPos(VIZI1, 104, 20);
          ViziColor(VIZI1, 'b');
          ViziPattern(VIZI1, '4', 0);
          ViziPos(VIZI2, 65, 249);
          ViziColor(VIZI2, 'b');
          ViziPattern(VIZI2, '4', 0);

          MinPos(MIN1, 160, 53);
          MinColor(MIN1, 0,0,255);
          MinPattern(MIN1, '4');
          MinPos(MIN2, 160, 255);
          MinColor(MIN2, 0,0,255);
          MinPattern(MIN2, '4');

          break;

        case 3:  
          IntimPos(ELIM1, 90, 0);
          IntimColor(ELIM1, 'y');
          IntimPattern(ELIM1, '3');
          IntimPos(ELIM2, 198, 0);
          IntimColor(ELIM1, 'y');
          IntimPattern(ELIM1, '3');
          
          ViziPos(VIZI1, 104, 47);
          ViziColor(VIZI1, 'y');
          ViziPattern(VIZI1, '4', 0);
          ViziPos(VIZI2, 65, 147);
          ViziColor(VIZI2, 'y');
          ViziPattern(VIZI2, '4', 0);

          MinPos(MIN1, 196, 14);
          MinColor(MIN1, 255,255,0);
          MinPattern(MIN1, '4');
          MinPos(MIN2, 137, 78);
          MinColor(MIN2, 255,255,0);
          MinPattern(MIN2, '4');

          break;
      }
      SeqStep++;
      if (SeqStep > 3) SeqStep = 0;
      break;

    case 5:
      stepLen = 4;
      
      switch (SeqStep)
      {   
        case 0:
          MinPos(MIN1, 196, 14);
          MinColor(MIN1, 255,255,0);
          MinPattern(MIN1, 'd');
          MinPos(MIN2, 137, 78);
          MinColor(MIN2, 255,255,0);
          MinPattern(MIN2, 'd');
          ViziPos(VIZI1, 0, 127);
          ViziPos(VIZI2, 0, 127);
          ViziColor(VIZI1, 'y');
          ViziColor(VIZI2, 'r');
          
          break;
        case 1:
          MinPos(MIN1, 50, 50);
          MinColor(MIN1, 255,255,0);
          MinPos(MIN2, 50, 50);
          MinColor(MIN2, 255,255,0);
          ViziPos(VIZI1, 0, 210);
          ViziPos(VIZI2, 0, 30);
          ViziColor(VIZI1, 'r');
          ViziColor(VIZI2, 'y');
          
          
          break;
        case 2:
          MinPos(MIN1, 90, 90);
          MinColor(MIN1, 0,0,255);
          MinPos(MIN2, 90, 90);
          MinColor(MIN2, 0,0,255);
          ViziPos(VIZI1, 50, 210);
          ViziPos(VIZI2, 60, 210);
          ViziColor(VIZI1, 'b');
          ViziColor(VIZI2, 'b');
          
          break;
        case 3:
          MinPos(MIN1, 220, 90);
          MinColor(MIN1, 0,0,255);
          MinPos(MIN2, 90, 220);
          MinColor(MIN2, 0,0,255);

          ViziPos(VIZI1, 5, 225); // Mirror Ball
          ViziPos(VIZI2, 7, 25);
          
        
          break;
      }
      SeqStep++;
      if (SeqStep > 3) SeqStep = 0;
      break;
      
      
   
    case 6:
      stepLen = 0.05;
      
      switch (SeqStep)
      {   
        case 0:
          MinColor(MIN1, pp,pp,pp);
          MinColor(MIN2, 255 - pp,255 - pp,255 - pp);
          
          MinPos(MIN1, 140, pp/4);
          MinPos(MIN2, 31, pp/4);          
          ViziDim(VIZI1, pp);
          ViziDim(VIZI2, 255 - pp);
          ViziColor(VIZI1, 'b');
          ViziColor(VIZI2, 'r');
          
          ViziPos(VIZI1, 72, 50);
          ViziPos(VIZI2, 31, 50);          
          
          pp++;
          if (pp == 255) SeqStep++;
          
          break; 
      
      
       case 1:
          MinColor(MIN1, pp,pp,pp);
          MinColor(MIN2, 255 - pp,255 - pp,255 - pp);

          MinPos(MIN1, 140, pp/4);
          MinPos(MIN2, 31, pp/4);          

          ViziDim(VIZI1, pp);
          ViziDim(VIZI2, 255 - pp);
          ViziColor(VIZI1, 'b');
          ViziColor(VIZI2, 'r');
          pp--;
          if (pp == 0) SeqStep++;
          
          break; 
      
      }
      // SeqStep++;
      if (SeqStep > 1) SeqStep = 0;
      break;

    case 7:
      stepLen = 2;

      MinColor(MIN1, random(255),random(255),random(255));
      MinColor(MIN2, random(255),random(255),random(255));
      MinPos(MIN1, random(255), random(255));
      MinPos(MIN2, random(255), random(255));
      ViziStrobe(VIZI1, 85);
      ViziStrobe(VIZI2, 82);
      ViziColor(VIZI1, '7'); // Crazy fast
      ViziColor(VIZI2, '7');
      ViziPos(VIZI1, random(255), random(255));
      ViziPos(VIZI2, random(255), random(255));

      IntimDim(ELIM1, 255);
      IntimDim(ELIM2, 255);
      IntimColor(ELIM1, '3');
      IntimColor(ELIM2, '3');
      IntimPos(ELIM1, random(255), random(255));
      IntimPos(ELIM2, random(255), random(255));
      
      break;
  
     case 8: // Splines
       
       IntimColor(ELIM1, '0');       
       IntimColor(ELIM2, '0');       
       ViziColor(VIZI1, '0');
       ViziColor(VIZI2, '0');
       MinColor(MIN1, random(255),random(255),random(255));
       MinColor(MIN2, random(255),random(255),random(255));
       

       stepLen = 0.5;
       switch (SeqStep)
       {
         case 0:   // Move the lights only while they are off.
           MinDim(MIN2, 0);
           MinPos(MIN2, random(255), random(255));  
           ViziDim(VIZI1, 255);
           break;
         case 1:  
           ViziDim(VIZI1, 0);
           ViziPos(VIZI1, random(255), random(255));
           ViziDim(VIZI2, 255);
           break;
         case 2:  
           ViziDim(VIZI2, 0);
           ViziPos(VIZI2, random(255), random(255));
           IntimDim(ELIM1, 255);
           break;
         case 3:  
           IntimDim(ELIM1, 0);
           IntimPos(ELIM1, random(255), random(255));
           IntimDim(ELIM2, 255);
           break;
         case 4:  
           IntimDim(ELIM2, 0);
           IntimPos(ELIM2, random(255), random(255));
           MinDim(MIN1, 255);
           break;
         case 5:  
           MinDim(MIN1, 0);
           MinPos(MIN1, random(255), random(255));  
           MinDim(MIN2, 255);
           break;
       }
      
       SeqStep++;
       if (SeqStep > 5) SeqStep = 0;     
     
       break;

  }
  
  nextBeat = millis() + (stepLen * 500); // TODO: Implement BPM here  
}

void resetFixtures()
{
    MinSetup(MIN1);
    MinSetup(MIN2);
    ViziSetup(VIZI1);
    ViziSetup(VIZI2);
    IntimSetup(ELIM1);
    IntimSetup(ELIM2);
}

char clrWOverride(char c)
{
  switch (colorOverride) // If a color override is set, send only the override color.  Otherwise send the requested color.
  {
    case 0: return c;
    case 1: return 'r';
    case 2: return 'g';
    case 3: return 'b';
    case 4: return 'y';
    case 5: return 'w';
  } 
}

void checkButtons()
{
  // See if buttons are being pressed.
  
  byte b1Down = 0;
  byte b2Down = 0;
  
  byte bRead = digitalRead(BUTTON_UP);
  if (bRead == LOW) b1Down = 1;
  if (bRead != lastButtonUp)
  {
    if (millis() - lastButton > 50)
    {
      lastButtonUp = bRead;
      // If the button is down, change the sequence.
      if (bRead == LOW) 
          butPress(BUTTON_UP);
      lastButton = millis();
    }
  }
  bRead = digitalRead(BUTTON_OK);
  if (bRead == LOW) b2Down = 1;
  if (bRead != lastButtonOk)   {
    if (millis() - lastButton > 50)     {
      lastButtonOk = bRead;
      if (bRead == LOW) 
        butPress(BUTTON_OK);
      lastButton = millis();
    }
  }


  if (b1Down == 1 && b2Down == 1)
  {
    // Both buttons are being held down!  Needs to be more than 3000 ms
    if (millis() - lastButton > 3000 && MenuMode != 10)
    {
       // TOGGLE POWER
       MenuMode = 10;
       setLampPower(1 - LAMP_POWER);
       updateScreen();
    } 
  }
  

}

void butPress(byte theButton)
{
  
  if (BACKLIGHT_ON == 0)
  {
   BACKLIGHT_ON = 1;
   lcd.backlight(); 
  }
  
  // Later: Multiple menu modes.
  switch (theButton)
  {
    case BUTTON_UP:
      colorOverride++;
      if (colorOverride > 5) colorOverride = 0;
      MenuMode = 4;
      break;

    case BUTTON_OK:
      CurrentSequence++;
      if (CurrentSequence > 9) CurrentSequence = 0;
      MenuMode = 2;
      break;
  }
  /*
  switch (MenuMode)
  {
    case 0: // Idle menu
      MenuMode = 1; // Go to the main menu
      MenuItem = 0;
      break;
      
    case 1: // Main menu
      switch (theButton)
      {
        case BUTTON_UP:
          MenuItem++;
          if (MenuItem > 2) MenuItem = 0;
          break;
        case BUTTON_OK: // Go into this menu item.
          if (MenuItem == 0) MenuMode = 2; // Pattern menu
          if (MenuItem == 1) MenuMode = 3; // Speed menu
          if (MenuItem == 2) MenuMode = 4; // Color menu
          break;
      }
      break;
    case 2: // Sequence Menu
      switch (theButton)
      {
        case BUTTON_UP:
          CurrentSequence++;
          if (CurrentSequence > 9) CurrentSequence = 0;
          break;
        case BUTTON_OK: // Back to Main Menu
          MenuMode = 1;
          break;
      }
      break;
    case 3: // Speed menu
      MenuMode = 1;
      break;
    case 4: // Color menu
      switch (theButton)
      {
        case BUTTON_UP:
          colorOverride++;
          if (colorOverride > 5) colorOverride = 0;
          break;
        case BUTTON_OK: // Back to Main Menu
          MenuMode = 1;
          break;
      }
      break;
  } 
  */
  updateScreen();
}

void updateScreen()
{
  lcd.clear();
  byte cs = CurrentSequence;
  switch (MenuMode)
  { 
    case 0:
      //         1234567890123456
      if (cs == 9) cs = CurrentAutoSequence;
      lcd.print("HACKLITE by AV: ");
      lcd.setCursor(0, 1);
      strcpy_P(buffer, (char*)pgm_read_word(&(string_table[cs]))); // Necessary casts and dereferencing, just copy. 
      lcd.print(buffer);
      
      break;
    case 1:
      //         1234567890123456
      lcd.print("   MAIN  MENU   ");
      lcd.setCursor(0, 1);
      switch (MenuItem)
      {
        case 0: lcd.print("1. Pattern"); break;
        case 1: lcd.print("2. Speed"); break;
        case 2: lcd.print("3. Colors"); break;
      }
      break;
    case 2:
      //         1234567890123456
      lcd.print("CHOOSE PATTERN");
      lcd.setCursor(0, 1);
      strcpy_P(buffer, (char*)pgm_read_word(&(string_table[CurrentSequence]))); // Necessary casts and dereferencing, just copy. 
      lcd.print(buffer);
      break;
    case 3:
      //         1234567890123456
      lcd.print(" ADJUST SPEED ");
      lcd.setCursor(0, 1);
      break;
    case 4:
      //         1234567890123456
      lcd.print("COLOR OVERRIDE");
      lcd.setCursor(0, 1);
      switch (colorOverride)
      {
        //                 1234567890123456
        case 0: lcd.print("1. No Override"); break;
        case 1: lcd.print("2. Red"); break;
        case 2: lcd.print("3. Green"); break;
        case 3: lcd.print("4. Blue"); break;
        case 4: lcd.print("5. Yellow"); break;
        case 5: lcd.print("6. White"); break;
      }
      break;
      
    case 10:
      //         1234567890123456
      lcd.print("   SHOW POWER   ");
      lcd.setCursor(0, 1);
      //           1234567890123456
      if (LAMP_POWER == 1)
        lcd.print("  POWERING ON"); 
      else
        lcd.print("  POWERING OFF"); 
      
      break;      
      
  } 
  
}


















void ViziSetup(int chan)
{
  // Channel map: 
  // 0-X, 1-XFine, 2-Y, 3-YFine
  // 4-Color
  // 5 Gobo
  // 6 Gobo rotate
  // 7 Prism Type
  // 8 Prism Spin
  // 9 Lens Focus
  // 10 Strobe
  // 11 Dimmer
  // 12 Movement Speed
  // 13 Reset
  DmxSimple.write(chan + 10, 255);
  DmxSimple.write(chan + 11, 255);
  DmxSimple.write(chan + 9, 190);
  
  ViziPrism(chan, '0', 0);
  ViziPattern(chan, '0', 0);  
}

void ViziDim(int chan, byte brightness)
{
  DmxSimple.write(chan + 11, brightness);
}

void ViziStrobe(int chan, byte strobe)
{
  DmxSimple.write(chan + 10, strobe);
}

void ViziColor(int chan, char col)
{
  col = clrWOverride(col);
  switch (col)
  {
    case 'w':   DmxSimple.write(chan + 4, 0); break;
    case 'r':   DmxSimple.write(chan + 4, 15); break;
    case 'b':   DmxSimple.write(chan + 4, 30); break;
    case 'g':   DmxSimple.write(chan + 4, 45); break;
    case 'y':   DmxSimple.write(chan + 4, 60); break;
    case 'm':   DmxSimple.write(chan + 4, 75); break;
    case 'o':   DmxSimple.write(chan + 4, 90); break;
    case 'p':   DmxSimple.write(chan + 4, 105); break;
    case 'k':   DmxSimple.write(chan + 4, 120); break;
    case '0':   DmxSimple.write(chan + 4, 189); break;
    case '1':   DmxSimple.write(chan + 4, 170); break;
    case '2':   DmxSimple.write(chan + 4, 150); break;
    case '3':   DmxSimple.write(chan + 4, 128); break;
    case '4':   DmxSimple.write(chan + 4, 194); break;
    case '5':   DmxSimple.write(chan + 4, 210); break;
    case '6':   DmxSimple.write(chan + 4, 235); break;
    case '7':   DmxSimple.write(chan + 4, 255); break;
  }    
}

void ViziPos(int chan, byte p, byte t)
{
  // X is 42 to 127
  // Y is 0 to 255
  
  DmxSimple.write(chan + 0, p);
  DmxSimple.write(chan + 2, t);  
}

void ViziPrism(int chan, char p, byte s)
{
  if (p == '0') DmxSimple.write(chan + 7, 0);
  if (p == '3') DmxSimple.write(chan + 7, 32);
  if (p == 'i') DmxSimple.write(chan + 7, 64);
  if (p == 'f') DmxSimple.write(chan + 7, 96);
  DmxSimple.write(chan + 8, s);
}

void ViziPattern(int chan, char p, byte s)
{
  if (p == '0') DmxSimple.write(chan + 5, 0);
  if (p == '1') DmxSimple.write(chan + 5, 10);
  if (p == '2') DmxSimple.write(chan + 5, 20);
  if (p == '3') DmxSimple.write(chan + 5, 30);
  if (p == '4') DmxSimple.write(chan + 5, 40);
  if (p == '5') DmxSimple.write(chan + 5, 50);
  if (p == '6') DmxSimple.write(chan + 5, 60);
  if (p == '7') DmxSimple.write(chan + 5, 70);
  DmxSimple.write(chan + 6, s);
}


void IntimSetup(int chan)
{
  // 0 Pan
  // 1 Tilt
  // 2 Color
  // 3 Strobe
  // 4 Dimmer
  // 5 Gobo
  // 6 Function
  // 7 Macro
 DmxSimple.write(chan + 3, 255);
 DmxSimple.write(chan + 4, 255); 
}


void IntimColor(int chan, char col)
{
  col = clrWOverride(col);
  switch (col)
  {
    case 'w':   DmxSimple.write(chan + 2, 0); break;
    case 'l':   DmxSimple.write(chan + 2, 8); break; // Dark blue
    case 'y':   DmxSimple.write(chan + 2, 16); break;
    case 'k':   DmxSimple.write(chan + 2, 24); break;
    case 'g':   DmxSimple.write(chan + 2, 32); break;
    case 'r':   DmxSimple.write(chan + 2, 40); break;
    case 'b':   DmxSimple.write(chan + 2, 48); break;
    case 'o':   DmxSimple.write(chan + 2, 56); break;

    case '0':   DmxSimple.write(chan + 2, 128); break; // Spin
    case '1':   DmxSimple.write(chan + 2, 150); break;
    case '2':   DmxSimple.write(chan + 2, 170); break;
    case '3':   DmxSimple.write(chan + 2, 191); break;
  }    
}

void IntimPos(int chan, byte p, byte t)
{
  DmxSimple.write(chan + 0, p);
  DmxSimple.write(chan + 1, t);  
}

void IntimDim(int chan, byte d)
{
  DmxSimple.write(chan + 4, d);
}


void IntimPattern(int chan, char p)
{
  if (p == '0') DmxSimple.write(chan + 5, 0);
  if (p == '1') DmxSimple.write(chan + 5, 8);
  if (p == '2') DmxSimple.write(chan + 5, 16);
  if (p == '3') DmxSimple.write(chan + 5, 24);
  if (p == '4') DmxSimple.write(chan + 5, 32);
  if (p == '5') DmxSimple.write(chan + 5, 40);
  if (p == '6') DmxSimple.write(chan + 5, 48);
  if (p == '7') DmxSimple.write(chan + 5, 56);
}


void MinSetup(int chan)
{
  // 0 Pan
  // 1 Pan Fine
  // 2 Tilt
  // 3 Tilt Fine
  // 4 Vector Speed
  // 5 Dimmer/Strobe   8 to 134 - 100 to 0
  // 6 R
  // 7 G
  // 8 B
  // 9 Color Macro
  // 10 Color Speed
  // 11 Move Macro
  // 12 Pattern
 
  DmxSimple.write(chan + 5, 255);    
  DmxSimple.write(chan + 12, 0);
}

void MinColor(int chan, byte r, byte g, byte b)
{
  switch (colorOverride) // If a color override is set, send only the override color.  Otherwise send the requested color.
  {
    case 0: break;
    case 1: r = 255; g = 0; b = 0; break;
    case 2: r = 0; g = 255; b = 0; break;
    case 3: r = 0; g = 0; b = 255; break;
    case 4: r = 255; g = 255; b = 0; break;
    case 5: r = 255; g = 255; b = 255; break;
  } 

  DmxSimple.write(chan + 6, r);    
  DmxSimple.write(chan + 7, g);    
  DmxSimple.write(chan + 8, b);   
}

void MinPos(int chan, byte p, byte t)
{
  DmxSimple.write(chan + 0, p);
  DmxSimple.write(chan + 2, t);  
}

void MinDim(int chan, byte d)
{
  DmxSimple.write(chan + 5, (127 - (d / 2)) + 8);
}

void MinPattern(int chan, char p)
{
  if (p == '0') DmxSimple.write(chan + 12, 0);
  if (p == '1') DmxSimple.write(chan + 12, 13);
  if (p == '2') DmxSimple.write(chan + 12, 16);
  if (p == '3') DmxSimple.write(chan + 12, 24);
  if (p == '4') DmxSimple.write(chan + 12, 32);
  if (p == '5') DmxSimple.write(chan + 12, 40);
  if (p == '6') DmxSimple.write(chan + 12, 48);
  if (p == '7') DmxSimple.write(chan + 12, 56);
  if (p == '8') DmxSimple.write(chan + 12, 64);
  if (p == '9') DmxSimple.write(chan + 12, 72);

  if (p == 'a') DmxSimple.write(chan + 12, 220); // Slow
  if (p == 'b') DmxSimple.write(chan + 12, 230);
  if (p == 'c') DmxSimple.write(chan + 12, 240);
  if (p == 'd') DmxSimple.write(chan + 12, 250); // Fast
}


void setLampPower(byte setPower)
{
  LAMP_POWER = setPower;
  // HLCMD("192.168.2.44", "/test/light.asp")
  // HLCMD("192.168.2.44", "/test/light.asp?p=off")
  mySerial.print("HLCMD(\"192.168.2.44\", \"/test/light.asp?p=");
  if (LAMP_POWER==1) mySerial.println("on\")");
  if (LAMP_POWER==0) mySerial.println("off\")");
  

}
