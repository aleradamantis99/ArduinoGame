#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <EEPROM.h>
#include "Image.h"
#define TFT_CS         9
#define TFT_RST        10
#define TFT_DC         8
#define TFT_DIN        11
#define TFT_CLK        13 

#define EjeX A5
#define EjeY A4
#define Switch A3

#define ACTORRAD 7
#define BULLETRAD 3
#define HISCOREADDR 24
unsigned genRand_()
{
  unsigned byteRand = (analogRead(0)&1) << 7;
  byteRand |= (analogRead(0)&1) << 6;
  byteRand |= (analogRead(0)&1) << 5;
  byteRand |= (analogRead(0)&1) << 4;
  byteRand |= (analogRead(0)&1) << 3;
  byteRand |= (analogRead(0)&1) << 2;
  byteRand |= (analogRead(0)&1) << 1;
  byteRand |= (analogRead(0)&1);
  return byteRand;
}
//Returns a number in range [1, 255]
unsigned genRand()
{
  
  unsigned byteRand;
  do{
     byteRand = genRand_();
  } while(byteRand == 0);
  return byteRand;
}
// Range [0, max(upper, 254)]
unsigned genRand(unsigned min, unsigned upper)
{
  return map(genRand()-1, 0, 255, min, upper);
}
struct Vect2d
{
  int x;
  int y;
  /*Vect2d() = default;
  Vect2d(int x_, int y_): x(x_), y(y_){}*/
  float setModule(unsigned module)
  {
    float currentModule = sqrt((float)(x*x + y*y));
    float xf = ((float)x / currentModule)*(float)module;
    float yf = ((float)y / currentModule)*(float)module;
    x = round(xf);
    y = round(yf);
  }
  Vect2d& operator*=(int n)
  {
    x*=n;
    y*=n;
    return *this;
  }
  Vect2d& operator/=(int n)
  {
    x/=n;
    y/=n;
    return *this;
  }
};

float distance(struct Vect2d a, struct Vect2d b)
{
  return sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
}

struct Entity
{
  Vect2d pos;
  Vect2d speed;
  void advance()
  {
    pos.x += speed.x;
    pos.y += speed.y;
  }
};

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
float p = 3.1415926;
unsigned score = 0;
bool ismenu = true;
void (*reset_func)(void) = 0;

Entity* bullet = NULL;
unsigned x = tft.width()/2;
unsigned y = tft.height()/2;
Vect2d facing;
unsigned nEnemies = 2;
Entity enemies[6];
unsigned currentMenu = 0;
const unsigned MAXMOVE = 4;
const unsigned MINMOVE = 1;
const unsigned UPBOUND = 600;
const unsigned LOWBOUND = 300;

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(9600);
  pinMode(A3, INPUT_PULLUP);
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

  Serial.println(F("Initialized"));

  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  Serial.println(time, DEC);

  // a single pixel
  tft.fillCircle(tft.width()/2, tft.height()/2, ACTORRAD, ST77XX_GREEN);
  menu();
  
  Serial.println("done");
}

bool is_out_display(struct Vect2d pos)
{
  return pos.x<0 || pos.x > tft.width() || pos.y<0 || pos.y > tft.height();
}

