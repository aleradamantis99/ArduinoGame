#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>


#define TFT_CS         9
#define TFT_RST        10
#define TFT_DC         8
#define TFT_DIN        11
#define TFT_CLK        13 

#define EjeX A5
#define EjeY A4
#define Switch A3

struct Vect2d
{
  int x;
  int y;
  /*Vect2d() = default;
  Vect2d(int x_, int y_): x(x_), y(y_){}*/
};

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


Entity* bullet = NULL;
unsigned x = tft.width()/2;
unsigned y = tft.height()/2;
Vect2d facing;
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
  tft.fillCircle(tft.width()/2, tft.height()/2, 7, ST77XX_GREEN);
  facing.x = 1;
  facing.y = 0;
  Serial.println("done");
}

bool is_out_display(Vect2d pos)
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
  Vect2d newfacing{0,0};
  if (press == LOW and bullet == NULL)
  {
    bullet = new Entity();
    bullet->pos.x = x;
    bullet->pos.y = y;
    bullet->speed.x = 5*facing.x;
    bullet->speed.y = 5*facing.y;
    tft.fillCircle(x, y, 3, ST77XX_BLUE);
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
    tft.fillCircle(prevX, prevY, 7, ST77XX_BLACK);
  }
  float ang = atan2(facing.y, facing.x);
  tft.drawLine(x, y, 1000*cos(ang), 1000*sin(ang), ST77XX_RED);
  tft.fillCircle(x, y, 7, ST77XX_GREEN);
  
  delay(100);
}

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
  do
  {
     byteRand = genRand();
  } while (byteRand==0);
  return byteRand;
}
