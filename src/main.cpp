#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <WiFi.h>
#include <PubSubClient.h>

using namespace std;

/*
TODO LIST:
Atribuicao automatica de ID distribuidos pelo MESTRE
Funcao de tratamento dos dados recebidos via mqtt
*/

// Definicao dos pinos utilizados
#define BT1 4    // Botao 1(Incremento)
#define BT2 13   // Botao 2(Decremento)
#define SWT 5    // Alavanca para alternar entre selecao de campo e selecao de umidade
#define U0_TXD 1 // Pino TX da serial
#define U0_RXD 3 // Pino RX da serial

// Definicao dos parametros do MQTT
#define BROKER "broker.hivemq.com"
#define PORT 1883
#define TUMIDADE "Umidade"
#define TCAMPO "Campo"
#define TPUMIDADE "PUmidade"

// Definicao dos parametros de rede
#define SSID "TP-Link"
#define PASS "lari2404"

//Definicao do intervalo de tempo entre as comunicacoes
#define INTCOM 500

// Variaveis
String msg;
float PUmidade;
float media;
int stateSWT;
LiquidCrystal_I2C lcd(0x27, 20, 4);
int j; // J indica o numero do campo
WiFiClient wifiCliente;
PubSubClient cliente;
bool jaEnviei = false;
bool enviadoBroker = false;
int t1, t2;

// Funcao de comunicação serial
void slaveCOM(int const slave, String msg, float const PUmidade)
{
  char c;
  // Procura o escravo para solicitar inforcao
  if (Serial.availableForWrite() > 0)
  {
    Serial.println(slave);
  }

  // Verifica se houve resposta na serial
  if (Serial.available() > 0)
  {
    msg = "";
    // Quando a informacao e recebida
    while (Serial.available() > 0)
    {
      c = (char)Serial.read();
      msg.concat(c);
    }
  }
}

// TODO: Funcao de tratamento dos dados recebidos via mqtt
void callback(const char *topic, byte *payload, unsigned int length)
{
  // Variaveis auxiliares para o recebimento das informacoes via mqtt
  String aux = "";   // Guarda as substrings
  String param = ""; // Guarda os parametros a serem alterados
  String value = ""; // Guarda os valores a serem alterados

  // Percorre o buffer para recebimento da mensagem - salva na variavel interna aux
  for (int i = 0; i < length; i++)
  {
    aux += (char)payload[i];
  }

  if (topic == TCAMPO)
  {
    if (aux.toInt() > 0)
    {
      j = aux.toInt();
      jaEnviei = false;
    }
  }
}

void setup()
{

  // Iniciar a serial e definir os pinos usados na transmicao serial
  Serial.begin(9600);
  pinMode(U0_TXD, INPUT_PULLUP);
  pinMode(U0_RXD, OUTPUT);

  // Tempo de intervalo
  delay(10);

  // Iniciar conexao wifi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);

  // Verificar conexao wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Exibir mensagem de sucesso
  Serial.println("");
  Serial.println("WiFi connected.");

  // Conectar ao HiveMQ
  cliente.setClient(wifiCliente);
  cliente.setServer(BROKER, PORT);
  cliente.connect("Esp32Master", "rega", "gotejamento");
  cliente.subscribe(TUMIDADE);
  cliente.subscribe(TCAMPO);
  cliente.subscribe(TPUMIDADE);
  cliente.setCallback(callback);

  // Iniciar variaveis
  j = 1;
  PUmidade = 50;
  media = 50;
  msg = "";

  // Definir os pinos
  pinMode(BT1, INPUT_PULLDOWN);
  pinMode(BT2, INPUT_PULLDOWN);
  pinMode(SWT, INPUT);

  // Iniciar o LCD
  lcd.init();
  lcd.begin(20, 4, 0);
  lcd.backlight();

  //Iniciar conometro para envio e recebimento dos dados da serial e envio ao broker
  t1 = millis();
}

void loop()
{
  t2 = millis();
  if ((t2 - t1) > INTCOM)
  {
    jaEnviei = false
    t1 = millis();
  }
  
  // Limpar mensagem
  msg = "";

  cliente.loop();

  if (!jaEnviei)
  {
    // Efetuar comunicacao com o escravo responsavel pelo campo selecionado
    slaveCOM(j, msg, PUmidade);
    Serial.println(msg);
    jaEnviei = true;
    enviadoBroker = false;
  }

  if (!enviadoBroker)
  {
    cliente.publish(TUMIDADE, msg.c_str());
    enviadoBroker = true;
  }

  // Verificar o estado da chave de selecao
  stateSWT = digitalRead(SWT);
  if (stateSWT == 1)
  {
    // Selecao de campo
    if (digitalRead(BT1) == 1)
    {
      j++;
    }
    if (digitalRead(BT2) == 1)
    {
      j--;
    }
  }
  else
  {
    // Controle do parametro umidade
    if (digitalRead(BT1) == 1)
    {
      PUmidade++;
    }
    if (digitalRead(BT2) == 1)
    {
      PUmidade--;
    }
  }

  // Limpar o LCD e exibir a mensagem
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  lcd.print(msg);
  // Serial.println(msg);
}