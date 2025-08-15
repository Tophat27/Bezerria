#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiManager.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 4  // Pino da ESP32
#define EEPROM_SIZE 512
#define URL_MAX_LENGTH 100
#define CHECKSUM_OFFSET 0
#define URL_OFFSET 4
#define BACKUP_OFFSET 200  // Backup da configuração em local diferente
#define VALIDATION_MAGIC 0xAA55AA55  // Número mágico para validação
#define MAGIC_OFFSET 8

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiManager wifiManager;

// Configurações do servidor - ALTERE PARA SEU IP REAL
String serverURL = "http://192.168.0.10:5000/temperature";

// Declaração antecipada do callback
void configModeCallback(WiFiManager *myWiFiManager);

// Função para calcular checksum mais robusto
unsigned int calculateChecksum(const String& data) {
  unsigned int checksum = 0;
  for (int i = 0; i < data.length(); i++) {
    checksum = ((checksum << 5) + checksum) + data.charAt(i); // Checksum mais robusto
  }
  return checksum;
}

// Função para validar URL
bool isValidURL(const String& url) {
  if (url.length() == 0 || url.length() > URL_MAX_LENGTH) {
    return false;
  }
  
  // Verifica se começa com http:// ou https://
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    return false;
  }
  
  // Verifica se contém pelo menos um ponto (para IP ou domínio)
  if (url.indexOf('.') == -1) {
    return false;
  }
  
  // Verifica se contém dois pontos (para porta)
  if (url.indexOf(':') == -1) {
    return false;
  }
  
  return true;
}

// Função para salvar configurações na EEPROM com múltiplas proteções
void saveConfig() {
  // Calcula checksum da URL
  unsigned int checksum = calculateChecksum(serverURL);
  
  // Salva o checksum primeiro
  EEPROM.put(CHECKSUM_OFFSET, checksum);
  
  // Salva o tamanho da URL
  int urlLength = serverURL.length();
  EEPROM.put(URL_OFFSET, urlLength);
  
  // Salva o número mágico para validação adicional
  EEPROM.put(MAGIC_OFFSET, VALIDATION_MAGIC);
  
  // Salva a URL
  for (int i = 0; i < urlLength; i++) {
    EEPROM.write(MAGIC_OFFSET + 4 + i, serverURL.charAt(i));
  }
  
  // Cria backup em local diferente
  EEPROM.put(BACKUP_OFFSET, checksum);
  EEPROM.put(BACKUP_OFFSET + 4, urlLength);
  EEPROM.put(BACKUP_OFFSET + 8, VALIDATION_MAGIC);
  
  for (int i = 0; i < urlLength; i++) {
    EEPROM.write(BACKUP_OFFSET + 12 + i, serverURL.charAt(i));
  }
  
  EEPROM.commit();
  Serial.println("Configurações salvas na EEPROM com proteção dupla");
  Serial.println("URL: " + serverURL);
  Serial.printf("Checksum: %u\n", checksum);
  Serial.println("Backup criado em offset " + String(BACKUP_OFFSET));
}

// Função para limpar a EEPROM
void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("EEPROM limpa!");
}

// Função para tentar recuperar configurações da EEPROM
bool tryLoadConfigFromOffset(int offset) {
  // Lê o checksum salvo
  unsigned int savedChecksum;
  EEPROM.get(offset, savedChecksum);
  
  // Lê o tamanho da URL
  int urlLength;
  EEPROM.get(offset + 4, urlLength);
  
  // Lê o número mágico
  unsigned int magic;
  EEPROM.get(offset + 8, magic);
  
  // Valida o tamanho da URL
  if (urlLength <= 0 || urlLength > URL_MAX_LENGTH) {
    return false;
  }
  
  // Valida o número mágico
  if (magic != VALIDATION_MAGIC) {
    return false;
  }
  
  // Lê a URL da EEPROM
  String loadedURL = "";
  for (int i = 0; i < urlLength; i++) {
    char c = EEPROM.read(offset + 12 + i);
    loadedURL += c;
  }
  
  // Calcula checksum da URL lida
  unsigned int calculatedChecksum = calculateChecksum(loadedURL);
  
  // Verifica se o checksum confere
  if (savedChecksum != calculatedChecksum) {
    return false;
  }
  
  // Valida a URL
  if (!isValidURL(loadedURL)) {
    return false;
  }
  
  // Se chegou até aqui, a URL é válida
  serverURL = loadedURL;
  return true;
}

