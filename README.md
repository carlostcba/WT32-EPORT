# WT32-ETH01 OEMPARK Gateway

Gateway/Proxy para conectar módulos OEMPARK (protocolo TTL) con tecnologías modernas (TCP, HTTP, APIs REST).

## 🎯 Función Principal

El WT32-ETH01 actúa como **conversor de protocolos** entre:
- **Módulo OEMPARK** (TTL 9600 bps) ↔ **TCP/IP, HTTP, APIs REST**

## 🏗️ Arquitectura del Sistema

```
┌─────────────────┐    TTL     ┌──────────────────┐    Ethernet    ┌─────────────────┐
│   Módulo        │◄──────────►│  WT32-ETH01      │◄──────────────►│  Red TCP/IP     │
│   OEMPARK       │  9600 bps  │  Gateway         │   100 Mbps     │  APIs REST      │
│                 │            │                  │                │  Clientes TCP   │
└─────────────────┘            └──────────────────┘                └─────────────────┘
```

## 📡 Servicios Disponibles

### 1. TCP Bridge (Puerto 23) - **PRIORIDAD 1**
```bash
# Conexión directa TCP ↔ OEMPARK
telnet 192.168.1.100 23
> 02S0
< 02S01000
```

### 2. Cliente HTTP - **PRIORIDAD 2**
```bash
# OEMPARK → API Externa automática
OEMPARK responde → WT32-ETH01 → POST https://api.servidor.com/oempark
```

### 3. API REST (Puerto 80) - **PRIORIDAD 3**
```bash
# HTTP ↔ OEMPARK
curl -X POST http://192.168.1.100/ttl \
  -H "Content-Type: application/json" \
  -d '{"address":"02","command":"S0"}'
```

### 4. Servicios Auxiliares
- **Interfaz Web**: `http://192.168.1.100` - Panel de control
- **Telnet Admin**: Puerto 2323 - Consola administrativa
- **OTA Updates**: Actualizaciones remotas de firmware

## 🔌 Hardware Requerido

### WT32-ETH01
- **Microcontrolador**: ESP32-S0WD
- **Conectividad**: Ethernet nativo LAN8720
- **Alimentación**: 3.3V / 500mA mínimo
- **Pines TTL**: GPIO16 (RX), GPIO17 (TX)

### Conexiones
```
WT32-ETH01          Módulo OEMPARK
-----------         --------------
GPIO16 (RX2) ◄───── TX TTL
GPIO17 (TX2) ─────► RX TTL  
GND          ◄───── GND
3.3V         ─────► VCC (si aplica)
```

## 🚀 Instalación

### 1. Preparar Entorno
```bash
# Instalar PlatformIO
pip install platformio

# Clonar/descargar código
# Copiar archivos src/
```

### 2. Configurar PlatformIO
```ini
[env:wt32-eth01]
platform = espressif32
board = wt32-eth01
framework = arduino
lib_deps = 
    ArduinoJson@^6.21.3
    ArduinoOTA@^1.0.7
```

### 3. Compilar y Subir
```bash
# Compilar
pio run

# Subir por USB
pio run --target upload

# Monitor serie
pio device monitor --baud 115200
```

## ⚙️ Configuración

### Configuración Web
1. Conectar WT32-ETH01 a red Ethernet
2. Abrir navegador en `http://IP_DISPOSITIVO`
3. Configurar parámetros del gateway

### Parámetros Principales
- **Puerto TCP**: Puerto para bridge directo (defecto: 23)
- **Velocidad TTL**: Baudrate OEMPARK (defecto: 9600)
- **URL API**: Endpoint para cliente HTTP automático
- **Dirección OEMPARK**: Dirección del módulo (defecto: "02")

## 📋 Protocolo OEMPARK

### Formato de Comandos
```
NNCC[parámetros]

NN = Dirección del dispositivo (01-99)
CC = Código de comando
[parámetros] = Parámetros opcionales
```

### Comandos Principales
```bash
02S0    # Solicitar status
02A1    # Leer dirección  
02V0    # Ver versión firmware
02O1    # Abrir barrera (estado)
02O3    # Pulso barrera
02R1    # Cerrar barrera
02K3    # Autorizar ingreso
02K2    # Denegar acceso
```

