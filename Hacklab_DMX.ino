#include <Wire.h>
#include <LiquidCrystal_I2C.h>          // Specifically for the SPI LCD module I am using.
#include <DmxSimple.h>									// DMX Control library
#include <avr/pgmspace.h>								// Used to store strings in PROGMEM

#define GPIO_ADDR     0x27             // (PCA8574A A0-A2 @5V) typ. A0-A3 Gnd 0x20 / 0x38 for A - 0x27 is the address of the Digispark LCD modules.

// All of the lights, starting DMX channel
#define VIZI1  60			// The two big American DJ Vizi Spot Pros
#define VIZI2  74
#define ELIM1  44			// Eliminator LED modules from Chauvet
#define ELIM2  52
#define MIN1   88			// Chauvet Min Spot RGB fixtures
#define MIN2   101
#define SC25   35  // FUTURE IMPLEMENTATION - these lights are present but not used in the code.
#define AVG    40
#define PAR4   1		// The Par4 can unit, also not currently implemented.

// Pins the buttons are attached to -- the DOWN button currently does not exist.
#define BUTTON_UP 10
#define BUTTON_DOWN 9
#define BUTTON_OK 8

// Pattern names          1234567890123456   <- 16-character LCD
// Use PROGMEM to save RAM
char text00[] PROGMEM = {"1. Party in haus"};
char text01[] PROGMEM = {"2. White Mirror"}; 
char text02[] PROGMEM = {"3. Mirror Colors"};
char text03[] PROGMEM = {"4. Slow Feeling"};
char text04[] PROGMEM = {"5. Purple World"};
char text05[] PROGMEM = {"6. Moose Factory"};
char text06[] PROGMEM = {"7. Smothermoth"};
char text07[] PROGMEM = {"8. Full Retard!"};
char text08[] PROGMEM = {"9. Splines"};

char text09[] PROGMEM = {"10. AUTO CYCLE"};
PROGMEM const char *string_table[] = {
    text00, text01, text02, text03, text04, text05, text06, text07, text08, text09};

char buffer[30];			// A string buffer used for the progmems
byte lastButtonUp;		// The last time the Up button was pressed
byte lastButtonOk; 		// The last time the OK button was pressed
byte lastSeqNum; 			// The last sequence number that was run.

byte MenuMode = 0; 				// Which menu are we in, 0 being the 'idle' menu.
byte MenuItem = 0; 				// Current item in the menu
byte CurrentSequence = 9; // The current sequence that is playing
byte pp = 0; 							// For incremental pattern effects
long lastButton = 0;			// The last time ANY button was pressed.
byte currBeat = 0; 				// Used for beat display on screen
byte lastBeat = 0; 				// Last beat shown on screen so we know when to render the next beat.
byte colorOverride = 0; 	// 0 means NO OVERRIDE, the other numbers are defined below.
long lastAutoChange = 0;	// Last time we changed to the next sequence in 'Auto Sequence' mode.
byte CurrentAutoSequence = 0;		// The current sequence we are in, in auto-sequence mode.

// Define our LCD display
LiquidCrystal_I2C lcd(GPIO_ADDR,16,2);  // set address & 16 chars / 2 lines

long nextBeat = 0; 				// When the next beat should run, based on Millis();
byte SeqStep = 0;					// The current step in the current sequence.

void setup(){

  lcd.init();                           // initialize the lcd 
  lcd.backlight();                      // Print a message to the LCD.
  lcd.print("Startup...");

  DmxSimple.maxChannel(200);						// We're not going to address channels above 200, so this speeds up the DMX transmission a bit.
  
  // Set up the button inputs
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_OK, INPUT);
  digitalWrite(BUTTON_UP, HIGH); // enable pullup resistor
  digitalWrite(BUTTON_OK, HIGH); // enable pullup resistor
  
  // Default channel settings on each of our lamps.
  ViziSetup(VIZI1);
  ViziSetup(VIZI2);
  IntimSetup(ELIM1);
  IntimSetup(ELIM2);
  MinSetup(MIN1);
  MinSetup(MIN2);
  
  // Put the default message on the screen
  updateScreen();
}


