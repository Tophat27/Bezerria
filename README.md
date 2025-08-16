# 🌡️ Sistema de Monitoramento de Temperatura - ESP32 + Python

Este projeto consiste em um sistema de monitoramento de temperatura usando ESP32 com sensor DS18B20 e um servidor Python Flask para receber e exibir os dados.

## 📁 Arquivos do Projeto

- `Beluga_jul25bwifi.ino` - Código principal da ESP32 com WiFiManager e proteção EEPROM
- `Beluga_jul25b.ino` - Versão simplificada sem WiFiManager
- `Beluga_jul25b_recep.py` - Servidor Python Flask para receber dados

## 🔧 Componentes Necessários

### Hardware
- ESP32
- Sensor DS18B20
- Resistor de 4.7kΩ
- Cabos de conexão

### Software
- Arduino IDE
- Python 3.x
- Bibliotecas Arduino:
  - WiFiManager
  - OneWire
  - DallasTemperature
  - HTTPClient

## 🔌 Conexões do Sensor DS18B20

```
DS18B20    ESP32
VCC    →   3.3V
GND    →   GND
DATA   →   Pino 4
```

**Importante**: Conecte um resistor de 4.7kΩ entre VCC e DATA (resistor pull-up).

## 📋 Instalação e Configuração

### 1. Instalar Bibliotecas Arduino

No Arduino IDE, vá em **Sketch → Include Library → Manage Libraries** e instale:
- `WiFiManager` por tzapu
- `OneWire` por Paul Stoffregen
- `DallasTemperature` por Miles Burton

### 2. Configurar a ESP32

1. Abra o arquivo `Beluga_jul25bwifi.ino` no Arduino IDE
2. Selecione sua placa ESP32 em **Ferramentas → Placa**
3. Configure a porta COM em **Ferramentas → Porta**
4. Carregue o código na ESP32

### 3. Configurar WiFi (Primeira Vez)

1. Após carregar o código, a ESP32 criará uma rede WiFi chamada **"Bezerra_Config"**
2. Conecte-se a essa rede no seu computador/celular
3. Abra o navegador e acesse: `192.168.4.1`
4. Configure sua rede WiFi e senha
5. Clique em "Salvar"

### 4. Configurar Servidor Python

1. Instale o Flask:
```bash
pip install flask
```

2. Execute o servidor:
```bash
python Beluga_jul25b_recep.py
```

### 5. Liberar Porta 5000 (Windows)

Como **Administrador**, execute no PowerShell:
```powershell
New-NetFirewallRule -DisplayName "Flask Port 5000" -Direction Inbound -Protocol TCP -LocalPort 5000 -Action Allow
```

Ou via interface gráfica:
1. Pressione `Win + R`
2. Digite: `wf.msc`
3. **Regras de Entrada → Nova Regra → Porta**
4. TCP, porta 5000, permitir conexão

## 🎮 Comandos Disponíveis

Via **Serial Monitor** (115200 baud):

| Comando | Função |
|---------|--------|
| `restart` ou `reset` | Reinicia a ESP32 |
| `wifi` | Abre portal de configuração WiFi |
| `clear` | Limpa a EEPROM (corrige URLs corrompidas) |
| `server` | Mostra URL atual do servidor |
| `server http://IP:PORTA/caminho` | Altera URL do servidor (com validação) |
| `validate` | Valida e reconstrói configurações da EEPROM |

## 🛡️ Sistema de Proteção EEPROM

### Problema Resolvido
O sistema agora inclui proteção robusta contra corrupção da EEPROM que pode ocorrer ao desconectar a ESP32 do computador e conectá-la na energia.

### Funcionalidades de Proteção

#### **1. Sistema de Checksum**
- Calcula checksum da URL antes de salvar
- Verifica integridade ao carregar
- Detecta automaticamente corrupção da EEPROM

#### **2. Validação Rigorosa de URL**
- Verifica formato HTTP/HTTPS
- Valida presença de IP/domínio e porta
- Limita tamanho máximo da URL (100 caracteres)

#### **3. Estrutura de Dados Melhorada**
```
EEPROM Layout:
Offset 0-3:   Checksum (4 bytes)
Offset 4-7:   Tamanho da URL (4 bytes)
Offset 8+:    URL (caracteres)
```

#### **4. Recuperação Automática**
- Se detectar corrupção, usa URL padrão automaticamente
- Reconstrói configurações corrompidas
- Logs detalhados para debug

## 🔄 Como Funciona

### ESP32 (Beluga_jul25bwifi.ino)
1. **Inicialização**: Verifica sensores e conecta ao WiFi
2. **Proteção EEPROM**: Carrega configurações com validação de checksum
3. **Leitura**: Lê temperatura do sensor DS18B20 a cada 10 segundos
4. **Envio**: Envia dados via HTTP POST para o servidor Python
5. **Persistência**: Salva configurações na EEPROM com proteção

