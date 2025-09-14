#define BUFFER_SIZE 1024
#define MAX_PACKETS 16
#define SDA_PIN D5
#define SCL_PIN D6

typedef struct {
    uint8_t data[BUFFER_SIZE];
    uint8_t index;
    bool is_master;
    uint8_t device_address;
} I2C_Packet;

I2C_Packet packets[MAX_PACKETS];
volatile uint8_t current_packet = 0;
volatile bool new_packet = false;
volatile uint8_t currentByte = 0;
volatile uint8_t bitCount = 0;

volatile bool lastSDA = HIGH;
volatile bool lastSCL = HIGH;

uint8_t bitsSda[BUFFER_SIZE];
uint8_t bitsScl[BUFFER_SIZE];
uint8_t bitCounter = 0;
uint16_t byteCounter = 0;
volatile bool isStarted = false;

unsigned long lastCheck = 0;  // Для контроля скорости
unsigned long bytesPerSecond = 0;  // Счетчик байтов в секунду


void setup() {
    Serial.begin(115200);
    
    // Инициализация всех пакетов
    for (int i = 0; i < MAX_PACKETS; i++) {
        packets[i].index = 0;
        packets[i].is_master = true;
    }

    // Настройка пинов как вход
    pinMode(SDA_PIN, INPUT);
    pinMode(SCL_PIN, INPUT);

    // Подтягивающие резисторы
    // digitalWrite(SDA_PIN, HIGH);
    // digitalWrite(SCL_PIN, HIGH);
}


void loop() {
    bool currentSDA = digitalRead(SDA_PIN);
    bool currentSCL = digitalRead(SCL_PIN);

    // Контроль скорости передачи
    if (millis() - lastCheck >= 1000) {
        Serial.print("Bytes per second: ");
        Serial.println(bytesPerSecond);
        bytesPerSecond = 0;  // Сброс счетчика
        lastCheck = millis();
    }

    // Фильтрация нестабильных сигналов
    if (currentSDA == lastSDA && currentSCL == lastSCL) {
        lastSDA = currentSDA;
        lastSCL = currentSCL;
        return;
    }
    
    // Простая фильтрация дребезга
    delayMicroseconds(2);
    
    // Перепроверка состояния после фильтрации
    currentSDA = digitalRead(SDA_PIN);
    currentSCL = digitalRead(SCL_PIN);

    // Определение условий START/STOP
    bool startCondition = (!currentSDA && lastSDA && currentSCL);
    bool stopCondition = (currentSDA && !lastSDA && currentSCL);
    
    // Обработка START условия
    if (startCondition) {
        isStarted = true;
        byteCounter = 0;
        bitCounter = 0;
    }
    
    if (isStarted) {
        // Проверка на переполнение буфера
        if (byteCounter < BUFFER_SIZE) {
            if (bitCounter == 0) {
                bitsSda[byteCounter] = 0;
                bitsScl[byteCounter] = 0;
            }
            
            // Сборка байтов с проверкой стабильности
            if (digitalRead(SDA_PIN) == currentSDA) {
                bitsSda[byteCounter] |= (currentSDA << bitCounter);
            }
            if (digitalRead(SCL_PIN) == currentSCL) {
                bitsScl[byteCounter] |= (currentSCL << bitCounter);
            }
            
            bitCounter++;
            
            if (bitCounter == 8) {
                bitCounter = 0;
                byteCounter++;
                bytesPerSecond++;  // Увеличиваем счетчик
            }
        }
    }
    
    if (stopCondition) {
        // Вывод собранных данных
        for (uint16_t byteIterator = 0; byteIterator < byteCounter; byteIterator++) {
            Serial.print("SCL: ");
            Serial.print(bitsScl[byteIterator], BIN);
            Serial.print(" | SDA: ");
            Serial.println(bitsSda[byteIterator], BIN);
        }
        
        // Сброс счетчиков
        byteCounter = 0;
        isStarted = false;
    }

    // Сохранение текущего состояния
    lastSDA = currentSDA;
    lastSCL = currentSCL;
}
//   return;

//     // Проверяем изменение состояния линий
//     if (currentSDA != lastSDA || currentSCL != lastSCL) {
//         handleInterrupt(currentSDA, currentSCL);
//     }

//     if (new_packet) {
//         I2C_Packet& p = packets[current_packet];
        
//         if (p.index > 0) {
//             // Определяем направление передачи
//             if (p.is_master) {
//                 Serial.print("MASTER -> SLAVE 0x");
//                 Serial.print(p.device_address, HEX);
//                 Serial.print(": ");
//             } else {
//                 Serial.print("SLAVE -> MASTER: ");
//             }
            
//             // Выводим данные
//             for (int i = 0; i < p.index; i++) {
//                 Serial.print("0x");
//                 Serial.print(p.data[i], HEX);
//                 Serial.print(" ");
//             }
//             Serial.println();
            
//             // Переходим к следующему пакету
//             current_packet = (current_packet + 1) % MAX_PACKETS;
//             p.index = 0;
//             new_packet = false;
//         }
//     }

//     // Сохраняем текущее состояние линий
//     lastSDA = currentSDA;
//     lastSCL = currentSCL;
// }

// Функция обработки изменений
void handleInterrupt(bool currentSDA, bool currentSCL) {
    static bool isStarted = false;
    I2C_Packet& p = packets[current_packet];
    
    // Определяем условия
    bool startCondition = (!currentSDA && lastSDA && currentSCL);
    bool stopCondition = (currentSDA && !lastSDA && currentSCL);
    
    // Обработка START условия
    if (startCondition) {
        isStarted = true;
        p.index = 0;
        currentByte = 0;
        bitCount = 0;
        p.device_address = 0;
    }
    
    // Обработка данных
    if (isStarted) {
        if (!currentSCL && lastSDA) {
            currentByte |= (lastSDA << bitCount);
            bitCount++;
            
            if (bitCount == 8) {
                p.data[p.index] = currentByte;
                p.index++;
                currentByte = 0;
                bitCount = 0;
                
                if (p.index >= BUFFER_SIZE - 1) {
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
}
