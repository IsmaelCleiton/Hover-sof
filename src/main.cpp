#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "sprites.h"
#include <DIYables_IRcontroller.h>
#define REMOTEXY_MODE__ESP8266WIFI_LIB_POINT
#include <ESP8266WiFi.h>
#include <RemoteXY.h>
#include <Ticker.h>
#define IR_RECEIVER_PIN D5
#define INA1 D0
#define INA2 D1
#define INB1 D2
#define INB2 D3
// WIFI
#define REMOTEXY_WIFI_SSID "martinha"
#define REMOTEXY_WIFI_PASSWORD "martinha"
#define REMOTEXY_SERVER_PORT 6377
// Distance Sensor
#define SOUND_VELOCITY 0.034
#define trigPin D10
#define echoPin D9
// Configuração do RemoteXY
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] = // 34 bytes
    {255, 2, 0, 0, 0, 27, 0, 17, 0, 0, 0, 31, 2, 106, 200, 200, 84, 1, 1, 1,
     0, 5, 35, 133, 40, 40, 154, 35, 28, 28, 42, 177, 24, 31};

// Definição das variáveis do aplicativo RemoteXY
struct
{

  int8_t joystick_1_x;  // de -100 a 100
  int8_t joystick_1_y;  // de -100 a 100
  uint8_t connect_flag; // =1 se conectado, se não =0

} RemoteXY;
#pragma pack(pop)

// Inicializa o display Oled
SSD1306Wire display(0x3c, D6, D7);
// Inicializa o Receptor Infra vermelho
DIYables_IRcontroller_21 irController(IR_RECEIVER_PIN, 200);
// Inicializa o flipper
Ticker flipper;
// Variaveis
bool _route[2] = {0, 0};
byte _speed[2] = {0, 0};
bool usingIR = false;
long duration;
float distanceCm;
int count = 0;
bool inGraph = false;

// Declarações das funções
void calcSpeed();
void getIR();
void moveRover();
void getEyes();
void carStop();
void getDistance();
void my_delay(uint32_t);
void flip();
void drawEyes(u_int8_t *, u_int8_t *);
void showDistance();
void showTitle();
void showTemp();
void showProgressBar();
void getIRtest();
void piscada();
void getEyes();
void carForward();
void carBackward();
void carLeft();
void carRight();
void carStop();
void carForwardLeft();
void carForwardRight();
void carBackwardLeft();
void carBackwardRight();

