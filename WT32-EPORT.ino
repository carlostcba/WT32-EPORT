#include "config.h"
#include "oempark_protocol.h"
#include "tcp_bridge.h"
#include "api_client.h"
#include "web_api_server.h"
#include "telnet_server.h"
#include "ota_update.h"
#include "security.h"

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("=== WT32-ETH01 OEMPARK GATEWAY      ===");
  Serial.println("=== Conversor TTL <-> TCP/HTTP/API  ===");
  Serial.println("=== Version 1.0 - Protocolo Completo===");
  Serial.println("========================================");
  Serial.println("Iniciando sistema...");
  
  // Cargar configuracion desde SPIFFS
  Serial.println("\n--- Cargando configuracion ---");
  loadConfiguration();
  
  // Configurar conexion Ethernet
  Serial.println("\n--- Configurando Ethernet ---");
  setupEthernet();
  
  if (!ETH.linkUp()) {
    Serial.println("  ADVERTENCIA: Sin conexion Ethernet");
    Serial.println("   - Verifique cable y conexiones de red");
    Serial.println("   - El sistema funcionara con limitaciones");
  }
  
  // Configurar servicios del gateway en orden de prioridad
  Serial.println("\n--- Configurando servicios ---");
  
  // 1. TCP Bridge (Prioridad 1) - Comunicacion directa
  Serial.println("1. Configurando TCP Bridge...");
  setupTCPBridge();
  
  // 2. Cliente HTTP (Prioridad 2) - API externa automatica
  Serial.println("2. Configurando Cliente HTTP...");
  setupSSL(); // Configuracion basica SSL
  
  // 3. Servidor Web/API unificado (Prioridad 3)
  Serial.println("3. Configurando Servidor Web/API...");
  setupWebAPIServer();
  
  // Servicios auxiliares
  Serial.println("4. Configurando servicios auxiliares...");
  setupTelnetServer();
  setupOTA();
  
  // Verificacion inicial de comunicacion OEMPARK
  Serial.println("\n--- Verificando comunicacion OEMPARK ---");
  delay(1000);
  
  // Limpiar buffer y enviar comando de prueba
  clearSerialBuffer();
  quickCommand("S0"); // Solicitar status
  
  String initialResponse = readOEMPARKResponse(3000);
  if (initialResponse.length() > 0) {
    Serial.println(" OEMPARK responde: " + initialResponse);
    
    // Solicitar version del firmware
    delay(500);
    quickCommand("V0");
    String version = readOEMPARKResponse(2000);
    if (version.length() > 0) {
      Serial.println(" Version OEMPARK: " + version);
    }
  } else {
    Serial.println("  ADVERTENCIA: Modulo OEMPARK no responde");
    Serial.println("   - Verifique conexiones TTL (GPIO16/17)");
    Serial.println("   - Verifique baudrate (" + String(config.baud_rate) + " bps)");
    Serial.println("   - Verifique alimentacion del modulo");
  }
  
  Serial.println("\n========================================");
  Serial.println("=== GATEWAY OEMPARK OPERATIVO       ===");
  Serial.println("========================================");
  Serial.println(" SERVICIOS ACTIVOS:");
  Serial.println("  1. TCP Bridge    : " + ETH.localIP().toString() + ":" + String(config.tcp_port) + " ↔ OEMPARK");
  Serial.println("  2. Cliente HTTP  : OEMPARK → " + (config.api_url.length() > 0 ? config.api_url : "(no configurada)"));
  Serial.println("  3. Servidor API  : http://" + ETH.localIP().toString() + "/api/ttl ↔ OEMPARK");
  Serial.println("  4. Interfaz Web  : http://" + ETH.localIP().toString());
  Serial.println("  5. Telnet Admin  : " + ETH.localIP().toString() + ":" + String(config.telnet_port));
  Serial.println("  6. OTA Updates   : Habilitado");
  
  Serial.println("\n PROTOCOLO OEMPARK:");
  Serial.println("  Velocidad TTL    : " + String(config.baud_rate) + " bps");
  Serial.println("  Pines Hardware   : RX=GPIO16, TX=GPIO17");
  Serial.println("  Direccion Modulo : " + config.oempark_address);
  Serial.println("  Formato Completo : STX + ADDR + CMD + PARAMS + SIB");
  Serial.println("  STX=0x02, SIB=0x0F segun documentacion");
  
  Serial.println("\n EJEMPLOS DE USO:");
  Serial.println("  TCP: telnet " + ETH.localIP().toString() + " " + String(config.tcp_port) + " → S0");
  Serial.println("  API: curl -X POST http://" + ETH.localIP().toString() + "/api/ttl -d '{\"command\":\"S0\"}'");
  Serial.println("  Web: http://" + ETH.localIP().toString() + " (Panel completo)");
  Serial.println("  Admin: telnet " + ETH.localIP().toString() + " " + String(config.telnet_port));
  
  Serial.println("\n Gateway iniciado correctamente");
  Serial.println("   Esperando conexiones y comandos...");
  Serial.println("   Memoria libre: " + String(ESP.getFreeHeap()) + " bytes");
}

