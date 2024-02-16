//importação das bibliotecas
#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <UbidotsEsp32Mqtt.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


const char *UBIDOTS_TOKEN = "BBUS-1LgQxtgwBPrxJDGE9GrBvX5wbZqfcJ";  // TOKEN do Ubidots
const char *WIFI_SSID = "Inteli-COLLEGE";                         // Wi-Fi SSID (teste no Inteli)
const char *WIFI_PASS = "";                             // Senha do Wi-Fi (teste no Inteli)
//const char *WIFI_SSID = "SHARE-RESIDENTE";                          // Wi-Fi SSID (teste no Share)
//const char *WIFI_PASS = "";                        // Senha do Wi-Fi (teste no Share)       
const char *DEVICE_LABEL = "balancairon";                           // Label do dispositivo onde os dados serão publicados
const char *WEIGHT_LABEL = "weight";                                // Variável (peso) do dispositivo onde os dados serão publicados
const char *TEMP_LABEL = "temperature";                             // Variável (temperatura) do dispositivo onde os dados serão publicados
const char *HMDT_LABEL = "humidity";                                // Variável (umidade) do dispositivo onde os dados serão publicados
const int PUBLISH_FREQUENCY = 2000;                                 // Frequência em milissegundos 

//declaração de pinos
const int displayButton = 17;                                       // Pino do botão para ligar/desligar o display
const int tareButton = 16;                                          // Pino do botão para tara
const int pinRed = 4;                                               // Pino vermelho conectado ao LED que indica quando o peso máximo foi atingido
const int pinGreen = 2;                                             // Pino verde conectado ao LED que indica quando o peso máximo foi atingido
const int pinBlue = 15;                                             // Pino azul conectado ao LED que indica quando o peso máximo foi atingido
bool displayOn = true;                                              // Inicialmente, o display está ligado
unsigned long timer;
uint8_t analogPin = 34;                                             // Pin usado para ler dados do GPIO34 ADC_CH6.
const float scaleFactor = 219359.9079;                              // Escala utilizada na célula específica

//declaração dos objetos
Ubidots ubidots(UBIDOTS_TOKEN);                                     // Declaração do Ubidots
Adafruit_BME280 bme;                                                // Declarando o sensor de temperatura
LiquidCrystal_I2C lcd(0x27, 16, 2);                                 // Declaração do display lcd

// Classe para representar a célula de carga como uma balança
class DigitalScale { 
private:
  int pinRed;                                                       // Pino vermelho conectado ao LED que indica quando o peso máximo foi atingido
  int pinGreen;                                                     // Pino verde conectado ao LED que indica quando o peso máximo foi atingido
  int pinBlue;                                                      // Pino azul conectado ao LED que indica quando o peso máximo foi atingido
  float tareValue;                                                  // Valor de tara, usado para zerar a balança
  float maxWeight;                                                  // Peso máximo que a balança pode medir antes de acionar o LED
  float cellMaxWeight;                                              // Peso máximo que a balança pode medir em relação ao limite de peso da célula de carga
  int dtPin;                                                        // Porta de conexão com o DOUT
  int sckPin;                                                       // Porta de conexão dom o SCK
  float scaleFactor;                                                // Escala que será utilizada para calibra a balança
public:
  HX711 scale;                                                      // Objeto da biblioteca HX711 para interagir com a célula de carga
  // Construtor: inicializa a balança, configura os pinos e define o fator de escala
  DigitalScale(int dtPin, int sckPin, int pinRed, int pinGreen, int pinBlue, float scaleFactor, float maxWeight, float cellMaxWeight)
      : pinRed(pinRed), pinGreen(pinGreen), pinBlue(pinBlue), scaleFactor(scaleFactor),  maxWeight(maxWeight), dtPin(dtPin), sckPin(sckPin), cellMaxWeight(cellMaxWeight) {
    pinMode(pinRed, OUTPUT);                                        // Configura o pino vermelho do LED como saída
    pinMode(pinGreen, OUTPUT);                                      // Configura o pino verde do LED como saída
    pinMode(pinBlue, OUTPUT);                                       // Configura o pino azul do LED como saída
    scale.begin(dtPin, sckPin);                                     // Inicializa a balança
    scale.set_scale(scaleFactor);                                   // Define o fator de escala para a conversão de unidades
    delay(2000);                                                    // Aguarda 2 segundos para estabilizar
  }

  // Função para medir o peso atual, descontando a tara
  float measureWeight() {
    Serial.print("entrou na função");
    float medida = scale.get_units(5);
    Serial.println(medida);                 // Exibe no Serial o valor medido coma  média de 5 leitura e subtrai a tara, usado para desenvolvimento e teste
    return (medida);                          // Lê o peso atual, faz a média de 5 leituras e subtrai a tara
  }

  void tara() {
    scale.tare();
  }