void setup()
{
  Serial.begin(115200);
  flipper.attach(1, flip);
  irController.begin();
  RemoteXY_Init();
  display.init();
  display.flipScreenVertically();
  pinMode(INA1, OUTPUT);
  pinMode(INA2, OUTPUT);
  pinMode(INB1, OUTPUT);
  pinMode(INB2, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(INA1, LOW);
  digitalWrite(INA2, LOW);
  digitalWrite(INB1, LOW);
  digitalWrite(INB2, LOW);
  drawEyes(sprites[0], sprites[1]);
}

void loop()
{
  if (RemoteXY.connect_flag)
  {
    RemoteXY_Handler();
    calcSpeed();
  }
  getIR();
  getEyes();
  // getDistance();
  //  if (distanceCm < 2 && distanceCm > 0)
  //  {
  //    carStop();
  //  }
  moveRover();
  if (count == 2)
  {
    piscada();
    flipper.attach(random(10), flip);
    count = 0;
  }
}

void flip()
{
  ++count;
}

void drawEyes(u_int8_t *e1, u_int8_t *e2)
{
  if (inGraph)
  {
    return;
  }
  display.clear();
  display.drawXbm(14, 16, 40, 48, e1);
  display.drawXbm(74, 16, 40, 48, e2);
  display.display();
}

void getDistance()
{
  digitalWrite(trigPin, LOW);
  my_delay(2);
  digitalWrite(trigPin, HIGH);
  my_delay(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_VELOCITY / 2;
}

void showDistance()
{
  inGraph = true;
  getDistance();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.println("Distancia(cm): ");
  display.println(distanceCm);
  display.display();
}

void my_delay(uint32_t ms)
{
  uint32_t t = millis();
  while (true)
  {

    if (RemoteXY.connect_flag)
    {
      RemoteXY_Handler();
      calcSpeed();
    }
    getIR();
    moveRover();
    getEyes();
    ::delay(1); // if does not use the delay it does not work on ESP8266
    if (millis() - t >= ms)
      break;
  }
}

void carForward()
{
  _route[0] = 0;
  _speed[0] = 255;
  _route[1] = 0;
  _speed[1] = 255;
}

void carBackward()
{
  _route[0] = 1;
  _speed[0] = 255;
  _route[1] = 1;
  _speed[1] = 255;
}

void carLeft()
{
  _route[0] = 0;
  _speed[0] = 255;
  _route[1] = 1;
  _speed[1] = 255;
}

void carRight()
{
  _route[0] = 1;
  _speed[0] = 255;
  _route[1] = 0;
  _speed[1] = 255;
}

void carStop()
{
  _speed[0] = 0;
  _speed[1] = 0;
}

void carForwardLeft()
{
  _route[0] = 0;
  _speed[0] = 255;
  _route[1] = 0;
  _speed[1] = 0;
}

void carForwardRight()
{
  _route[0] = 0;
  _speed[0] = 0;
  _route[1] = 0;
  _speed[1] = 255;
}

void carBackwardLeft()
{
  _route[0] = 1;
  _speed[0] = 255;
  _route[1] = 1;
  _speed[1] = 0;
}

void carBackwardRight()
{
  _route[0] = 1;
  _speed[0] = 0;
  _route[1] = 1;
  _speed[1] = 255;
}

void moveRover()
{
  if (_route[0] == 0)
  {
    digitalWrite(INB1, _speed[0]);
    digitalWrite(INB2, LOW);
  }
  else
  {
    digitalWrite(INB1, LOW);
    digitalWrite(INB2, _speed[0]);
  }
  if (_route[1] == 0)
  {
    digitalWrite(INA1, _speed[1]);
    digitalWrite(INA2, LOW);
  }
  else
  {
    digitalWrite(INA1, LOW);
    digitalWrite(INA2, _speed[1]);
  }
}

void calcSpeed()
{
  if (RemoteXY.joystick_1_y > 50 && RemoteXY.joystick_1_x < 50 && RemoteXY.joystick_1_x > -50)
  { // forward
    usingIR = false;
    carForward();
  }
  else if (RemoteXY.joystick_1_y < -50 && RemoteXY.joystick_1_x < 50 && RemoteXY.joystick_1_x > -50)
  { // backward
    usingIR = false;
    carBackward();
  }
  else if (RemoteXY.joystick_1_x < -50 && RemoteXY.joystick_1_y < 50 && RemoteXY.joystick_1_y > -50)
  { // left
    usingIR = false;
    carLeft();
  }
  else if (RemoteXY.joystick_1_x > 50 && RemoteXY.joystick_1_y < 50 && RemoteXY.joystick_1_y > -50)
  { // right
    usingIR = false;
    carRight();
  }
  else if (RemoteXY.joystick_1_y > 50 && RemoteXY.joystick_1_x < -50)
  { // forward left
    usingIR = false;
    carForwardLeft();
  }
  else if (RemoteXY.joystick_1_y > 50 && RemoteXY.joystick_1_x > 50)
  { // forward right
    usingIR = false;
    carForwardRight();
  }
  else if (RemoteXY.joystick_1_y < -50 && RemoteXY.joystick_1_x < -50)
  { // backward left
    usingIR = false;
    carBackwardLeft();
  }
  else if (RemoteXY.joystick_1_y < -50 && RemoteXY.joystick_1_x > 50)
  { // backward right
    usingIR = false;
    carBackwardRight();
  }
  else
  {
    if (!usingIR)
    {
      carStop();
    }
  }
}

void showTitle()
{
  inGraph = true;
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(63, 10, "NodeMCU");
  display.drawString(63, 26, "ESP8266");
  display.drawString(63, 45, "Display Oled");
  display.display();
}

void showTemp()
{
  inGraph = true;
  randomSeed(analogRead(D4));
  int temp = random(-125, 22);
  for (int counter = 0; counter <= 100; counter++)
  {
    display.clear();
    // Desenha a barra de progresso
    display.drawProgressBar(0, 32, 120, 10, counter);
    // Atualiza a porcentagem completa
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(counter) + "%");
    display.display();
    my_delay(2);
  }
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(63, 2, "Temperatura em Marte");
  display.setFont(ArialMT_Plain_16);
  display.drawString(63, 26, "=== " + String(temp) + " C° ===");
  display.drawString(63, 45, "--------------------");
  display.display();
}

void showProgressBar()
{
  inGraph = true;
  for (int counter = 0; counter <= 100; counter++)
  {
    display.clear();
    // Desenha a barra de progresso
    display.drawProgressBar(0, 32, 120, 10, counter);
    // Atualiza a porcentagem completa
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(counter) + "%");
    display.display();
    my_delay(10);
  }
  inGraph = false;
}