void loop() {
  // === PROCESAMIENTO PRINCIPAL ===
  // Orden según prioridad establecida para máximo rendimiento
  
  // 1. TCP Bridge (Prioridad MAXIMA)
  // Comunicacion directa sin latencia para control en tiempo real
  handleTCPBridge();
  
  // 2. Cliente HTTP para API externa (Prioridad 2)  
  // Envio automatico de eventos importantes del OEMPARK
  handleAPIClient();
  
  // 3. Servidor Web/API REST unificado (Prioridad 3)
  // Atiende peticiones HTTP tanto de API como interfaz web
  handleWebAPIServer();
  
  // === SERVICIOS AUXILIARES ===
  // Procesamiento con menor frecuencia para no interferir con servicios críticos
  
  static unsigned long lastAuxServices = 0;
  if (millis() - lastAuxServices > 50) { // Cada 50ms
    handleTelnetServer();
    handleOTA();
    lastAuxServices = millis();
  }
  
  // === MONITOREO Y DIAGNOSTICO ===
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastMemoryCheck = 0;
  static unsigned long lastConnectivityCheck = 0;
  
  // Heartbeat cada 30 segundos
  if (millis() - lastHeartbeat > 30000) {
    Serial.println(" Gateway Heartbeat - Sistema operativo");
    Serial.printf("    Memoria: %d bytes libres (%.1f%% uso)\n", 
                  ESP.getFreeHeap(), 
                  100.0 - (ESP.getFreeHeap() * 100.0) / ESP.getHeapSize());
    
    // Estado de conexiones
    Serial.printf("   🔗 TCP: %s | API: %s | Ethernet: %s\n",
                  isTCPClientConnected() ? "Conectado" : "Libre",
                  isAPIConfigured() ? "Configurada" : "No config",
                  ETH.linkUp() ? "OK" : "Error");
    
    lastHeartbeat = millis();
  }
  
  // Verificacion de memoria cada 5 minutos
  if (millis() - lastMemoryCheck > 300000) {
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 20000) { // Menos de 20KB libre
      Serial.println("  ALERTA: Memoria baja (" + String(freeHeap) + " bytes)");
      Serial.println("   Considere reiniciar el gateway si persiste");
    }
    
    // Log de estadisticas
    Serial.println(" Estadisticas del sistema:");
    Serial.println("   Uptime: " + String(millis() / 1000) + " segundos");
    Serial.println("   Free Heap: " + String(freeHeap) + " bytes");
    Serial.println("   Largest Free Block: " + String(ESP.getMaxAllocHeap()) + " bytes");
    
    lastMemoryCheck = millis();
  }
  
  // Verificacion de conectividad cada 2 minutos
  if (millis() - lastConnectivityCheck > 120000) {
    if (!ETH.linkUp()) {
      Serial.println("  Red: Intentando reconectar Ethernet...");
      // El ETH se reconecta automaticamente, solo log
    }
    
    // Test basico de OEMPARK cada 2 minutos
    if (millis() > 120000) { // Solo despues de 2 minutos de inicio
      quickCommand("S0");
      String testResponse = readOEMPARKResponse(1000);
      if (testResponse.length() == 0) {
        Serial.println(" OEMPARK: Sin respuesta en test periodico");
      }
    }
    
    lastConnectivityCheck = millis();
  }
  
  // === GESTION DE ERRORES CRITICOS ===
  // Verificacion de condiciones que requieren reinicio
  static int consecutiveErrors = 0;
  
  // Si la memoria baja de 10KB por mucho tiempo, reiniciar
  if (ESP.getFreeHeap() < 10000) {
    consecutiveErrors++;
    if (consecutiveErrors > 100) { // ~10 segundos con memoria critica
      Serial.println(" CRITICO: Memoria insuficiente - Reiniciando...");
      delay(1000);
      ESP.restart();
    }
  } else {
    consecutiveErrors = 0;
  }
  
  // Pausa minima para estabilidad (crucial para ESP32)
  delay(10);
}