void loop() { // This code runs continuously.

  // See if a button is being pressed.
  checkButtons();
  
  
  if (millis() > nextBeat) runNextBeat(); 		// Is it time to advance to the next step in our sequence?
  
  // If no button is pressed in 30 seconds, the Idle screen returns.
  // Time to switch back to the main screen?  
  if ((millis() > lastButton + 30000 || millis() < lastButton) && MenuMode > 0)
  {
    MenuMode = 0;
    updateScreen(); 
  }
  
  if (MenuMode == 0 && lastBeat != SeqStep) // Display current beat on the LCD screen
  {
    lastBeat = SeqStep;
    lcd.setCursor(15, 0);
    lcd.print(SeqStep + 1);
  }
  
}

void runNextBeat() // This is the lion's share of the actual movement code of the lamps.
{
  float stepLen = 2; // By default, a step is two beats.  But some sequences can redefine this on a per-sequence or even per-step basis.
  
  byte useSeq = CurrentSequence; // Which sequence are we displaying?
  
  if (useSeq == 9) // Sequence 9 is the AUTO CYCLE sequence.
  { 
    useSeq = CurrentAutoSequence;
    // Automatically go to the next sequence after 30 seconds.
    if (millis() - lastAutoChange > 30000)
    {
       CurrentAutoSequence++; 
       if (CurrentAutoSequence > 8) CurrentAutoSequence = 0; // There are currently 9 normal sequences.
       lastAutoChange = millis();
       updateScreen();
    }
  }
  
  
  // If the sequence has changed, run the reset routine.  This is to clean up any messy settings left by a sequence, such as strobing, prisms, patterns, etc.  
  // The setup routine should reset each lamp back to a default state.
  if (useSeq != lastSeqNum)
  {
    lastSeqNum = CurrentSequence;
    resetFixtures(); 
  }
  
  
  switch (useSeq) // OK, our big case statement to see what we should be doing for each sequence.
  {
    case 0: // Medium speed fun pattern

			/*
				NOTE: Please see below for what each of these functions does, I'm not going to document them in each sequence.
				But basically they set positions, colors, etc of each lamp.
			*/

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
      ViziPos(VIZI2, 7, 25);
      IntimPos(ELIM1, 127, 127);
      IntimPos(ELIM2, 127, 127);
      break;

    case 2: // Point to Mirror Ball with colors.
    
      ViziPattern(VIZI1, '1', 230);
      ViziPattern(VIZI2, '1', 230);

      ViziPos(VIZI1, 5, 225);
      ViziPos(VIZI2, 7, 25);
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
  
  nextBeat = millis() + (stepLen * 500); // TODO: Implement BPM here, so the speed of the sequences can be adjusted.
}

void resetFixtures()
{
		// Set each sequence back to a known default state.
    MinSetup(MIN1);
    MinSetup(MIN2);
    ViziSetup(VIZI1);
    ViziSetup(VIZI2);
    IntimSetup(ELIM1);
    IntimSetup(ELIM2);
}

char clrWOverride(char c) // If a color override is set, send only the override color.  Otherwise send the requested color.
{
  switch (colorOverride) 
  {
    case 0: return c;
    case 1: return 'r'; // Red
    case 2: return 'g'; // Green
    case 3: return 'b'; // Blue
    case 4: return 'y'; // Yellow
    case 5: return 'w'; // White
  } 
}

void checkButtons()
{
  // See if buttons are currently being pressed.
  byte bRead = digitalRead(BUTTON_UP);
  if (bRead != lastButtonUp) // Has the button state changed?
  {
    if (millis() - lastButton > 50) // Debounce
    {
      lastButtonUp = bRead;
      if (bRead == LOW) butPress(BUTTON_UP); // Trigger a button press action.
      lastButton = millis();
    }
  }
  bRead = digitalRead(BUTTON_OK);
  if (bRead != lastButtonOk)   {
    if (millis() - lastButton > 50)     {
      lastButtonOk = bRead;
      if (bRead == LOW) butPress(BUTTON_OK);
      lastButton = millis();
    }
  }


}

void butPress(byte theButton)
{
  // Complex menu-based code removed, now the left button just sets a color override and the right button sets a pattern.
  // The menu system can come back when there are more options.
  
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

void updateScreen() // Draw the current menu/options to the LCD screen
{
	
  lcd.clear();
  byte cs = CurrentSequence;
  switch (MenuMode)
  { 
    case 0: // Idle menu
      //         1234567890123456
      if (cs == 9) cs = CurrentAutoSequence;
      lcd.print("HACKLITE by AV: ");
      lcd.setCursor(0, 1);
      strcpy_P(buffer, (char*)pgm_read_word(&(string_table[cs]))); // Necessary casts and dereferencing, just copy. 
      lcd.print(buffer);
      
      break;
    case 1: // Main Menu
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
    case 2: // Choose Pattern menu
      //         1234567890123456
      lcd.print("CHOOSE PATTERN");
      lcd.setCursor(0, 1);
      strcpy_P(buffer, (char*)pgm_read_word(&(string_table[CurrentSequence]))); // Necessary casts and dereferencing, just copy. 
      lcd.print(buffer);
      break;
    case 3: // Adjust speed menu (TODO)
      //         1234567890123456
      lcd.print(" ADJUST SPEED ");
      lcd.setCursor(0, 1);
      break;
    case 4: // Colour override menu
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
      
  } 
  
}














/*

This set of functions deals with modifing the DMX channels of individual fixtures.
These really should be programmed as classes, but I didn't really have the time to do it.
Basically you just pass the base channel of each fixture and it knows the offset to get to the setting you want.

*/

void ViziSetup(int chan) // American DJ Vizi Spot Pro
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
  DmxSimple.write(chan + 10, 255); // Set the Strobe to 255 (solid on)
  DmxSimple.write(chan + 11, 255); // Set the dimmer to 255 (solid on)
  DmxSimple.write(chan + 9, 190);  // Set the focus to 190 (a distance of about 20 feet)
  
  ViziPrism(chan, '0', 0); 				 // Turn off the prism
  ViziPattern(chan, '0', 0);   	   // Turn off the gobo wheel
}

void ViziDim(int chan, byte brightness)   // Adjust the brightness of the fixture
{
  DmxSimple.write(chan + 11, brightness); 
}

void ViziStrobe(int chan, byte strobe)		// Change the strobe amount.
{
  DmxSimple.write(chan + 10, strobe);
}

void ViziColor(int chan, char col)				// Set the color wheel position.
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
    case 'o':   DmxSimple.write(chan + 4, 90); break;			// Orange
    case 'p':   DmxSimple.write(chan + 4, 105); break;		// Purple (UV)
    case 'k':   DmxSimple.write(chan + 4, 120); break;		// Pink
    case '0':   DmxSimple.write(chan + 4, 189); break; 		// Color wheel spin, Slow...
    case '1':   DmxSimple.write(chan + 4, 170); break;
    case '2':   DmxSimple.write(chan + 4, 150); break;
    case '3':   DmxSimple.write(chan + 4, 128); break;
    case '4':   DmxSimple.write(chan + 4, 194); break;
    case '5':   DmxSimple.write(chan + 4, 210); break;
    case '6':   DmxSimple.write(chan + 4, 235); break;
    case '7':   DmxSimple.write(chan + 4, 255); break;		// .. to SUPER FAST
  }    
}