### Servidor Python (Beluga_jul25b_recep.py)
1. **Recebe**: Dados de temperatura via HTTP POST
2. **Exibe**: Temperatura no console
3. **Responde**: Confirmação de recebimento

## 🛠️ Solução de Problemas

### Sensor retorna -127°C
- Verifique as conexões do sensor
- Confirme se o resistor de 4.7kΩ está conectado
- Teste com outro sensor DS18B20

### Connection Refused
- Verifique se o servidor Python está rodando
- Confirme se a porta 5000 está liberada
- Verifique se o IP do servidor está correto

### URL corrompida na EEPROM
- Digite `validate` no Serial Monitor para revalidar
- Digite `clear` para limpar completamente a EEPROM
- Digite `restart` para reiniciar

### IP do servidor incorreto
- Digite `server` para ver a URL atual
- Digite `server http://IP_CORRETO:PORTA/caminho` para alterar
- Exemplo: `server http://192.168.1.100:5000/temperature`

### WiFi não conecta
- Digite `wifi` no Serial Monitor
- Reconfigure a rede WiFi

### EEPROM Corrompida (NOVO)
- O sistema detecta automaticamente corrupção
- Usa URL padrão e reconstrói automaticamente
- Digite `validate` para forçar revalidação
- Digite `clear` para limpar completamente

## 📊 Exemplo de Saída

### Serial Monitor (ESP32)
```
Sensores encontrados: 1
Conectado ao WiFi!
IP: 192.168.1.100
URL do servidor carregada com sucesso: http://192.168.5.147:5000/temperature
Checksum validado: 1234

=== COMANDOS DISPONÍVEIS ===
Digite no Serial Monitor:
- 'restart' ou 'reset': Reinicia a ESP32
- 'wifi': Abre portal de configuração WiFi
- 'clear': Limpa a EEPROM (corrige URLs corrompidas)
- 'server': Mostra URL atual do servidor
- 'server http://IP:PORTA/caminho': Altera URL do servidor
- 'validate': Valida e reconstrói configurações da EEPROM
==============================

Temperatura lida: 25.50°C
Dados enviados com sucesso!
Resposta do servidor: {"status": "success"}
```

### Console Python
```
 * Running on all addresses (0.0.0.0)
 * Running on http://127.0.0.1:5000
Temperatura recebida: 25.50°C
```

## 🔧 Personalização

### Alterar Intervalo de Leitura
No arquivo `.ino`, modifique:
```cpp
delay(10000); // 10 segundos
```

### Alterar Pino do Sensor
```cpp
#define ONE_WIRE_BUS 4  // Mude para outro pino
```

### Alterar URL do Servidor

#### Via Código
```cpp
String serverURL = "http://SEU_IP:5000/temperature";
```

#### Via Serial Monitor (Recomendado)
1. Abra o **Serial Monitor** (115200 baud)
2. Digite: `server` para ver a URL atual
3. Digite: `server http://NOVO_IP:PORTA/caminho` para alterar

**Exemplos:**
```
server http://192.168.1.100:5000/temperature
server http://localhost:5000/temperature
server http://127.0.0.1:8080/api/temp
```

**Vantagens:**
- Não precisa reprogramar a ESP32
- Configuração salva automaticamente na EEPROM com proteção
- Funciona mesmo após queda de energia
- Validação automática de formato

## 🆕 Novas Funcionalidades

### Comando `validate`
- Valida configurações da EEPROM manualmente
- Reconstrói automaticamente se detectar corrupção
- Mostra logs detalhados do processo

### Payload JSON Melhorado
- Inclui timestamp usando `millis()`
- Formatação da temperatura com 2 casas decimais
- Estrutura: `{"temp": 25.50, "timestamp": 1234567}`

### Tratamento de Erros HTTP Aprimorado
- Verificação específica para `HTTP_CODE_OK`
- Mensagens de erro mais claras e informativas
- Separação entre erros HTTP e erros de rede

## 📝 Notas Importantes

- **Rede WiFi**: Funciona apenas com redes 2.4GHz
- **Alimentação**: Use fonte de 3.3V estável
- **Distância**: Sensor funciona até 100m com cabo adequado
- **Persistência**: Configurações ficam salvas mesmo após queda de energia
- **Proteção EEPROM**: Sistema robusto contra corrupção de dados
- **Recuperação Automática**: Detecta e corrige problemas automaticamente

## 🤝 Contribuição

Para melhorias ou correções, abra uma issue ou envie um pull request.

## 📄 Licença

Este projeto está sob licença MIT. Veja o arquivo LICENSE para mais detalhes.

---

**Desenvolvido com Bezerr.Ia para monitoramento de temperatura com proteção avançada de dados** 
