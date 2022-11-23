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
Funcao de tratamento dos dados recebidos via mqtt
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
#define SSID "TP-Link"
#define PASS "lari2404"

// Definicao do intervalo de tempo entre as comunicacoes
#define INTCOM 2500

// Variaveis
String msg;
float PUmidade;
float media;
int stateSWT; // Estado da alavanca, 1 - Selecao de campo, 0 - mudanca do parametro umidade
LiquidCrystal_I2C lcd(0x27, 20, 4);
int j; // J indica o numero do campo
WiFiClient wifiCliente;
PubSubClient cliente;
bool enviadoEscravo = false;
bool enviadoBroker = false;
SoftwareSerial slaveCOM(U0_RXD, U0_TXD);
int t1, t2; // Variaveis para temporizacao

/**
 * @brief
 * Funcao de envio do campo e parametro a ser estabelecido no escravo
 * @param int slave
 * @param float PUmidade
 */
void slaveCOMEnv(int const slave, float const PUmidade);

/**
 * @brief
 * Funcao de recebimento dos dados da serial
 */
void slaveCOMRec();

/**
 * @brief 
 * Funcao de tratamento dos dados recebidos via MQTT
 * @param char topic 
 * @param byte *payload 
 * @param int length 
 */
void callback(const char *topic, byte *payload, unsigned int length);

// Configuracao inicial do dispositivo mestre
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

  // Iniciar o LCD
  lcd.init();
  lcd.begin(20, 4, 0);
  lcd.backlight();

  // Iniciar conometro para envio e recebimento dos dados da serial e envio ao broker
  t1 = millis();
}

// Varredura
void loop()
{

  // Repeticao da conexao com o broker
  cliente.loop();

  // Atualizar o tempo atual do cronometro
  t2 = millis();

  // Verificar o tempo desde a ultima comunicacao
  if (((t2 - t1) > INTCOM) || !enviadoEscravo)
  {

    // Definir a flag de recebimento/envio dos dados do escravo como falso
    enviadoEscravo = false;

    // Verificar se os dados ja foram enviados ao escravo
    if (!enviadoEscravo)
    {
      // Verificar se a espaco para escrita na serial
      if (slaveCOM.availableForWrite() > 0)
      {

        // Efetuar comunicacao com o escravo responsavel pelo campo selecionado
        slaveCOMEnv(j, PUmidade);
      }

      // Definir a flag de envio ao escravo como verdadeiro
      // Definir a flag de envio ao broker como falso
      // Serial.println(msg);
      enviadoEscravo = true;
      enviadoBroker = false;
    }

    // Atualizar o inicio do cronometro
    t1 = millis();
  }

  // Verificar se ha resposta na serial
  if (slaveCOM.available() > 0)
  {

    // Receber os dados enviados do escravo
    slaveCOMRec();
  }

  // Verificar se ja foi enviado ao broker
  if (!enviadoBroker)
  {

    // Enviar ao broker e definir a flag de envio ao broker como verdadeiro
    cliente.publish(TUMIDADE, msg.c_str());
    enviadoBroker = true;
  }

  // Limpar o LCD e exibir a mensagem
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  lcd.print(msg);
  Serial.println(msg);

  // Verificar o estado da chave de selecao
  stateSWT = digitalRead(SWT);
  if (stateSWT == 1)
  {
    // Selecao de campo
    if (digitalRead(BT1) == 1)
    {
      j++; // Incremento de campo
    }
    if (digitalRead(BT2) == 1)
    {
      j--; // Decremento de campo
    }
  }
  else
  {
    // Controle do parametro umidade
    if (digitalRead(BT1) == 1)
    {
      PUmidade++; // Incremento do parametro umidade
    }
    if (digitalRead(BT2) == 1)
    {
      PUmidade--; // Decremento do parametro umidade
    }
  }
}

/*ROTINAS AUXILIARES*/

void slaveCOMEnv(int const slave, float const PUmidade)
{
  String aux = "Campo:" + String(slave) + ",Parametro:" + PUmidade + ";";

  // Procura o escravo para solicitar inforcao
  if (slaveCOM.availableForWrite() > 0)
  {
    slaveCOM.print(aux.c_str());
  }
}

void slaveCOMRec()
{
  char c;

  // Zerar mensagem
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
      enviadoEscravo = false;
    }
  }
  if (strcmp(topic, TPUMIDADE) == 0)
  {
    PUmidade = aux.toFloat();
    enviadoEscravo = false;
  }
}