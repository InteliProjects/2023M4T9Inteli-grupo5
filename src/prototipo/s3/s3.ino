//importação das bibliotecas
#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <UbidotsEsp32Mqtt.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

byte barra[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};// criação do caracter de teste do display

const char *UBIDOTS_TOKEN = "BBUS-l2u6fk81TxPSJN4nAGpbsCFJz6wLmf";  // TOKEN do Ubidots
const char *WIFI_SSID = "Inteli-welcome";                           // Wi-Fi SSID
const char *WIFI_PASS = "";                                         // Senha do Wi-Fi
//const char *WIFI_SSID = "SHARE-RESIDENTE";                        
//const char *WIFI_PASS = "";                      
const char *DEVICE_LABEL = "balancairon";                           // Label do dispositivo onde os dados serão publicados
const char *WEIGHT_LABEL = "weight";                                // Variável (peso) do dispositivo onde os dados serão publicados
const char *TEMP_LABEL = "temperature";                             // Variável (temperatura) do dispositivo onde os dados serão publicados
const char *HMDT_LABEL = "humidity";                                // Variável (umidade) do dispositivo onde os dados serão publicados
const int PUBLISH_FREQUENCY = 2000;                                 // Frequência em milissegundos 
unsigned long timer;
uint8_t analogPin = 34;                                             // Pin usado para ler dados do GPIO34 ADC_CH6.

Ubidots ubidots(UBIDOTS_TOKEN); // Declaração do Ubidots
Adafruit_BME280 bme; // I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Declaração do display lcd
int ledPin = 5; // Pino do LED


// Função de callback para mensagens recebidas
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
   // Imprime a carga útil da mensagem
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Classe para representar a célula de carga como uma balança
class DigitalScale {
private:
  HX711 scale; // Objeto da biblioteca HX711 para interagir com a célula de carga
  int ledPin; // Pino conectado ao LED que indica quando o peso máximo foi atingido
  float tareValue; // Valor de tara, usado para zerar a balança
  float maxWeight; // Peso máximo que a balança pode medir antes de acionar o LED
public:
  // Construtor: inicializa a balança, configura os pinos e define o fator de escala
  DigitalScale(int dtPin, int sckPin, int ledPin, float scaleFactor, float maxWeight)
      : ledPin(ledPin), maxWeight(maxWeight) {
    pinMode(ledPin, OUTPUT); // Configura o pino do LED como saída
    scale.begin(dtPin, sckPin); // Inicializa a balança
    scale.set_scale(scaleFactor); // Define o fator de escala para a conversão de unidades
    delay(2000); // Aguarda 2 segundos para estabilizar
  }
  // Função para zerar a balança
  void tare() {
    tareValue = scale.get_units(); // Lê o valor atual da balança e armazena como tara
    Serial.println("Balanca Zerada"); // Imprime uma mensagem no monitor serial
    lcd.setCursor(0,1);
    lcd.print("Balanca Zerada  "); // Imprime uma mensagem no monitor lcd
  }
  // Função para medir o peso atual, descontando a tara
  float measureWeight() {
    return scale.get_units(5) - tareValue; // Lê o peso atual, faz a média de 5 leituras e subtrai a tara
  }
  // Função para verificar se o peso máximo foi atingido e acionar o LED, caso atinja o peso máximo
  void checkMaxWeight() {
    float weight = measureWeight(); // Mede o peso atual com a função measureWeight
    if (weight >= maxWeight) { // Se o peso for maior ou igual ao máximo permitido
      digitalWrite(ledPin, HIGH); // Aciona o LED
      Serial.println("Peso maximo atingido!"); // Imprime uma mensagem no monitor serial de alerta de excesso de peso
      lcd.setCursor(0, 1); // localiza o cursor do display para a segunda linha
      lcd.print("Peso maximo!    "); // Imprime uma mensagem no monitor lcd de alerta de excesso de peso
    } else {
      digitalWrite(ledPin, LOW); // Desliga o LED
//      lcd.setCursor(0,1);  // localiza o cursor do display para a primeira linha
//      lcd.print("                "); // printa uma mensagem vazia para apagar a mensagem do display
    }
  }
};

// Cria um objeto da classe DigitalScale com os pinos e parâmetros especificados
DigitalScale myScale(26, 27, 5, 219359.9079, 1); // nesse caso, foi especifidado o pino do LED 5, peso máximo de 1kg e a escala conforme calibração