void getIRtest()
{
  inGraph = true;
  while (inGraph)
  {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.println("TEST ");
    display.println(irController.loop());
    display.display();
    my_delay(1);
  }
}

void getIR()
{
  Key21 key = irController.getKey();
  if (key != Key21::NONE)
  {
    usingIR = true;
    switch (key)
    {
    case Key21::KEY_CH_MINUS:
      getIRtest();
      break;

    case Key21::KEY_CH:
      showDistance();
      break;

    case Key21::KEY_CH_PLUS:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_PREV:
      carLeft();
      break;

    case Key21::KEY_NEXT:
      carRight();
      break;

    case Key21::KEY_PLAY_PAUSE:
      carStop();
      usingIR = false;
      break;

    case Key21::KEY_VOL_MINUS:
      carBackward();
      break;

    case Key21::KEY_VOL_PLUS:
      carForward();
      break;

    case Key21::KEY_EQ:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_100_PLUS:
      inGraph = false;
      break;

    case Key21::KEY_200_PLUS:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_0:
      showTitle();
      break;

    case Key21::KEY_1:
      inGraph = true;
      display.clear();
      display.drawXbm(14, 16, 40, 48, sprites[16]);
      display.drawXbm(74, 16, 40, 48, sprites[17]);
      display.display();
      break;

    case Key21::KEY_2:
      showProgressBar();
      break;

    case Key21::KEY_3:
      showTemp();
      break;

    case Key21::KEY_4:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_5:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_6:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_7:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_8:
      // TODO: YOUR CONTROL
      break;

    case Key21::KEY_9:
      // TODO: YOUR CONTROL
      break;

    default:
      break;
    }
  }
}

void piscada()
{
  if (inGraph)
  {
    return;
  }
  inGraph = true;
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[0]);
  display.drawXbm(74, 16, 40, 48, sprites[1]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[4]);
  display.drawXbm(74, 16, 40, 48, sprites[5]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[6]);
  display.drawXbm(74, 16, 40, 48, sprites[7]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[8]);
  display.drawXbm(74, 16, 40, 48, sprites[9]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[10]);
  display.drawXbm(74, 16, 40, 48, sprites[11]);
  display.display();
  my_delay(500);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[8]);
  display.drawXbm(74, 16, 40, 48, sprites[9]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[6]);
  display.drawXbm(74, 16, 40, 48, sprites[7]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[4]);
  display.drawXbm(74, 16, 40, 48, sprites[5]);
  display.display();
  my_delay(200);
  display.clear();
  display.drawXbm(14, 16, 40, 48, sprites[0]);
  display.drawXbm(74, 16, 40, 48, sprites[1]);
  display.display();
  inGraph = false;
}

void getEyes()
{
  if (_speed[0] == 0 & _speed[1] == 0)
  {
    drawEyes(sprites[0], sprites[1]); // parado
    return;
  }
  if (_route[0])
  {
    if (_route[1])
    {
      drawEyes(sprites[22], sprites[23]); // backward
      return;
    }
    else
    {
      drawEyes(sprites[2], sprites[3]); // right
      return;
    }
  }
  else
  {
    if (_route[1])
    {
      drawEyes(sprites[20], sprites[21]); // left
      return;
    }
    else
    {
      drawEyes(sprites[12], sprites[13]); // forward
      return;
    }
  }
}
