#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

using namespace std;

/*
TODO LIST:
Atribuicao automatica de ID distribuidos pelo MESTRE
*/

// Definicao dos pinos utilizados
#define BT1 4     // Botao 1(Incremento)
#define BT2 13    // Botao 2(Decremento)
#define SWT 5     // Alavanca para alternar entre selecao de campo e selecao de umidade
#define U0_TXD 17 // Pino TX da serial
#define U0_RXD 16 // Pino RX da serial

// Definicao dos parametros do MQTT
#define BROKER "broker.hivemq.com"
#define PORT 1883
#define TUMIDADE "Umidade"
#define TCAMPO "Campo"
#define TPUMIDADE "PUmidade"

// Definicao dos parametros de rede
#define SSID "*****"
#define PASS "*****"

// Definicao do intervalo de tempo entre as comunicacoes
#define INTCOM 2500

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
SoftwareSerial slaveCOM(U0_RXD, U0_TXD);
int t1, t2, t3, t4;

// Funcao de comunicação serial
void slaveCOMEnv(int const slave, float const PUmidade);
void slaveCOMRec();
void leituraBotao();

// TODO: Funcao de tratamento dos dados recebidos via mqtt
void callback(const char *topic, byte *payload, unsigned int length);

void setup()
{

  // Iniciar a serial e definir os pinos usados na transmicao serial
  Serial.begin(9600);
  slaveCOM.begin(9600, SWSERIAL_8N1);
  slaveCOM.enableTx(true);
  slaveCOM.enableRx(true);

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
  attachInterrupt(digitalPinToInterrupt(BT1), leituraBotao, HIGH);
  attachInterrupt(digitalPinToInterrupt(BT2), leituraBotao, HIGH);

  // Iniciar o LCD
  lcd.init();
  lcd.begin(20, 4, 0);
  lcd.backlight();

  // Iniciar cronometro para envio e recebimento dos dados da serial e envio ao broker
  t1 = millis();
  // Iniciar o cronometro para o intervalo entre as acoes dos botoes
  t3 = millis();
}

void loop()
{

  // Repeticao da conexao com o broker
  cliente.loop();

  t2 = millis();
  if ((t2 - t1) > INTCOM)
  {

    // Definir a flag de recebimento dos dados do escravo como falso
    jaEnviei = false;

    if (!jaEnviei)
    {
      if (slaveCOM.availableForWrite() > 0)
      {
        // Efetuar comunicacao com o escravo responsavel pelo campo selecionado
        slaveCOMEnv(j, PUmidade);
        if (slaveCOM.available() > 0)
        {
          slaveCOMRec();
        }
      }
      Serial.println(msg);
      jaEnviei = true;
      enviadoBroker = false;
    }

    if (!enviadoBroker)
    {
      cliente.publish(TUMIDADE, msg.c_str());
      enviadoBroker = true;
    }

    // Limpar o LCD e exibir a mensagem
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.backlight();
    lcd.print(msg);
    Serial.println(msg);

    t1 = millis();
  }
}

void leituraBotao()
{
  // Atualizar o tempo
  t4 = millis();

  if (t4 - t3 > 250)
  {
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
      if (digitalRead(BT1) == 1 && PUmidade < 100)
      {
        PUmidade++;
      }
      if (digitalRead(BT2) == 1 && PUmidade > 0)
      {
        PUmidade--;
      }
    }

    // Atualizar o tempo
    t3 = millis();
  }
}

void slaveCOMEnv(int const slave, float const PUmidade)
{
  String aux = "Id:" + String(slave) + ",Parametro:" + PUmidade + ";";

  // Procura o escravo para solicitar inforcao
  if (slaveCOM.availableForWrite() > 0)
  {
    slaveCOM.print(aux.c_str());
  }
}

void slaveCOMRec()
{
  char c;

  // Verifica se houve resposta na serial
  if (slaveCOM.available() > 0)
  {
    msg = "";
    // Quando a informacao e recebida
    while (slaveCOM.available() > 0)
    {
      c = (char)slaveCOM.read();
      msg.concat(c);
      if (c == ';')
      {
        break;
      }
    }
  }
}

void callback(const char *topic, byte *payload, unsigned int length)
{
  // Variavel auxiliar para o recebimento das informacoes via mqtt
  String aux = ""; // Guarda as substrings

  // Percorre o buffer para recebimento da mensagem - salva na variavel interna aux
  for (int i = 0; i < length; i++)
  {
    aux += (char)payload[i];
  }

  if (strcmp(topic, TCAMPO) == 0)
  {
    if (aux.toInt() > 0)
    {
      j = aux.toInt();
      jaEnviei = false;
    }
  }
  if (strcmp(topic, TPUMIDADE) == 0)
  {
    if (aux.toFloat() >= 0 && aux.toFloat() <= 100)
    {
      PUmidade = aux.toFloat();
      jaEnviei = false;
    }
  }
}