  // Função para verificar se o peso máximo foi atingido e acionar o LED, caso atinja o peso máximo
  void checkMaxWeight() {
    float weight = measureWeight();                                 // Mede o peso atual com a função measureWeight
    if ((weight >= maxWeight) && (cellMaxWeight >= weight)) {       // Se o peso for maior ou igual ao máximo permitido
      digitalWrite(pinRed, 0);                                      // Liga o LED vermelho
      digitalWrite(pinGreen, 0);                                    // Liga o LED verde
      digitalWrite(pinBlue, 255);                                   // Desliga o LED azul
      Serial.println("Peso maximo atingido!");                      // Imprime uma mensagem no monitor serial de alerta de excesso de peso
      lcd.setCursor(0, 1);                                          // Localiza o cursor do display para a segunda linha
      lcd.print("Peso maximo!    ");                                // Imprime uma mensagem no monitor lcd de alerta de excesso de peso      
    } else if ((weight >= cellMaxWeight) && (weight > maxWeight)) { // Se o peso for maior ou igual ao peso limite físico da célula de carga
      digitalWrite(pinRed, 0);                                      // Liga o LED vermelho
      digitalWrite(pinGreen, 255);                                  // Desliga o LED verde
      digitalWrite(pinBlue, 255);                                   // Desliga o LED azul
      Serial.println("Peso maximo da celula atingido!");            // Imprime uma mensagem no monitor serial de alerta de excesso de peso da célula
      lcd.setCursor(0, 1);                                          // localiza o cursor do display para a segunda linha
      lcd.print("Maximo da celula!    ");                           // Imprime uma mensagem no monitor lcd de alerta de excesso de peso da célula
    }
    else {
      digitalWrite(pinRed, 0);                                    // Desliga o LED vermelho
      digitalWrite(pinGreen, 255);                                    // Liga o LED verde
      digitalWrite(pinBlue, 0);                                   // Desliga o LED azul
    }
  }
};

// função de callback para as mensagens recebidas
void callback(char *topic, byte *payload, unsigned int length) { 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");                                               // Imprime a carga útil da mensagem
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
};

// criação do caracter de teste do display
byte barra[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};

// Cria um objeto da classe DigitalScale com os pinos e parâmetros especificados
DigitalScale myScale(27, 26, pinRed, pinGreen, pinBlue, scaleFactor, 1, 1.5);        // Nesse caso, foi especificado os pinos 4, 2 e 15 do LED RGB, peso máximo de 1kg, peso limite da célula de carga de 1,5kg e a escala conforme calibração