// Função para carregar configurações da EEPROM com recuperação automática
void loadConfig() {
  Serial.println("Carregando configurações da EEPROM...");
  
  // Tenta carregar da localização principal
  if (tryLoadConfigFromOffset(0)) {
    Serial.println("URL do servidor carregada com sucesso da localização principal");
    Serial.println("URL: " + serverURL);
    return;
  }
  
  Serial.println("Localização principal corrompida, tentando backup...");
  
  // Tenta carregar do backup
  if (tryLoadConfigFromOffset(BACKUP_OFFSET)) {
    Serial.println("URL do servidor carregada com sucesso do backup");
    Serial.println("URL: " + serverURL);
    
    // Restaura o backup para a localização principal
    Serial.println("Restaurando backup para localização principal...");
    saveConfig();
    return;
  }
  
  Serial.println("Ambas as localizações estão corrompidas!");
  Serial.println("Usando URL padrão e recriando configurações...");
  
  // Se ambas falharem, usa URL padrão
  serverURL = "http://192.168.0.10:5000/temperature";
  saveConfig();
}

// Função para verificar integridade da EEPROM
void checkEEPROMIntegrity() {
  Serial.println("\n=== VERIFICAÇÃO DE INTEGRIDADE DA EEPROM ===");
  
  // Verifica localização principal
  unsigned int mainChecksum, mainMagic;
  int mainLength;
  EEPROM.get(0, mainChecksum);
  EEPROM.get(4, mainLength);
  EEPROM.get(8, mainMagic);
  
  Serial.printf("Principal - Checksum: %u, Tamanho: %d, Magic: 0x%08X\n", 
                mainChecksum, mainLength, mainMagic);
  
  // Verifica backup
  unsigned int backupChecksum, backupMagic;
  int backupLength;
  EEPROM.get(BACKUP_OFFSET, backupChecksum);
  EEPROM.get(BACKUP_OFFSET + 4, backupLength);
  EEPROM.get(BACKUP_OFFSET + 8, backupMagic);
  
  Serial.printf("Backup   - Checksum: %u, Tamanho: %d, Magic: 0x%08X\n", 
                backupChecksum, backupLength, backupMagic);
  
  // Verifica qual está válida
  bool mainValid = (mainMagic == VALIDATION_MAGIC && mainLength > 0 && mainLength <= URL_MAX_LENGTH);
  bool backupValid = (backupMagic == VALIDATION_MAGIC && backupLength > 0 && backupLength <= URL_MAX_LENGTH);
  
  if (mainValid && backupValid) {
    Serial.println("Status: AMBAS as localizações estão válidas");
  } else if (mainValid) {
    Serial.println("Status: Apenas localização PRINCIPAL está válida");
  } else if (backupValid) {
    Serial.println("Status: Apenas BACKUP está válido");
  } else {
    Serial.println("Status: AMBAS as localizações estão CORROMPIDAS");
  }
  
  Serial.println("=============================================\n");
}

// Função de leitura de temperatura
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
  
  // Aguarda estabilização da alimentação
  delay(1000);
  
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
  Serial.println("- 'validate': Valida e reconstrói configurações da EEPROM");
  Serial.println("- 'integrity': Verifica integridade da EEPROM");
  Serial.println("- 'backup': Força criação de backup");
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
    
    if (command == "validate") {
      Serial.println("Validando configurações da EEPROM...");
      loadConfig();
    }
    
    if (command == "integrity") {
      checkEEPROMIntegrity();
    }
    
    if (command == "backup") {
      Serial.println("Forçando criação de backup...");
      saveConfig();
    }
    
    if (command.startsWith("server ")) {
      String newServerURL = command.substring(7); // Remove "server " do início
      if (newServerURL.length() > 0) {
        if (isValidURL(newServerURL)) {
          serverURL = newServerURL;
          saveConfig();
          Serial.println("URL do servidor atualizada: " + serverURL);
        } else {
          Serial.println("URL inválida! Use o formato: http://IP:PORTA/caminho");
          Serial.println("Exemplo: server http://192.168.0.10:5000/temperature");
        }
      } else {
        Serial.println("Uso: server http://IP:PORTA/caminho");
        Serial.println("Exemplo: server http://192.168.0.10:5000/temperature");
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
      // Cria o payload JSON com timestamp
      unsigned long timestamp = millis();
      String payload = "{\"temp\":" + String(temp, 2) + ",\"timestamp\":" + String(timestamp) + "}";
      
      int httpCode = http.POST(payload);
      
      if (httpCode == HTTP_CODE_OK) {
        Serial.println("Dados enviados com sucesso!");
        String response = http.getString();
        if (response.length() > 0) {
          Serial.println("Resposta do servidor: " + response);
        }
      } else if (httpCode > 0) {
        Serial.printf("Erro HTTP: %d - %s\n", httpCode, http.errorToString(httpCode).c_str());
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