void setup() {
  Serial.begin(57600); // Inicializa a comunicação serial com uma taxa de 57600 bps
  pinMode(23, INPUT_PULLUP); // Configura o pino do botão como INPUT_PULLUP para evitar resistor externo
  lcd.init(); // inicia o dislay lcd
  lcd.backlight(); // ativa a luz de fundo do display
  lcd.createChar(0, barra);
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS); // Conecta-se à rede Wi-Fi usando credenciais fornecidas
  ubidots.setCallback(callback); // Configura a função de retorno de chamada para o cliente Ubidots
  ubidots.setup(); // Configuração inicial do cliente Ubidots
  ubidots.reconnect(); // Tenta reconectar ao servidor Ubidots
  timer = millis();// Inicializa a variável de temporização com o tempo atual em milissegundos

  // Exibe o caractere personalizado em todas as posições do LCD
  for (int i = 0; i < 32; i++) {
    lcd.setCursor(i % 16, i / 16);  // Defina a posição do cursor
    lcd.write(byte(0));             // Escreva o caractere personalizado
    delay(100);                     // Aguarde um curto período de tempo para observar o display
  }
  delay(1000);
  lcd.clear(); // Limpa o display LCD
  lcd.print("Bem vindo!"); // Exibe a mensagem "Bem vindo!" no display LCD
  pinMode(ledPin, OUTPUT); // Configura o pino do LED como saída
  digitalWrite(ledPin, HIGH); // Liga o LED durante a mensagem "Bem vindo!"
  delay(1000);
  digitalWrite(ledPin, LOW); // Desliga o LED quando a mensagem "Bem vindo!" apaga
  lcd.clear();
  lcd.print("Peso:");
  bool status;
  status = bme.begin(0x76); // Inicializa o sensor BME280 com o endereço I2C 0x76
  if (!status) {
    Serial.println("Sensor BME280 não encontrado, cheque a fiação!"); // Imprime no console se o sensor BME280 não for encontrado
  }

}
void loop() {
  // Verifica se o botão foi pressionado para zerar a balança
  if (digitalRead(23) == LOW) {
    myScale.tare(); // Zera a balança
  }
  // Mede o peso atual e imprime no monitor serial
  float weight = myScale.measureWeight();
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  lcd.setCursor(6, 0); // Muda o cursor para atualizar o valor do peso
  lcd.print(weight, 3); // Printa o valor medida na célula de carga no display
  lcd.print("kg   "); // Exibe a unidade de medida
  Serial.print("Peso:"); // Exibe a mensagem peso no monitor serial
  Serial.print(weight, 3); //
  Serial.println(" kg"); // Printa o valor medida na célula de carga do monitor serial
  Serial.print("Temperatura:");
  Serial.print(temp);
  Serial.println("°C");
  Serial.print(humidity);
  Serial.println("%");
  
// Atualiza o display LCD com a temperatura e umidade
  lcd.setCursor(0, 1); // Muda o cursor para a segunda linha do display
  lcd.print(temp, 1); // Exibe a temperatura no display com uma casa decimal
  lcd.print("*C  ");
  lcd.print(humidity, 1); // Exibe a umidade no display com uma casa decimal
  lcd.print("%");

  // Verifica se o peso máximo foi atingido
  myScale.checkMaxWeight();
  delay(1000); // Aguarda 1 segundo antes de fazer a próxima leitura

  // Reconecta ao servidor Ubidots se não estiver conectado
  if (!ubidots.connected()) {
    ubidots.reconnect();
  }

  // Publica os dados no servidor Ubidots com a frequência especificada
  if (labs(millis() - timer) > PUBLISH_FREQUENCY) {
    ubidots.add(WEIGHT_LABEL, weight);   // Adiciona o valor do peso aos dados a serem publicados
    ubidots.add(TEMP_LABEL, temp);       // Adiciona o valor da temperatura aos dados a serem publicados
    ubidots.add(HMDT_LABEL, humidity);   // Adiciona o valor da umidade aos dados a serem publicados
    ubidots.publish(DEVICE_LABEL);       // Publica os dados no servidor Ubidots
    timer = millis();
  }
  ubidots.loop(); // Executa o loop do cliente Ubidots para manter a comunicação
  delay(1000);
}

 