void ViziPos(int chan, byte p, byte t) // Set Pan and Tilt position
{
  // X is 42 to 127
  // Y is 0 to 255
  
  DmxSimple.write(chan + 0, p);
  DmxSimple.write(chan + 2, t);  
}

void ViziPrism(int chan, char p, byte s) 	// Set the optical prism  and prism rotation
{
  if (p == '0') DmxSimple.write(chan + 7, 0);		// No prism
  if (p == '3') DmxSimple.write(chan + 7, 32);	// 3-facet prism
  if (p == 'i') DmxSimple.write(chan + 7, 64);  // Infinite prism
  if (p == 'f') DmxSimple.write(chan + 7, 96);  // Frost effect
  DmxSimple.write(chan + 8, s);									// Prism rotation, 0-127 indexed angle, and 128-255 is various rotation speeds and directions, see the fixture manual.
}

void ViziPattern(int chan, char p, byte s)			// Set the gobo.
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


void IntimSetup(int chan)		// Chavet Intimidator LED
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

    case '0':   DmxSimple.write(chan + 2, 128); break; // Spin Slow
    case '1':   DmxSimple.write(chan + 2, 150); break;
    case '2':   DmxSimple.write(chan + 2, 170); break;
    case '3':   DmxSimple.write(chan + 2, 191); break;	// Spin fast
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


void MinSetup(int chan) // Chauvet Min RGB
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

void MinPattern(int chan, char p)		// Gobo
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