### Respuestas Típicas
```bash
02S01000         # Status normal
02A102           # Dirección 02
02V0OEMV1.5      # Versión firmware
ACK              # Comando aceptado
NAK              # Comando rechazado
```

## 🌐 Uso de APIs

### Envío Automático (Cliente HTTP)
El gateway envía automáticamente datos del OEMPARK a APIs externas:

```json
{
  "device_id": "WT32-ETH01-OEMPARK-Gateway",
  "timestamp": "1672531200",
  "address": "02",
  "command": "S0", 
  "status_code": "1000",
  "identifier": "1234567890",
  "event_type": "card_read"
}
```

### API REST Server
```bash
# GET - Leer datos no bloqueante
curl http://192.168.1.100/ttl

# POST - Enviar comando
curl -X POST http://192.168.1.100/ttl \
  -H "Content-Type: application/json" \
  -d '{"address":"02","command":"S0"}'

# GET - Consulta con timeout
curl "http://192.168.1.100/ttl/query?addr=02&cmd=S0"
```

## 🔧 Administración

### Telnet Admin
```bash
telnet 192.168.1.100 2323

# Comandos disponibles:
status              # Estado del gateway
config              # Ver configuración
serial 02S0         # Enviar comando a OEMPARK
test                # Comando de prueba
reboot              # Reiniciar gateway
```

### Interfaz Web
- Panel de control completo
- Comandos OEMPARK predefinidos
- Consola en tiempo real
- Configuración del sistema
- Actualizaciones OTA

## 🔍 Monitoreo

### Logs del Sistema
```bash
pio device monitor --baud 115200

# Logs típicos:
TCP->TTL: 02S0
TTL->TCP: 02S01000
OEMPARK->API: {"command":"S0","status":"1000"}
API->TTL: 02K3
```

### Heartbeat
- Cada 30 segundos: Estado del gateway
- Monitoreo de memoria
- Verificación de conectividad

## 🚨 Troubleshooting

### Problemas Comunes

**❌ Sin conexión Ethernet**
```bash
# Verificar:
- Cable Ethernet conectado
- Switch/Router funcionando  
- Alimentación 3.3V estable (500mA mínimo)
- LEDs del WT32-ETH01
```

**❌ No comunica con OEMPARK**
```bash
# Verificar:
- Conexiones RX/TX cruzadas (RX16 ↔ TX_OEMPARK)
- Baudrate correcto (9600 bps)
- Niveles de voltaje (3.3V TTL)
- GND común entre dispositivos
```

**❌ Interfaz web no carga**
```bash
# Verificar:
- IP correcta del dispositivo
- Puerto 80 no bloqueado por firewall
- Gateway en misma red
```

### Comandos de Diagnóstico
```bash
# Desde Telnet Admin:
status              # Estado completo del sistema
serial 02S0         # Test comunicación OEMPARK
test                # Comando de prueba automático

# Desde API:
curl http://IP/status    # Estado del gateway
```

### Reset de Fábrica
```bash
# Desde Telnet Admin:
reboot

# O físicamente:
# 1. Desconectar alimentación
# 2. Mantener BOOT presionado
# 3. Conectar alimentación
# 4. Soltar BOOT después de 3 segundos
```

## 📈 Rendimiento

### Métricas Típicas
- **Latencia TCP Bridge**: < 10ms
- **Throughput HTTP**: 100+ req/min
- **Memoria libre**: > 200KB
- **CPU usage**: < 30%
- **Conexiones concurrentes**: 5 max

### Limitaciones
- **Baudrate TTL**: Máximo 115200 bps
- **Comandos/segundo**: ~50 máximo
- **Timeout respuestas**: 3 segundos
- **Buffer TTL**: 256 bytes por comando

## 🔄 Actualizaciones

### OTA via Web
1. Ir a `http://IP_DISPOSITIVO`
2. Sección "Actualización OTA"
3. Seleccionar archivo `.bin`
4. Subir firmware

### OTA via PlatformIO
```bash
# Configurar en platformio.ini:
upload_protocol = espota
upload_port = IP_DISPOSITIVO

# Subir OTA
pio run --target upload
```

## 📄 Licencia

Proyecto de código abierto para uso comercial y educativo.

## 🆘 Soporte

Para reportar bugs o solicitar funcionalidades, crear issue en el repositorio del proyecto.