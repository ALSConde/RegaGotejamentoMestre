#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <WiFi.h>
#include <PubSubClient.h>

using namespace std;

/*
TODO LIST:
Atribuicao de ID aos dispositivos MESTRE, para uso no mqtt
Atribuicao automatica de ID distribuidos pelo MESTRE
Funcao de tratamento dos dados recebidos via mqtt
*/

// Definicao do ID do mestre
#define ID 1

// Definicao dos pinos utilizados
#define BT1 4
#define BT2 13
#define SWT 5

// Definicao dos parametros de rede
#define SSID "Tp-link"
#define PASS "lari2404"

// Variaveis
String msg;
float PUmidade;
float media;
int stateSWT;
LiquidCrystal_I2C lcd(0x27, 20, 4);
int j; // J indica o numero do campo
WiFiClient wifiCliente;
PubSubClient cliente;

// Funcao de comunicação serial
void slaveCOM(int const slave, String msg, float const PUmidade)
{

  // Procura o escravo para solicitar inforcao
  Serial.write(slave);
  Serial.print(PUmidade);
  // Quando a informacao e recebida
  if (Serial.available())
  {
    // Atualiza a mensagem
    msg = Serial.readString();
  }
}

// TODO: Funcao de tratamento dos dados recebidos via mqtt
void callback(const char *topic, byte *payload, unsigned int length)
{
  String aux = "";
  String id = "";
  String param = "";
  String value = "";
  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  aux = msg.substring(0, msg.indexOf(","));
  id = aux.substring(aux.indexOf(":") + 1, aux.indexOf(","));
  msg = msg.substring(msg.indexOf(",") + 1, length);
  if (ID == id.toInt())
  {
    aux = "";
    aux = msg.substring(0, msg.indexOf(","));
    param = aux.substring(0, aux.indexOf(":"));
    msg = msg.substring(msg.indexOf(":") + 1, length);

    if (param.compareTo("Umidade") == 0)
    {
      value = "";
      value = msg;
      PUmidade = value.toDouble();
    }

    aux = "";
    aux = msg.substring(0, msg.indexOf(","));
    param = aux.substring(0, aux.indexOf(":"));
    msg = msg.substring(msg.indexOf(":") + 1, length);

    if (param.compareTo("Campo") == 0)
    {
      value = "";
      value = msg;
      j = value.toInt();
    }
  }
  delay(750);
}

void setup()
{

  // Iniciar a Serial
  Serial.begin(9600);

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
  cliente.setServer("broker.hivemq.com", 1883);
  cliente.connect("Esp32Master", "rega", "gotejamento");
  cliente.subscribe("testeESP32");
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
  pinMode(LED, OUTPUT);

  // Iniciar o LCD
  lcd.init();
  lcd.begin(20, 4, 0);
  lcd.backlight();
}

void loop()
{
  delay(750);

  // Limpar mensagem
  msg = "";

  cliente.loop();

  // Efetuar comunicacao com o escravo responsavel pelo campo selecionado
  slaveCOM(j, msg, PUmidade);

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
    Serial.println("Campo: " + String(j));
    msg = ("Campo: " + String(j));
    // cliente.publish("testeESP32", msg.c_str());
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

    Serial.println("Parametro: " + String(PUmidade));
    msg = ("Parametro: " + String(PUmidade));
    cliente.publish("testeESP32", msg.c_str());
  }

  // Limpar o LCD e exibir a mensagem
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  lcd.print(msg);
  Serial.println(msg);
  delay(750);
}