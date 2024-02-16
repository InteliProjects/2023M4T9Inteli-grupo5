//importação das bibliotecas
#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

byte customChar[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
  };

 
// declaração do display lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
    lcd.print("Balanca Zerada"); // Imprime uma mensagem no monitor lcd
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
      lcd.print("Peso maximo!"); // Imprime uma mensagem no monitor lcd de alerta de excesso de peso
    } else {
      digitalWrite(ledPin, LOW); // Desliga o LED
      lcd.setCursor(0,1);  // localiza o cursor do display para a primeira linha
      lcd.print("                 "); // printa uma mensagem vazia para apagar a mensagem do display
    }
  }
};
int ledPin = 5; // Pino do LED
// Cria um objeto da classe DigitalScale com os pinos e parâmetros especificados
DigitalScale myScale(26, 27, 5, 207039.327045, 1); // nesse caso, foi especifidado o pino do LED 5, peso máximo de 1kg e a escala conforme calibração
void setup() {
  Serial.begin(57600); // Inicializa a comunicação serial com uma taxa de 57600 bps
  pinMode(23, INPUT_PULLUP); // Configura o pino do botão como INPUT_PULLUP para evitar resistor externo
  lcd.init(); // inicia o dislay lcd
  lcd.backlight(); // ativa a luz de fundo do display
  lcd.createChar(0, customChar);

  // Exiba o caractere personalizado em todas as posições do LCD
  for (int i = 0; i < 32; i++) {
    lcd.setCursor(i % 16, i / 16);  // Defina a posição do cursor
    lcd.write(byte(0));             // Escreva o caractere personalizado
    delay(100);                     // Aguarde um curto período de tempo para observar o display
  }
  delay(1000);
  lcd.clear();
  lcd.print("Bem vindo!");
  pinMode(ledPin, OUTPUT); // Configura o pino do LED como saída
  digitalWrite(ledPin, HIGH); // Liga o LED durante a mensagem "Bem vindo!"
  delay(1000);
  digitalWrite(ledPin, LOW); // Desliga o LED quando a mensagem "Bem vindo!" apaga
  lcd.clear();
  lcd.print("Peso:");
}
void loop() {
  // Verifica se o botão foi pressionado para zerar a balança
  if (digitalRead(23) == LOW) {
    myScale.tare(); // Zera a balança
  }
  // Mede o peso atual e imprime no monitor serial
  float weight = myScale.measureWeight();
  lcd.setCursor(6, 0); // muda o cursor para atualizar o valor do peso
  lcd.print(weight, 5); //printa o valor medida na célula de carga no display
  lcd.print("kg   "); // exibe a unidade de medida
  Serial.print("Peso:"); // exibe a mensagem peso no monitor serial
  Serial.print(weight, 3); //
  Serial.println(" kg"); //printa o valor medida na célula de carga do monitor serial
  // Verifica se o peso máximo foi atingido
  myScale.checkMaxWeight();
  delay(1000); // Aguarda 1 segundo antes de fazer a próxima leitura
}