void loop() {
  //tft.fillScreen(ST77XX_BLACK);
  unsigned prevX = x;
  unsigned prevY = y;
  unsigned moveX;
  moveX = analogRead(EjeX);
  unsigned moveY;
  moveY = analogRead(EjeY);
  int press = digitalRead(A3);
  if (ismenu)
  {
    menu_control(moveX, moveY, press);
    delay(100);
    return;
  }
  draw_enemies();
  Vect2d newfacing{0,0};
  collisions();
  if (press == LOW and bullet == NULL)
  {
    bullet = new Entity();
    bullet->pos.x = x;
    bullet->pos.y = y;
    bullet->speed.x = 5*facing.x;
    bullet->speed.y = 5*facing.y;
    tft.fillCircle(x, y, BULLETRAD, ST77XX_BLUE);
  }
  else if (bullet != NULL)
  {
    tft.fillCircle(bullet->pos.x, bullet->pos.y, 3, ST77XX_BLACK);
    bullet->advance();
    if (is_out_display(bullet->pos))
    {
      delete bullet;
      bullet = NULL;
    }
    else
    {
      tft.fillCircle(bullet->pos.x, bullet->pos.y, 3, ST77XX_BLUE);
    }
  }
  if (moveY > UPBOUND)
  {
    //move arriba
    Serial.println("Up");
    newfacing.y = 1;
    y += map(moveY, UPBOUND, 1021, MINMOVE, MAXMOVE);
  }
  else if (moveY < LOWBOUND)
  {
    //move abajo
    Serial.println("Right");
    newfacing.y = -1;
    y-= map(moveY, LOWBOUND, 0, MINMOVE, MAXMOVE);
  }
  
  if (moveX > UPBOUND)
  {
    //move arriba
    
     Serial.println("Down");
    newfacing.x = 1;
    x += map(moveX, UPBOUND, 1021, MINMOVE, MAXMOVE);
  }
  else if (moveX < LOWBOUND)
  {
    //move abajo
    Serial.println("Left");
    newfacing.x = -1;
    x -= map(moveX, LOWBOUND, 0, MINMOVE, MAXMOVE);
  }
  if (x != prevX || y != prevY)
  {
    float ang = atan2(facing.y, facing.x);
    tft.drawLine(prevX, prevY, 1000*cos(ang), 1000*sin(ang), ST77XX_BLACK);
    facing= newfacing;
    ang = atan2(facing.y, facing.x);
    tft.drawLine(x, y, 1000*cos(ang), 1000*sin(ang), ST77XX_RED);
    tft.fillCircle(prevX, prevY, ACTORRAD, ST77XX_BLACK);
  }
  float ang = atan2(facing.y, facing.x);
  tft.drawLine(x, y, 1000*cos(ang), 1000*sin(ang), ST77XX_RED);
  tft.fillCircle(x, y, ACTORRAD, ST77XX_GREEN);
  delay(100);
}

void gameOver()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_RED);
  tft.setTextWrap(true);
  tft.setTextSize(3);

  tft.print("Game Over\nScore:");
  tft.print(score);
  int hiscore = EEPROM.read(HISCOREADDR);
  if (score > hiscore || hiscore == 255)
  {
    EEPROM.write(HISCOREADDR, score);
  }
  tft.print("HiScore:");
  tft.print(hiscore);
  delay(2000);
  reset_func();
}

void collisions()
{
  for (int i=0; i<nEnemies; i++)
  {
    if (distance(enemies[i].pos, Vect2d{x,y}) <= ACTORRAD*2)
    {
      gameOver();
      reset_func();
    }
  }
  if (bullet != NULL)
  {
    
    for (int i=0; i<nEnemies; i++)
    {
      if (distance(enemies[i].pos, bullet->pos) <= ACTORRAD+BULLETRAD){
        tft.fillCircle(bullet->pos.x, bullet->pos.y, BULLETRAD, ST77XX_BLACK);
        delete bullet;
        bullet = NULL;
        score++;
        tft.fillCircle(enemies[i].pos.x, enemies[i].pos.y, ACTORRAD, ST77XX_BLACK);
        do
        {
          enemies[i].pos.x = genRand(0, tft.width()-1);
          enemies[i].pos.y = genRand(0, tft.height()-1);
        }while (not is_fair(enemies[i].pos));
      }
    }
  }

  
}
void draw_enemies()
{
  for (int i=0; i<nEnemies; i++)
  {
    Entity& en = enemies[i];
    tft.fillCircle(en.pos.x, en.pos.y, ACTORRAD, ST77XX_BLACK);
    en.advance();
    
    tft.fillCircle(en.pos.x, en.pos.y, ACTORRAD, ST77XX_RED);
    if (is_out_display(en.pos))
    {
      en.speed *= -1;
    }
  }
  if (nEnemies == 5)
  {
    Entity& ensp = enemies[4];
    ensp.speed = Vect2d{x-ensp.pos.x, y-ensp.pos.y};
    ensp.speed.setModule(2);
  }
}

