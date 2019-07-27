#include <LiquidCrystal.h>
#include <EEPROM.h>
#define scroll_start_delay 3
#define EEPROM_read_bytes 16
#define EEPROM_read_lines 32
#define morse_speed 250 // ms
#define end_lights_speed 4
#define r1 8
#define r2 7
#define r3 6
#define r4 5
#define c1 4
#define c2 3
#define c3 2
#define rled 11
#define gled 10
#define bled 9
const byte sprite_count = 4;
LiquidCrystal lcd(A0,A1,A2,A3,A4,A5);
byte screen,serial,t[12],pos,sz,menu_start,menu_length,menu_end,input_size,next_scr,prev_scr,after_scr,game_tick,game_speed,
     serial_screen_list[]={2,8};
unsigned long hash,hash_old;
int scroll_menu_pos,scroll_title_pos;
  //0,1,2,3,4,5,6,7,8,9,del,enter;
bool up_scr,scroll_title,scroll_menu,b[12],serial_available,scroll_title_itr,scroll_menu_itr,menu_locked,serial_screen,game_finished;
char input[17],k[17],
     splash[] = "Powered byArduino",
     hex_charset[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

const char levels[][2][16] = {
  {
    { 1,32,32,32,32,32, 2,32,32, 0, 0,32,32,32,32, 3},
    {32,32,32,32, 0,32,32,32,32,32,32,32, 0,32, 2, 3}
  },
  {
    { 2,32,32,32,32, 0,32, 1,32,32,32,32, 0,32, 6, 3},
    { 4,32, 0,32,32,32,32,32,32,32, 0,32,32, 2, 6, 3}
  },
  {
    { 6,32,32, 0, 0,32,32,32, 4,32,32,32,32,32,32, 1},
    { 3, 6,32, 2,32,32,32,32, 0, 0, 0,32,32,32,32,32}
  },
  {
    { 3,32,32, 2,32, 6,32,32,32,32,32,32,32,32,32, 4},
    { 3,32,32,32,32, 0,32,32, 1,32,32, 0,32,32,32, 2}
  }
};

const byte morse_code[] = {
//0      1      2      3      4      5      6      7      8      9
  B11111,B01111,B00111,B00011,B00001,B00000,B10000,B11000,B11100,B11110,
  
//A   B     C     D    E  F     G    H     I   J     K    L     M   N   O
  B01,B1000,B1010,B100,B0,B0010,B110,B0000,B00,B0111,B101,B0100,B11,B10,B111,
//P     Q     R    S    T  U    V     W    X     Y     Z
  B0110,B1101,B010,B000,B1,B001,B0001,B011,B1001,B1011,B1100
};

const byte morse_length[] = {5,5,5,5,5,5,5,5,5,5,
                             2,4,4,3,1,4,3,4,2,4,3,4,2,2,3,
                             4,4,3,3,1,3,4,3,4,4,4};

char level[2][16];
byte level_update_map[2][2];
byte level_speeds[] = {3,3,1,2};
byte game_after_screens[] = {13,14,16,15};

byte game1_sprites[][8] {
  {
    B11111,
    B01000,
    B01000,
    B01000,
    B11111,
    B00010,
    B00010,
    B00010
  },{
    B01110,
    B10001,
    B10001,
    B01110,
    B00100,
    B11111,
    B00100,
    B01010
  },{
    B01110,
    B11111,
    B10101,
    B11111,
    B10101,
    B01010,
    B11111,
    B01010
  },{
    B11001,
    B11001,
    B00110,
    B00110,
    B11001,
    B11001,
    B00110,
    B00110
  },{
    B01110,
    B10001,
    B10001,
    B01110,
    B00100,
    B00110,
    B00100,
    B00110
  },{
    B00010,
    B00101,
    B00101,
    B00100,
    B11110,
    B11110,
    B11110,
    B00000
  },{
    B00000,
    B01100,
    B10010,
    B10010,
    B11110,
    B11110,
    B11110,
    B00000
  }
};

byte cryptic_text[][8] = {
  {
    0,0,
    B11000,
    B00110,
    B00001,
    B00110,
    B11000,
    0
  },{
    0,0,
    B10001,
    B10001,
    B01010,
    B01010,
    B00100,
    0
  },{
    0,0,
    B10001,
    B10001,
    B10101,
    B10001,
    B11111,
    0
  },{
    0,0,
    B11111,
    B00001,
    B00101,
    B00001,
    B00001,
    0
  },{
    0,0,
    B11111,
    B00001,
    B00001,
    B00001,
    B11111,
    0
  },{
    0,0,
    B11111,
    B10000,
    B10100,
    B10000,
    B10000,
    0
  },{
    0,0,
    B11111,
    B10001,
    B10101,
    B10001,
    B11111,
    0
  },{
    0,0,
    B11111,
    B00001,
    B00001,
    B00001,
    B00001,
    0
  }
};

unsigned long appeared;
/*  0: Password Unlock
 *  1: Main Menu
 *  2: Developer Mode
 *  3: Wrong Password
 *  4: Correct Password
 *  5: Hash Generator Input
 *  6: Hash Generator
 *  7: Hash Generator Output
 *  8: EEPROM Editor
 *  9: Room Light Unlocked
 * 10: Game Level 1
 * 11: You Lost
 * 12: You Won!
 * 13: Game Level 2
 * 14: Escape the Room password 1
 * 15: The end
 * 16: Escape the Room password 2
 * 17: Game Level 3
 * 18: Escape the Room password 3
 * 19: Game Level 4
 * 20: The End
 */

char scr_t[] = "Main Menu"                // 9
               "Hash Generator"           // 23 
               "Enter Password"           // 37
               "Developer Mode"           // 51
               "Wrong Password"           // 65
               "Accepted"                 // 73
               "(0) Unlock Room Light"    // 94
               "Escape the Room"          // 109
               "Enter Number"             // 121
               "Please Wait..."           // 135
               "Hashed code:    ........ "// 160
               "EEPROM Editor"            // 173
               "Room Light Un-  locked"   // 195
               "You Lost"                 // 203
               "You Won!"                 // 211
               "Congratulations!........";// 235
byte scr_t_size[][2] = {
  {23,14},  //  0
  {0,9},    //  1
  {37,14},  //  2
  {51,14},  //  3
  {65,8},   //  4
  {109,12}, //  5
  {121,14}, //  6
  {135,25}, //  7
  {160,13}, //  8
  {173,22}, //  9
  {0,0},    // 10
  {195,8},  // 11
  {203,8},  // 12
  {0,0},    // 13
  {23,14},  // 14
  {211,24}, // 15
  {23,14},  // 16
  {0,0},    // 17
  {23,14},  // 18
  {0,0}     // 19
};
// Screen Types:
// 0: Message
// 1: Menu
// 2: Input
// 3: Game 1
const byte scr_type[][2] = {
  {2,4}, //  0
  {1,0}, //  1
  {0,0}, //  2
  {0,0}, //  3
  {0,0}, //  4
  {2,8}, //  5
  {0,0}, //  6
  {0,0}, //  7
  {0,0}, //  8
  {0,0}, //  9
  {3,0}, // 10
  {0,0}, // 11
  {0,0}, // 12
  {3,1}, // 13
  {2,8}, // 14
  {0,0}, // 15
  {2,4}, // 16
  {3,2}, // 17
  {2,4}, // 18
  {3,3}, // 19
  {0,0}  // 20
};
const byte mx_title[] = {32,12,16}; // Maximum characters for the title for each screen type
byte scr_m[] = {
  2,B11,
  9,14,
  94,15,
  5,10
};

void fatal_error() {
  lcd.setCursor(0,0);
  lcd.print("Fatal Error");
  exit(0);
}
/*
void pwm(byte pin,int ms,int duty_cycle) {
  for (int i=0;i<ms;i++) {
    digitalWrite(pin,0);
    delayMicroseconds(1024-duty_cycle*4);
    digitalWrite(pin,1);
    delayMicroseconds(duty_cycle*4);
  }
}
*/
ISR(TIMER2_COMPA_vect){
  for (byte i=0;i<12;i++) 
    if (t[i]>0)
      t[i]--;
  //itr_btn = false;
}

ISR(TIMER1_COMPA_vect) {
  if (scroll_title&&!menu_locked)
    scroll_title_itr = true;
  if (scroll_menu)
    scroll_menu_itr = true;
  game_tick++;
}

byte level_needs_update(byte x,byte y) {
  return bitRead(level_update_map[y][x/8],x%8);
}

void level_disable_update(byte x,byte y) {
  bitClear(level_update_map[y][x/8],x%8);
}

void empty_input() {
  for (byte i=0;i<16;i++)
    input[i] = 0;
}

bool is_hex(char c) {
  if ('0'<=c&&c<='9')
    return true;
  if ('a'<=c&&c<='f')
    return true;
  if ('A'<=c&&c<='F')
    return true;
  return false;
}

byte hex_to_byte(char c) {
  if ('0'<=c&&c<='9')
    return c-'0';
  if ('a'<=c&&c<='f')
    return c-'a'+10;
  if ('A'<=c&&c<='F')
    return c-'A'+10;
  return 0;
}

void update_menu() {
  scroll_menu = false;
  scroll_menu_pos = 0;
  lcd.setCursor(13,0);
  lcd.print(pos+1);
  lcd.setCursor(0,1);
  if ((scr_m[scr_type[screen][1]+1]>>pos)%2)
    lcd.print('>');
  else
    lcd.print('X');
  lcd.print(' ');
  menu_start = scr_m[scr_type[screen][1]+pos*2+2];
  menu_length = scr_m[scr_type[screen][1]+pos*2+3];
  menu_end = menu_start+menu_length;
  byte e;
  if (menu_length>14) {
    scroll_menu = true;
    e = menu_start+14;
    scroll_menu_pos = -scroll_start_delay;
  } else
    e = menu_start+menu_length;
  for (byte i=menu_start;i<e;i++)
    lcd.print(scr_t[i]);
  if (menu_locked) {
    menu_locked = false;
    lcd.setCursor(0,0);
    if (!scroll_title)
      for (byte i=scr_t_size[screen][0];i<scr_t_size[screen][0]+scr_t_size[screen][1];i++)
        lcd.print(scr_t[i]);
  }
}

void update_game1() {
  lcd.setCursor(0,0);
  for (byte i=0;i<16;i++)
    lcd.write(level[0][i]);
  lcd.setCursor(0,1);
  for (byte i=0;i<16;i++)
    lcd.write(level[1][i]);
  game_tick = 0;
}

void update_screen() {
  Serial.write(7);
  prev_scr = screen;
  screen = next_scr;
  Serial.println("Updating screen");
  lcd.clear();
  lcd.noCursor();
  lcd.noBlink();
  scroll_title=false;
  scroll_title_pos = 0;
  scroll_menu=false;
  scroll_menu_pos = 0;

  digitalWrite(rled,1);
  digitalWrite(gled,1);
  digitalWrite(bled,1);
  
  serial_screen = false;
  for (byte i=0;i<sizeof(serial_screen_list);i++)
    if (serial_screen_list[i]==screen)
      serial_screen = true;
  
  switch(screen) {
    case 15:
      for (byte i=0;i<8;i++)
        lcd.createChar(i, cryptic_text[i]);
      lcd.begin(16,2);
      break;
  }
  
  if (scr_type[screen][0]!=3) {
    int n;
    byte mx = mx_title[scr_type[screen][0]];
    pos = 0;
    for (byte i=0;i<16;i++)
      input[i] = 0;
    
    if (scr_t_size[screen][1]>mx) {
      scroll_title=true;
      n = scr_t_size[screen][0]+mx;
    } else {
      n = scr_t_size[screen][0]+scr_t_size[screen][1];
    }
    
    for (byte i=scr_t_size[screen][0];i<n;i++) {
      if (i-scr_t_size[screen][0]==16)
        lcd.setCursor(0,1);
      lcd.print(scr_t[i]);
    }
  }
  switch(scr_type[screen][0]) {
    case 1:
      sz = scr_m[scr_type[screen][1]];
      lcd.setCursor(13,0);
      lcd.print("1/");
      lcd.print(sz);
      update_menu();
      break;
    case 2:
      pos=0;
      lcd.setCursor(0,1);
      for (byte i=0;i<scr_type[screen][1];i++) {
        lcd.print('_');
        input[i] = 0;
      }
      lcd.setCursor(0,1);
      lcd.cursor();
      lcd.blink();
      break;
    case 3:
      for (byte i=0;i<7;i++)
        lcd.createChar(i, game1_sprites[i]);
      lcd.begin(16,2);
      after_scr = scr_type[screen][1];
      for (byte i=0;i<2;i++)
        for (byte j=0;j<16;j++) {
          level[i][j] = levels[scr_type[screen][1]][i][j];
          if (level[i][j]==1)
            pos = i*16+j;
        }
        game_speed = level_speeds[scr_type[screen][1]];
      after_scr = game_after_screens[scr_type[screen][1]];
      update_game1();
  }

  switch(screen) {
    case 0:
      for (byte i=0;i<4;i++)
        k[i] = "5869"[i];
      break;
    case 3:
      digitalWrite(rled,0);
      break;
    case 4:
      digitalWrite(gled,0);
      break;
    case 11:
      digitalWrite(rled,0);
      break;
    case 12:
      digitalWrite(gled,0);
      break;
    case 14:
      for (byte i=0;i<8;i++)
        k[i] = "25133195"[i];
      break;
    case 16:
      for (byte i=0;i<4;i++)
        k[i] = "6000"[i];
      break;
  }
  
  up_scr = false;
  appeared = millis();
}

void reset_buttons() {
  for (byte i=0;i<12;i++)
    b[i] = false;
}

void push_button(byte pin,byte bt) {
  if (!digitalRead(pin)) {
    if (t[bt]==0)
      b[bt] = true;
    t[bt]=20; 
  }
}

bool check_input(byte l) {
  for (byte i=0;i<l;i++)
    if (input[i]!=k[i])
      return false;
  return true;
}

void set_screen(byte scr) {
  next_scr = scr;
  up_scr = true;
}

void game1_player_move(byte x,byte y) {
  //Serial.println("Moving player");
  switch (level[y][x]) {
    case 0:
      break;
    case 1:
      fatal_error();
    case 2:
      set_screen(11);
      break;
    case 3:
      set_screen(12);
      break;
    case 4:
      for (byte y=0;y<2;y++)
        for (byte x=0;x<16;x++)
          if (level[y][x]==6)
            level[y][x]=5;
      update_game1();
    case 5:
    case 32:
      lcd.setCursor(pos%16,pos/16);
      if (levels[scr_type[screen][1]][pos/16][pos%16]==6) {
        lcd.write(5);
        level[pos/16][pos%16] = 5;
      } else {
        lcd.write(32);
        level[pos/16][pos%16] = 32;
      }
      lcd.setCursor(x,y);
      lcd.write(1);
      level[y][x] = 1;
      pos = y*16+x;
      //update_game1();
  }
}

bool is_unlocked() {
  return (scr_m[scr_type[screen][1]+1]>>pos)%2;
}
/*
void morse(char s,byte led) {
  //Serial.println("Morse");
  byte index;
  if ('0'<=s&&s<='9')
    index=s-'0';
  else if ('A'<=s&&s<='Z')
    index=s-'A'+10;
  else if ('a'<=s&&s<='z')
    index=s-'a'+10;
  else
    fatal_error();
  for (byte i=morse_length[index]-1;i<8;i--) {
    //Serial.print(s);
    //Serial.println(i);
    digitalWrite(led,0);
    if (bitRead(morse_code[index],i))
      delay(morse_speed*3);
    else
      delay(morse_speed);
    digitalWrite(led,1);
    delay(morse_speed);
  }
}*/

void setup() {
  pinMode(rled,OUTPUT);
  pinMode(gled,OUTPUT);
  pinMode(bled,OUTPUT);
  digitalWrite(rled,0);
  digitalWrite(gled,0);
  digitalWrite(bled,1);
  lcd.begin(16,2);
  
  for (int i=0;i<8;i++)
    scr_t[227+i]=i;
  
  for (int i=0;i<17;i++) {
    if (i==10)
      lcd.setCursor(0,1);
    lcd.print(splash[i]);
    delay(33);
  }
  
  pinMode(r1,OUTPUT);
  pinMode(r2,OUTPUT);
  pinMode(r3,OUTPUT);
  pinMode(r4,OUTPUT);
  pinMode(c1,INPUT_PULLUP);
  pinMode(c2,INPUT_PULLUP);
  pinMode(c3,INPUT_PULLUP);
  
  digitalWrite(r1,HIGH);
  digitalWrite(r2,HIGH);
  digitalWrite(r3,HIGH);
  digitalWrite(r4,HIGH);
  
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 3906;
  TCCR1B|=(1 << WGM12);
  TCCR1B|=(1 << CS12) | (1 << CS10);  
  TIMSK1|=(1 << OCIE1A);
  
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2  = 0;
  OCR2A  = 250;
  TCCR2A|= (1 << WGM21);
  TCCR2B|= (1 << CS22);
  TIMSK2|= (1 << OCIE2A);
  sei();

  /*byte room_light_byte = EEPROM.read(0);
  if (room_light_byte) {
    bitClear(scr_m[1],1);
    scr_t[74]= '0'+room_light_byte;
    EEPROM.write(0,room_light_byte-1);
  } else {
    bitSet(scr_m[1],1);
  }*/
  
  /***/
  /*
  digitalWrite(rled,0);
  delay(100);
  digitalWrite(rled,1);
  analogWrite(10,254);
  delay(900);
  digitalWrite(rled,0);
  delay(100);
  digitalWrite(rled,1);
  analogWrite(10,127);
  delay(900);
  digitalWrite(rled,0);
  delay(100);
  digitalWrite(rled,1);
  analogWrite(10,0);
  delay(900);
  digitalWrite(rled,0);
  delay(100);
  digitalWrite(rled,1);
  */
  /***/
  
  delay(1000);
  set_screen(0);
  lcd.clear();
  Serial.begin(115200);
  
}

void refresh_keypad() {
  pinMode(c1,OUTPUT);
  pinMode(c2,OUTPUT);
  pinMode(c3,OUTPUT);
  digitalWrite(c1,LOW);
  digitalWrite(c2,LOW);
  digitalWrite(c3,LOW);
  delay(1);
  pinMode(c1,INPUT_PULLUP);
  pinMode(c2,INPUT_PULLUP);
  pinMode(c3,INPUT_PULLUP);
}

void loop() {
  Serial.write(127);
  serial_available = Serial.available()>0;
  if (serial_available)
    serial = Serial.read();
  //Serial.print('.');
  /*
  digitalWrite(2,0);
  delay(10);
  push_button(6,3);
  push_button(7,2);
  push_button(8,1);
  digitalWrite(2,1);
  digitalWrite(3,0);
  delay(10);
  push_button(6,6);
  push_button(7,5);
  push_button(8,4);
  digitalWrite(3,1);
  digitalWrite(4,0);
  delay(10);
  push_button(6,9);
  push_button(7,8);
  push_button(8,7);
  digitalWrite(4,1);
  digitalWrite(5,0);
  delay(10);
  push_button(6,11);
  push_button(7,0);
  push_button(8,10);
  digitalWrite(5,1);
  */
  
  // 2:R4,3:R3,4:R2,5:R1,6:C3,7:C2,8:C1
  // 9:B,10:G,11:B
  
  
  refresh_keypad();
  digitalWrite(r1,0);
  push_button(c1,1);
  push_button(c2,2);
  push_button(c3,3);
  digitalWrite(r1,1);
  
  refresh_keypad();
  digitalWrite(r2,0);
  push_button(c1,4);
  push_button(c2,5);
  push_button(c3,6);
  digitalWrite(r2,1);
  
  refresh_keypad();
  digitalWrite(r3,0);
  push_button(c1,7);
  push_button(c2,8);
  push_button(c3,9);
  digitalWrite(r3,1);
  
  refresh_keypad();
  digitalWrite(r4,0);
  push_button(c1,10);
  push_button(c2,0);
  push_button(c3,11);
  digitalWrite(r4,1);
  
  if (!serial_screen && serial_available) {
    if ('1'<=serial&&serial<='3') {
      b[serial-'0'+6] = true;
    } else if ('4'<=serial&&serial<='6'||serial=='0') {
      b[serial-'0'] = true;
    } else if ('7'<=serial&&serial<='9') {
      b[serial-'0'-6] = true;
    } /*else if (('A'<=serial&&serial<='Z')||('a'<=serial&&serial<='z')||serial==' ') {
      input[pos] = serial;
      lcd.print((char)serial);
      pos++;
    }*/ else switch (serial) {
      case 127:
        b[10] = true;
        break;
      case 13:
        b[11] = true;
        break;
      case '#':
        set_screen(2);
        Serial.println("Dev mode");
        break;
      case 's':
        set_screen(game_after_screens[scr_type[screen][1]]);
        break;
      default:
        Serial.print('?');
        Serial.print(serial);
    }
  }
  
  if (up_scr)
    update_screen();

  if (scroll_title_itr) {
    byte s,e=scr_t_size[screen][0]+scr_t_size[screen][1];
    if (scroll_title_pos>scr_t_size[screen][1]+6)
      scroll_title_pos=scroll_start_delay*2;
    if (scroll_title_pos<0)
      s = scr_t_size[screen][0];
    else
      s = scr_t_size[screen][0]+scroll_title_pos;
    lcd.setCursor(0,0);
    if (scroll_title_pos==scroll_start_delay*2 || scroll_title_pos>=0)
      for (int i=s; i<s + mx_title[scr_type[screen][0]]; i++) {
        if (i<e)
          lcd.print(scr_t[i]);
        else if (i<e+6)
          lcd.print(' ');
        else
          lcd.print(scr_t[i-scr_t_size[screen][1]-6]);
      }
    
    if (scr_type[screen][0]==2)
      lcd.setCursor(pos,1);
    
    scroll_title_pos++;
    scroll_title_itr = false;
  }
  if (scroll_menu_itr) {
    byte s;
    if (scroll_menu_pos>menu_length+6)
      scroll_menu_pos=scroll_start_delay*2;
    if (scroll_menu_pos<0)
      s = menu_start;
    else
      s = menu_start+scroll_menu_pos;
    lcd.setCursor(2,1);
    if (scroll_menu_pos==scroll_start_delay*2 || scroll_menu_pos>=0)
      for (int i=s;i<s+14;i++) {
        if (i<menu_end)
          lcd.print(scr_t[i]);
        else if (i<menu_end+6)
          lcd.print(' ');
        else
          lcd.print(scr_t[i-menu_length-6]);
      }
    scroll_menu_pos++;
    scroll_menu_itr = false;
  }
  
  switch (screen) {
    case 0:
      if (b[11]) {
        if (check_input(4))
          set_screen(4);
        else
          set_screen(3);
      } break;
    
    case 2:
      if (serial_available) {
        switch (serial) {
          case 'r':
            screen=0;
            pos=0;
            scroll_title=false;
            scroll_menu=false;
            Serial.println("Reset");
            setup();
            break;
          case 'e':
            set_screen(8);
            break;
          case 'w':
            set_screen(15);
            break;
        }
      }
      lcd.setCursor(0,1);
      Serial.println();
      for (byte i=0;i<12;i++) {
        if (t[i]>0) {
          if (i<10) {
            lcd.print(i);
            Serial.print(i);
          }
          else if (i==10) {
            lcd.print('d');
            Serial.print('d');
          }
          else {
            lcd.print('e');
            Serial.print('e');
          }
        } else {
          lcd.print('.');
            Serial.print('.');
        }
      }
      lcd.print("  ");
      lcd.print(t[0]);
      lcd.print(' ');
      break;
      
    case 3:
      if (millis()-appeared>=2000)
        set_screen(prev_scr);
      break;
      
    case 4:
      randomSeed(millis());
      if (millis()-appeared>=2000)
        set_screen(1);
      break;
    case 5:
      if (b[10]&&pos==0)
        set_screen(1);
      
      if (b[11]) {
        hash=0;
        for (byte i=0;i<pos;i++) {
          hash*=10;
          hash+= (input[i]-'0');
        }
        Serial.print(hash);
        Serial.println(" will be hashed");
        set_screen(6);
      } break;
    case 6:
      digitalWrite(12,0);
      for (unsigned int i=0;i<(unsigned int)65535;i++) {
        /*if (i%1024==0) {
          Serial.print("Cycle ");
          Serial.println(i);
        }*/
        hash_old = hash;
        byte n = hash%20;
        hash++;
        for (byte j=0;j<n;j++) {
          hash*=2;
          hash--;
          hash << (hash_old/2%2+1);
          hash/=2;
          hash++;
          hash >> (hash_old/2%2+1);
          hash+=hash_old;
        }
      }
      Serial.print("Decimal: ");
      Serial.println(hash);
      Serial.print("Hexadecimal: ");
      for (byte i=0;i<8;i++) 
        scr_t[151+i] = hex_charset[(hash>>28-i*4)%16];
      
      set_screen(7);
      break;
    case 7:
      if (b[10]&&millis()-appeared>2000)
        set_screen(1);
      /*if (b[11]&&millis()-appeared>100) {
        delay(500);
        for (byte i=0;i<8;i++) {
          morse(hash_hex[i],11);
          delay(morse_speed*2);
        }
      }*/
      break;
    case 8:
      if (serial_available) {
        switch(serial) {/*
          case 'r':
            Serial.println();
            Serial.println();
            char EEPROM_bytes[EEPROM_read_bytes+1];
            EEPROM_bytes[EEPROM_read_bytes] = 0;
            int address;
            for (int i=0;i<EEPROM.length()/EEPROM_read_bytes;i++) {
              if (i%EEPROM_read_lines==0&&i!=0) {
                Serial.println("Press any key to continue...");
                while (!Serial.available());
                Serial.read();
              }
              
              Serial.print("0x");
              Serial.print(hex_charset[(i*EEPROM_read_bytes)/256]);
              Serial.print(hex_charset[((i*EEPROM_read_bytes)/16)%16]);
              Serial.print(hex_charset[(i*EEPROM_read_bytes)%16]);
              Serial.print("  ");
              for (int j=0;j<EEPROM_read_bytes;j++) {
                address = i*EEPROM_read_bytes+j;
                byte EEPROM_byte = EEPROM.read(address);
                Serial.print(hex_charset[EEPROM_byte/16]);
                Serial.print(hex_charset[EEPROM_byte%16]);
                Serial.print(' ');
                if (32<=EEPROM_byte&&EEPROM_byte<127)
                  EEPROM_bytes[j] = EEPROM_byte;
                else
                  EEPROM_bytes[j] = '.';
              }
              Serial.println(EEPROM_bytes);
            }
            Serial.print("\n\n\n");
            break;*/
          case 'q':
            set_screen(2);
            break;
          case 'w': {
            int address = 0;
            Serial.print("Enter Address: ");
            bool w_f_input = true;
            while (w_f_input) {
              if (Serial.available()) {
                serial = Serial.read();
                if (is_hex(serial)) {
                  address+=hex_to_byte(serial)*256;
                  Serial.write(serial);
                  w_f_input = false;
                }
                else if (serial=='x')
                  goto EEPROM_write_cancel;
                
              }
            }
            w_f_input = true;
            while (w_f_input) {
              if (Serial.available()) {
                serial = Serial.read();
                if (is_hex(serial)) {
                  address+=hex_to_byte(serial)*16;
                  Serial.write(serial);
                  w_f_input = false;
                }
                else if (serial=='x')
                  goto EEPROM_write_cancel;
                
              }
            }
            w_f_input = true;
            while (w_f_input) {
              if (Serial.available()) {
                serial = Serial.read();
                if (is_hex(serial)) {
                  address+=hex_to_byte(serial);
                  Serial.write(serial);
                  w_f_input = false;
                }
                else if (serial=='x')
                  goto EEPROM_write_cancel;
                
              }
            }
            Serial.println();
            byte data = 0;
            Serial.print("Enter Data: ");
            w_f_input = true;
            while (w_f_input) {
              if (Serial.available()) {
                serial = Serial.read();
                if (is_hex(serial)) {
                  data+=hex_to_byte(serial)*16;
                  Serial.write(serial);
                  w_f_input = false;
                }
                else if (serial=='x')
                  goto EEPROM_write_cancel;
                
              }
            }
            w_f_input = true;
            while (w_f_input) {
              if (Serial.available()) {
                serial = Serial.read();
                if (is_hex(serial)) {
                  data+=hex_to_byte(serial);
                  Serial.write(serial);
                  w_f_input = false;
                }
                else if (serial=='x')
                  goto EEPROM_write_cancel;
                
              }
            }
            Serial.println();
            Serial.print("0x");
            Serial.print(hex_charset[address/256]);
            Serial.print(hex_charset[(address/16)%16]);
            Serial.print(hex_charset[address%16]);
            Serial.print('=');
            Serial.print(hex_charset[data/16]);
            Serial.println(hex_charset[data%16]);
            EEPROM.write(address,data);
            Serial.println("Written to EEPROM.");
          }
            break;
            EEPROM_write_cancel:
            Serial.println();
            Serial.println("Cancelled");
            break;
        }
      }
      break;
    case 9:
      if (b[11]&&millis()-appeared>2000)
        set_screen(1);
      break;
    case 10:
      if (b[10])
        set_screen(1);
      break;
    case 11:
      if (millis()-appeared>2000) 
        set_screen(prev_scr);
      break;
    case 12:
      if (millis()-appeared>3000)
        set_screen(after_scr);
      break;
    case 13:
      if (b[10])
        set_screen(1);
      break;
    case 14:
      if (b[11]) {
        if (check_input(8))
          set_screen(17);
        else
          set_screen(3);
      } break;
    case 15:
    {
      byte stage=0,r=0,g=0,b=0;
      while (1) {
        for (byte tm=0;tm<end_lights_speed;tm++) {
          if (r!=0)
            digitalWrite(rled,0);
          if (g!=0)
            digitalWrite(gled,0);
          if (b!=0)
            digitalWrite(9,0);
          for (byte i=0;i<255;i++) {
            if (r<i)
              digitalWrite(rled,1);
            if (g<i)
              digitalWrite(gled,1);
            if (b<i)
              digitalWrite(9,1);
            delayMicroseconds(1);
          }
        }
        switch (stage) {
          case 0: // red on
            r++;
            if (r==254)
              stage++;
            break;
          case 1: // green on
            g++;
            if (g==254)
              stage++;
            break;
          case 2: // red off
            r--;
            if (r==0)
              stage++;
            break;
          case 3: // blue on
            b++;
            if (b==254)
              stage++;
            break;
          case 4: // green off
            g--;
            if (g==0)
              stage++;
            break;
          case 5: // red on
            r++;
            if (r==254)
              stage++;
            break;
          case 6: // blue off
            b--;
            if (b==0)
              stage=1;
            break;
        }
      }
    }
      break;
    case 16:
      if (b[11]) {
        if (check_input(4))
          set_screen(19);
        else
          set_screen(3);
      } break;
  }

  switch (scr_type[screen][0]) {
    case 1:
      if (pos>0&&(b[2]||b[4])){
        pos--;
        update_menu();
      } else if (pos<sz-1&&(b[8]||b[6])) {
        pos++;
        update_menu();
      }
      if (b[11])
        if (is_unlocked())
          set_screen(scr_m[scr_type[screen][1]+scr_m[scr_type[screen][1]]*2+2+pos]);
        else {
          lcd.setCursor(0,0);
          lcd.setCursor(0,0);
          lcd.print("Locked!      ");
          menu_locked = true;
        }
      break;
    case 2:
      if (scr_type[screen][1]>pos)
        for (byte i=0;i<10;i++)
          if (b[i]&&scr_type[screen][1]>pos) {
              lcd.print(i);
              input[pos] = '0'+i;
              pos++;
            }
      if (pos>0&&b[10]) {
        lcd.setCursor(--pos,1);
        input[pos] = 0;
        lcd.print('_');
        lcd.setCursor(pos,1);
      } break;
    case 3:
      if (b[2]&&pos/16!=0)
        game1_player_move(pos%16,pos/16-1);
      if (b[8]&&pos/16!=1)
        game1_player_move(pos%16,pos/16+1);
      if (b[4]&&pos%16!=0)
        game1_player_move(pos%16-1,pos/16);
      if (b[6]&&pos%16!=15)
        game1_player_move(pos%16+1,pos/16);
      
      if (game_tick==game_speed) {
        level_update_map[0][0] = B11111111;
        level_update_map[0][1] = B11111111;
        level_update_map[1][0] = B11111111;
        level_update_map[1][1] = B11111111;
        //Serial.println("Updating ghosts");
        
        for (byte y=0;y<2;y++)
          for (byte x=0;x<16;x++)
            if (level[y][x]==2)
              //Serial.println(level_needs_update(x,y));
              if (level_needs_update(x,y)) {
                //Serial.println("Needs update");
                bool ghost_top    = y!=0  && (level[y-1][x]==32 || level[y-1][x]==1);
                bool ghost_bottom = y!=1  && (level[y+1][x]==32 || level[y+1][x]==1);
                bool ghost_left   = x!=0  && (level[y][x-1]==32 || level[y][x-1]==1);
                bool ghost_right  = x!=15 && (level[y][x+1]==32 || level[y][x+1]==1);
                if (ghost_top||ghost_bottom||ghost_left||ghost_right) {
                  //Serial.println("Found path");
                  bool work = true,found_player=false,shorter_path=false;
                  byte new_x,new_y;
                  if (random(4)) {
                    /*Serial.print(x);
                    Serial.print(',');
                    Serial.print(y);
                    Serial.println(" looking for player");*/
                    if (ghost_top && level[y-1][x]==1) {
                      found_player = true;
                      new_x=x;
                      new_y=y-1;
                    }
                    if (ghost_bottom && level[y+1][x]==1) {
                      found_player = true;
                      new_x=x;
                      new_y=y+1;
                    }
                    if (ghost_left && level[y][x-1]==1) {
                      found_player = true;
                      new_x=x-1;
                      new_y=y;
                    }
                    if (ghost_right && level[y][x+1]==1) {
                      found_player = true;
                      new_x=x+1;
                      new_y=y;
                    }/*
                    if (found_player)
                      Serial.println("Found");
                    else
                      Serial.println("Nope");*/
                  }
                  if (!found_player && random(5)) {
                    if (ghost_left && pos%16<x) {
                      shorter_path = true;
                      new_x=x-1;
                      new_y=y;
                    } else if (ghost_right && pos%16>x) {
                      shorter_path = true;
                      new_x=x+1;
                      new_y=y;
                    } else if (ghost_top && pos/16<y) {
                      shorter_path = true;
                      new_x=x;
                      new_y=y-1;
                    } else if (ghost_bottom && pos/16>y) {
                      shorter_path = true;
                      new_x=x;
                      new_y=y+1;
                    }
                  }
                  if (!found_player && !shorter_path)
                    while (work)
                      switch (random(4)) {
                        case 0:
                          if (ghost_top) {
                            new_x=x;
                            new_y=y-1;
                            work=false;
                          }break;
                        case 1:
                          if (ghost_bottom) {
                            new_x=x;
                            new_y=y+1;
                            work=false;
                          }break;
                        case 2:
                          if (ghost_left) {
                            new_x=x-1;
                            new_y=y;
                            work=false;
                          }break;
                        case 3:
                          if (ghost_right) {
                            new_x=x+1;
                            new_y=y;
                            work=false;
                          } break;
                      };
                  if (level[new_y][new_x]==1)
                    set_screen(11);
                  level[new_y][new_x] = 2;
                  level[y][x] = 32;
                  level_disable_update(new_x,new_y);
                }
              }
            
            //Serial.print(bitRead(level_update_map[y][x/8],x%8));
          
          /*Serial.print(' ');
          Serial.print(hex_charset[level_update_map[y][0]]/16);
          Serial.println(hex_charset[level_update_map[y][1]]%16);*/
        
        update_game1();
      }
      break;
  }
  
  reset_buttons();
  //delay();
}
