#define BUFFER_SIZE 1024
#define SDA_PIN D5
#define SCL_PIN D6

typedef struct {
  uint8_t data[BUFFER_SIZE];
  uint8_t index;
  bool is_master;
  uint8_t device_address;
} I2C_Packet;

I2C_Packet packet;
volatile bool new_packet = false;
volatile bool lastSDA = HIGH;
volatile uint8_t currentByte = 0;
volatile uint8_t bitCount = 0;
volatile bool interrupt_happened = false;

void setup() {
  Serial.begin(115200);
  packet.index = 0;
  packet.is_master = true;

  // Настройка пинов как вход
  pinMode(SDA_PIN, INPUT);
  pinMode(SCL_PIN, INPUT);

  // Подтягивающие резисторы (если нужно)
  // digitalWrite(SDA_PIN, HIGH);
  // digitalWrite(SCL_PIN, HIGH);

  // На Wemos D1 Mini прерывания работают только на определенных пинах
  attachInterrupt(digitalPinToInterrupt(SDA_PIN), handleInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SCL_PIN), handleInterrupt, CHANGE);
}

void loop() {
  if (interrupt_happened) {
    interrupt_happened = false;
  }

  if (new_packet) {
    if (packet.index > 0) {  // Проверяем, что есть данные для вывода
      Serial.print("Packet length: ");
      Serial.println(packet.index);

      for (int i = 0; i < packet.index; i++) {
        Serial.print("0x");
        Serial.print(packet.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (packet.is_master) {
        Serial.print("MASTER -> SLAVE 0x");
        Serial.print(packet.device_address, HEX);
        Serial.print(": ");
      } else {
        Serial.print("SLAVE -> MASTER: ");
      }

      for (int i = 0; i < packet.index; i++) {
        Serial.print("0x");
        Serial.print(packet.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      packet.index = 0;
      new_packet = false;
    }
  }
}

ICACHE_RAM_ATTR void handleInterrupt() {
    static bool isStarted = false;
    bool currentSDA = digitalRead(SDA_PIN);
    bool currentSCL = digitalRead(SCL_PIN);
    
    // Определяем текущие условия
    bool startCondition = (!currentSDA && lastSDA && currentSCL);
    bool stopCondition = (currentSDA && !lastSDA && currentSCL);
    
    // Обработка START условия
    if (startCondition) {
        isStarted = true;
        packet.index = 0;
        currentByte = 0;
        bitCount = 0;
        packet.device_address = 0;
        return; // Выходим сразу после обнаружения START
    }
    
    // Обработка данных во время передачи
    if (isStarted) {
        if (!currentSCL && lastSDA) {
            currentByte |= (lastSDA << bitCount);
            bitCount++;
            
            if (bitCount == 8) {
                packet.data[packet.index] = currentByte;
                packet.index++;
                currentByte = 0;
                bitCount = 0;
                
                if (packet.index >= BUFFER_SIZE - 1) {
                    new_packet = true;
                    isStarted = false;
                }
            }
        }
    }
    
    // Обработка STOP условия
    if (stopCondition) {
        new_packet = true;
        isStarted = false;
    }
    
    lastSDA = currentSDA;
}