void setup() {
  Serial.begin(57600);                                              // Inicializa a comunicação serial com uma taxa de 57600 bps
  pinMode(displayButton, INPUT_PULLUP);                             // Configura o pino do botão como INPUT_PULLUP para evitar resistor externo
  pinMode(tareButton, INPUT_PULLUP);                                // Declaração segundo botão de PullUp
  lcd.init();                                                       // Inicia o dislay lcd
  lcd.backlight();                                                  // Ativa a luz de fundo do display
  lcd.createChar(0, barra);                                         // Cria o caracter especial de teste para o LCD

  // Exibe o caractere personalizado em todas as posições do LCD
  for (int i = 0; i < 32; i++) {
    lcd.setCursor(i % 16, i / 16);                                  // Define a posição do cursor
    lcd.write(byte(0));                                             // Escreva o caractere personalizado
    delay(100);                                                     // Aguarde um curto período de tempo para observar o display
  };
  delay(1000);
  lcd.clear();                                                      // Limpa o display LCD para exibir as mensagens
  lcd.print("Bem vindo!");                                          // Exibe a mensagem "Bem vindo!" no display LCD
  lcd.clear();                                                      // Limpa o diplay para exibir uma nova mensagem
  bool status;                                                      // Status para indicar conexão com o sensor de temperatura
  status = bme.begin(0x76);                                         // Inicializa o sensor BME280 com o endereço I2C 0x76
  if (!status) {
    Serial.println("Sensor BME280 não encontrado, cheque a fiação!");// Imprime no console se o sensor BME280 não for encontrado
    lcd.print("sensor temp");                                       // Imprime no diplay que há um problema no sensor de temperatura  
    lcd.setCursor(0,1);
    lcd.print("desconectado!");
    delay(1500);
    
  }
  lcd.clear();                                                      // Limpa display para exibir uma nova mensagem
  lcd.print("conectando wifi");                                     // Exibe no display mensagem de conexão com Wi-Fi
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);                      // Conecta-se à rede Wi-Fi usando credenciais fornecidas
  delay(500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("conectando ao");
  lcd.setCursor(0,1);
  lcd.print("servidor...");
  ubidots.setCallback(callback);                                    // Configura a função de retorno de chamada para o cliente Ubidots
  delay(500);
  ubidots.setup();                                                  // Configuração inicial do cliente Ubidots
  ubidots.reconnect();                                              // Tenta reconectar ao servidor Ubidots
  timer = millis();                                                 // Inicializa a variável de temporização com o tempo atual em milissegundos
  pinMode(pinRed, OUTPUT);                                          // Configura o pino do LED como saída
  pinMode(pinGreen, OUTPUT);                                        // Configura o pino do LED como saída
  pinMode(pinBlue, OUTPUT);                                         // Configura o pino do LED como saída
  digitalWrite(pinRed, 255);                                        // Desliga o LED vermelho durante a mensagem "Bem vindo!"
  digitalWrite(pinGreen, 0);                                        // Liga o LED verde durante a mensagem "Bem vindo!"
  digitalWrite(pinBlue, 255);                                       // Desliga o LED azul durante a mensagem "Bem vindo!"
  delay(1000);                                                      // Tempo para permanecer o LED aceso
  digitalWrite(pinGreen, 255);                                      // Desliga o LED verde quando a mensagem "Bem vindo!" apaga
  lcd.clear();
  lcd.print("Peso:");                                               // Exibe no display a palavra peso, para variar no loop somente o valor e facilitar a visualização
  // myScale.tara();
  // Serial.print("tara");
};
void loop() {
  Serial.print("entrou no looping");
  
  float weight = myScale.measureWeight();                           // Mede o peso atual e imprime no monitor serial
  Serial.print(weight);
  float temp = bme.readTemperature();                               // Mede a temperatura atual
  float humidity = bme.readHumidity();                              // Mede a umidade atual
  lcd.setCursor(6, 0);                                              // Muda o cursor para atualizar o valor do peso
  lcd.print(weight, 3);                                             // Printa o valor medida na célula de carga no display
  lcd.print("kg   ");                                               // Exibe a unidade de medida
  Serial.print("Peso:");                                            // Exibe a mensagem peso no monitor serial
  Serial.print(weight, 3);                                          // Exibe o valor medido na célula de carga do monitor serial
  Serial.println(" kg");                                            // Exibe unidade de medida
  Serial.print("Temperatura:");                                     // Exibe o valor medido no sensor de temperatura do monitor serial
  Serial.print(temp);
  Serial.println("°C");
  Serial.print(humidity);                                           // Exibe valor da umidade no monitor serial
  Serial.println("%");
  
  // Atualiza o display LCD com a temperatura e umidade
  lcd.setCursor(0, 1);                                              // Muda o cursor para a segunda linha do display
  lcd.print(temp, 1);                                               // Exibe a temperatura no display
  lcd.print("*C  ");
  lcd.print(humidity, 1);                                           // Exibe a umidade no display
  lcd.print("%");

  myScale.checkMaxWeight();                                         // Verifica se o peso máximo foi atingido
  delay(1000);                                                      // Aguarda 1 segundo antes de fazer a próxima leitura

  if (!ubidots.connected()) {                                       // se o Ubidots apresentar falha na conexão, uma mensagem será exibida do dislay e ele tentará se conectar novamente
    Serial.print("Erro na conexao");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Erro na conexao!");
    lcd.setCursor(0,1);
    lcd.print("Reconectando...");
    ubidots.reconnect();
    delay(1000);
    lcd.clear();
  }

  if (labs(millis() - timer) > PUBLISH_FREQUENCY) {                 // Publica os dados no servidor Ubidots com a frequência especificada
    ubidots.add(WEIGHT_LABEL, weight);                              // Adiciona o valor do peso aos dados a serem publicados
    ubidots.add(TEMP_LABEL, temp);                                  // Adiciona o valor da temperatura aos dados a serem publicados
    ubidots.add(HMDT_LABEL, humidity);                              // Adiciona o valor da umidade aos dados a serem publicados
    ubidots.publish(DEVICE_LABEL);                                  // Publica os dados no servidor Ubidots
    timer = millis();
  }

  ubidots.loop();                                                   // Executa o loop do cliente Ubidots para manter a comunicação
  delay(1000);

  if (digitalRead(displayButton) == LOW) {
    delay(50);  // Debounce (evita leituras instáveis)
    if (digitalRead(displayButton) == LOW) {
      // Inverte o estado do display (liga/desliga)
      displayOn = !displayOn;
      // Ligar ou desligar o display com base no estado
      if (displayOn) {
        lcd.backlight();  // Ligar o backlight
      } else {
        lcd.noBacklight();  // Desligar o backlight
      }
      // Aguarda um curto período para evitar múltiplas leituras do botão
      delay(500);
    }
  };

  if (digitalRead(tareButton) == LOW) {                             // Verifica se o botão foi pressionado para zerar a balança
    myScale.tara();                                                 // Lê o valor atual da balança e armazena como tara
    lcd.setCursor(0,1);
    lcd.print("Balanca Zerada  ");                                  // Imprime a mensagem que a tara foi acionada 
    delay(200);                                                     // Delay para estabilizar
  };


};

 
