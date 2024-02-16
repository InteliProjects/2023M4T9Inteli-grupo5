#include <HX711.h>

// Classe para representar uma balança digital
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
    Serial.println("Balança Zerada"); // Imprime uma mensagem no monitor serial
  }

  // Função para medir o peso atual, descontando a tara
  float measureWeight() {
    return scale.get_units(5) - tareValue; // Lê o peso atual, faz a média de 5 leituras e subtrai a tara
  }

  // Função para verificar se o peso máximo foi atingido e acionar o LED, se necessário
  void checkMaxWeight() {
    float weight = measureWeight(); // Mede o peso atual
    if (weight >= maxWeight) { // Se o peso for maior ou igual ao máximo permitido
      digitalWrite(ledPin, HIGH); // Aciona o LED
      Serial.println("Peso máximo atingido!"); // Imprime uma mensagem no monitor serial
    } else {
      digitalWrite(ledPin, LOW); // Desliga o LED
    }
  }
};

// Cria um objeto da classe DigitalScale com os pinos e parâmetros especificados
DigitalScale myScale(4, 2, 23, 419.4, 4); // Exemplo com pino do LED 23 e peso máximo de 4kg

void setup() {
  Serial.begin(57600); // Inicializa a comunicação serial com uma taxa de 57600 bps
  pinMode(22, INPUT_PULLUP); // Configura o pino do botão como INPUT_PULLUP para evitar resistor externo
}

void loop() {
  // Verifica se o botão foi pressionado para zerar a balança
  if (digitalRead(22) == LOW) {
    myScale.tare(); // Zera a balança
  }

  // Mede o peso atual e imprime no monitor serial
  float weight = myScale.measureWeight();
  Serial.print("Peso: ");
  Serial.print(weight, 3);
  Serial.println(" kg");

  // Verifica se o peso máximo foi atingido
  myScale.checkMaxWeight();
  delay(1000); // Aguarda 1 segundo antes de fazer a próxima leitura
}