/*Moved = 0, no move
  Moved = 1, move up
  Moved = -1, move down
*/
void menu()
{
  tft.fillTriangle(10, 13, 10, 37, 20, 25, ST77XX_GREEN);
  tft.fillRoundRect(25, 10, 78, 30, 8, ST77XX_WHITE);
  tft.drawRGBBitmap(64-8, 25-8, image_data_Skull, 16, 16);

  tft.fillRoundRect(25, 50, 78, 30, 8, ST77XX_WHITE);
  tft.drawRGBBitmap(51-8, 65-8, image_data_Skull, 16, 16);
  tft.drawRGBBitmap(77-8, 65-8, image_data_Skull, 16, 16);
  
  tft.fillRoundRect(25, 90, 78, 30, 8, ST77XX_WHITE);
  tft.drawRGBBitmap(44-8, 105-8, image_data_Skull, 16, 16);
  tft.drawRGBBitmap(63-8, 105-8, image_data_Skull, 16, 16);
  tft.drawRGBBitmap(82-8, 105-8, image_data_Skull, 16, 16);
}

void update_menu(unsigned newMenu)
{
  tft.fillTriangle(10, 13, 10, 37, 20, 25, ST77XX_BLACK);
   tft.fillTriangle(10, 53, 10, 77, 20, 65, ST77XX_BLACK);
   tft.fillTriangle(10, 93, 10, 117, 20, 105, ST77XX_BLACK);
  if (newMenu == 0)
  {
    tft.fillTriangle(10, 13, 10, 37, 20, 25, ST77XX_GREEN);
  }
  else if(newMenu == 1)
  {
    tft.fillTriangle(10, 53, 10, 77, 20, 65, ST77XX_GREEN);
  }
  else
  {
    tft.fillTriangle(10, 93, 10, 117, 20, 105, ST77XX_GREEN);
  }
  currentMenu = newMenu;
}
void menu_control(unsigned moveX, unsigned moveY, int press)
{
  if (press == LOW)
  {
      tft.fillScreen(ST77XX_BLACK);
      ismenu = false;
      setup_game();
      return;
  }
  int move = 0;
  if (moveY > UPBOUND)
  {
    //move arriba
    move = 1;
    Serial.println("Up");
  }
  else if (moveY < LOWBOUND)
  {
    //move abajo
    move = -1;
    Serial.println("Right");
  }
  if (move != 0)
  {
    update_menu((currentMenu + move)%3);
  }
}
void place_enemies()
{
  if (currentMenu == 0)
  {
    nEnemies = 2;
    enemies[0].speed = Vect2d{1, 0};
    enemies[1].speed = Vect2d{0, 2};
  }
  else if(currentMenu == 1)
  {
    nEnemies = 3;
    enemies[0].speed = Vect2d{1, 0};
    enemies[1].speed = Vect2d{0, 2};
    enemies[2].speed = Vect2d{1, 1};
  }
  else if(currentMenu == 2)
  {
    nEnemies = 5;
    enemies[0].speed = Vect2d{1, 0};
    enemies[1].speed = Vect2d{0, 2};
    enemies[2].speed = Vect2d{3, 2};
    enemies[3].speed = Vect2d{2, 0};
    enemies[4].speed = Vect2d{0, 1};
    //enemies[5].speed = Vect2d{3, 3};
  }
  for (int i=0; i< nEnemies; i++)
  {
    do
    {
      enemies[i].pos.x = genRand(0, tft.width()-1);
      enemies[i].pos.y = genRand(0, tft.height()-1);
    }while (not is_fair(enemies[i].pos));
    Serial.println("Added enemy");
  }
  
}

bool is_fair(Vect2d pos)
{
  return (distance(pos, Vect2d{x, y}) > 2*ACTORRAD+5);
}
void setup_game()
{
  facing.x = 1;
  facing.y = 0;
  Serial.println(nEnemies);
  place_enemies();
  
}
