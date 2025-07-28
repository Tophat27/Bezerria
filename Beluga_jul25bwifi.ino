#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiManager.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 4  // Pino da ESP32
#define EEPROM_SIZE 512

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiManager wifiManager;

// Configurações do servidor
String serverURL = "http://192.168.5.147:5000/temperature";

// Declaração antecipada do callback
void configModeCallback(WiFiManager *myWiFiManager);

// Função para salvar configurações na EEPROM
void saveConfig() {
  EEPROM.put(0, serverURL);
  EEPROM.commit();
  Serial.println("Configurações salvas na EEPROM");
}

// Função para limpar a EEPROM
void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("EEPROM limpa!");
}

// Função para carregar configurações da EEPROM
void loadConfig() {
  EEPROM.get(0, serverURL);
  if (serverURL.length() == 0 || serverURL.length() > 100) {
    Serial.println("URL inválida na EEPROM. Usando URL padrão...");
    serverURL = "http://192.168.5.147:5000/temperature";
    saveConfig(); // Salva a URL correta
  }
  Serial.println("URL do servidor carregada: " + serverURL);
}

// Função simulada de leitura de temperatura
float readTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  // Verifica se o sensor está funcionando
  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Erro: Sensor não encontrado!");
    return -999.0; // Valor de erro
  }
  
  if (temp == -127.0) {
    Serial.println("Erro: Sensor retornando valor inválido (-127°C)");
    return -999.0; // Valor de erro
  }
  
  Serial.printf("Temperatura lida: %.2f°C\n", temp);
  return temp;
}

void setup() {
  Serial.begin(115200);
  
  // Inicializa a EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Inicializa o sensor de temperatura
  sensors.begin();
  
  // Verifica se há sensores conectados
  int deviceCount = sensors.getDeviceCount();
  Serial.printf("Sensores encontrados: %d\n", deviceCount);
  
  if (deviceCount == 0) {
    Serial.println("Nenhum sensor DS18B20 encontrado!");
    Serial.println("Verifique as conexões:");
    Serial.println("- VCC -> 3.3V");
    Serial.println("- GND -> GND");
    Serial.println("- DATA -> Pino 4");
    Serial.println("- Resistor de 4.7kΩ entre VCC e DATA");
  }
  
  // Configura o WiFiManager
  wifiManager.setConfigPortalTimeout(180); // 3 minutos para configurar
  wifiManager.setAPCallback(configModeCallback);
  
  // Tenta conectar ao WiFi salvo
  if (!wifiManager.autoConnect("Bezerra_Config")) {
    Serial.println("Falha ao conectar ao WiFi. Iniciando portal de configuração...");
    // Se falhar, abre o portal de configuração
    if (!wifiManager.startConfigPortal("Bezerra_Config")) {
      Serial.println("Falha ao iniciar portal de configuração!");
      ESP.restart();
    }
  }
  
  Serial.println("Conectado ao WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Carrega as configurações da EEPROM
  loadConfig();
  
  // Mostra comandos disponíveis
  Serial.println("\n=== COMANDOS DISPONÍVEIS ===");
  Serial.println("Digite no Serial Monitor:");
  Serial.println("- 'restart' ou 'reset': Reinicia a ESP32");
  Serial.println("- 'wifi': Abre portal de configuração WiFi");
  Serial.println("- 'clear': Limpa a EEPROM (corrige URLs corrompidas)");
  Serial.println("- 'server': Mostra URL atual do servidor");
  Serial.println("- 'server http://IP:PORTA/caminho': Altera URL do servidor");
  Serial.println("==============================\n");
}

void loop() {
  // Verifica comandos via Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "restart" || command == "reset") {
      Serial.println("Reiniciando ESP32...");
      delay(1000);
      ESP.restart();
    }
    
    if (command == "wifi") {
      Serial.println("Abrindo portal de configuração WiFi...");
      wifiManager.startConfigPortal("Bezerra_Config");
    }
    
    if (command == "clear") {
      Serial.println("Limpando EEPROM...");
      clearEEPROM();
      Serial.println("EEPROM limpa! Reinicie para aplicar.");
    }
    
    if (command.startsWith("server ")) {
      String newServerURL = command.substring(7); // Remove "server " do início
      if (newServerURL.length() > 0) {
        serverURL = newServerURL;
        saveConfig();
        Serial.println("URL do servidor atualizada: " + serverURL);
      } else {
        Serial.println("Uso: server http://IP:PORTA/caminho");
        Serial.println("Exemplo: server http://192.168.1.100:5000/temperature");
      }
    }
    
    if (command == "server") {
      Serial.println("URL atual do servidor: " + serverURL);
      Serial.println("Para alterar, use: server http://IP:PORTA/caminho");
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    
    float temp = readTemperature();
    
    // Só envia se a temperatura for válida
    if (temp > -999.0) {
      // Cria o payload JSON
      String payload = "{\"temp\":" + String(temp) + "}";
      
      int httpCode = http.POST(payload);
      
      if (httpCode > 0) {
        Serial.printf("HTTP Code: %d\n", httpCode);
        Serial.println(http.getString());
      } else {
        Serial.printf("Erro na requisição: %s\n", http.errorToString(httpCode).c_str());
      }
    } else {
      Serial.println("Não enviando dados - sensor com erro");
    }
    
    http.end();
  } else {
    Serial.println("Conexão WiFi perdida!");
    // Tenta reconectar
    WiFi.reconnect();
  }
  
  delay(10000); // Espera 10 segundos
}

// Callback para o modo AP
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entrou no modo AP");
  Serial.print("IP do portal: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("SSID do portal: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
}