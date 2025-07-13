#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <WiFi.h>
#include "config.h"
#include "oempark_protocol.h"
#include "tcp_bridge.h"
#include "api_client.h"

WiFiServer telnetServer(2323); // Puerto 2323 para evitar conflicto con TCP Bridge
WiFiClient telnetClient;

void setupTelnetServer() {
  telnetServer.begin(config.telnet_port);
  Serial.println("Servidor Telnet Admin iniciado en puerto " + String(config.telnet_port));
}

void handleTelnetServer() {
  // Verificar nuevas conexiones Telnet
  if (telnetServer.hasClient()) {
    if (telnetClient && telnetClient.connected()) {
      telnetClient.stop(); // Desconectar cliente anterior
    }
    telnetClient = telnetServer.available();
    
    if (telnetClient) {
      telnetClient.println("========================================");
      telnetClient.println("=== WT32-ETH01 OEMPARK Gateway Admin ===");
      telnetClient.println("========================================");
      telnetClient.println("Comandos disponibles:");
      telnetClient.println("  status       - Ver estado del gateway");
      telnetClient.println("  config       - Ver configuracion actual");
      telnetClient.println("  oempark CMD  - Enviar comando a OEMPARK");
      telnetClient.println("  test         - Comandos de prueba");
      telnetClient.println("  api          - Estado de API externa");
      telnetClient.println("  tcp          - Estado TCP Bridge");
      telnetClient.println("  reboot       - Reiniciar gateway");
      telnetClient.println("  help         - Mostrar ayuda");
      telnetClient.println("");
      telnetClient.println("Ejemplos OEMPARK:");
      telnetClient.println("  oempark S0   - Status del modulo");
      telnetClient.println("  oempark A1   - Leer direccion");
      telnetClient.println("  oempark O1   - Abrir barrera");
      telnetClient.println("  oempark K3   - Autorizar entrada");
      telnetClient.println("========================================");
      telnetClient.print("admin# ");
      Serial.println("Telnet Admin: Cliente conectado desde " + telnetClient.remoteIP().toString());
    }
  }
  
  // Procesar comandos Telnet
  if (telnetClient && telnetClient.connected() && telnetClient.available()) {
    String command = telnetClient.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      telnetClient.println("=== Estado del Gateway ===");
      telnetClient.println("Device ID: " + config.device_name);
      telnetClient.println("IP: " + ETH.localIP().toString());
      telnetClient.println("Puerto TCP: " + String(config.tcp_port));
      telnetClient.println("Puerto Telnet: " + String(config.telnet_port));
      telnetClient.println("Velocidad TTL: " + String(config.baud_rate) + " bps");
      telnetClient.println("Direccion OEMPARK: " + config.oempark_address);
      telnetClient.println("Cliente TCP: " + String(isTCPClientConnected() ? "Conectado" : "Desconectado"));
      telnetClient.println("API Externa: " + (isAPIConfigured() ? config.api_url : "No configurada"));
      telnetClient.println("Memoria libre: " + String(ESP.getFreeHeap()) + " bytes");
      telnetClient.println("Uptime: " + String(millis() / 1000) + " segundos");
      
    } else if (command == "config") {
      telnetClient.println("=== Configuracion Actual ===");
      telnetClient.println("device_name: " + config.device_name);
      telnetClient.println("tcp_port: " + String(config.tcp_port));
      telnetClient.println("telnet_port: " + String(config.telnet_port));
      telnetClient.println("baud_rate: " + String(config.baud_rate));
      telnetClient.println("oempark_address: " + config.oempark_address);
      telnetClient.println("api_url: " + (config.api_url.length() > 0 ? config.api_url : "(no configurada)"));
      telnetClient.println("ssl_enabled: " + String(config.ssl_enabled ? "true" : "false"));
      
    } else if (command.startsWith("oempark ")) {
      String cmd = command.substring(8);
      cmd.trim();
      
      if (cmd.length() >= 2) {
        telnetClient.println("Enviando a OEMPARK: " + cmd);
        quickCommand(cmd);
        
        // Esperar respuesta
        String response = readOEMPARKResponse(3000); // 3 segundos timeout
        if (response.length() > 0) {
          telnetClient.println("Respuesta OEMPARK: " + response);
          
          // Mostrar interpretacion
          OEMPARKData data = parseOEMPARKToStruct(response);
          telnetClient.println("Tipo: " + data.event_type);
          if (data.status_code.length() > 0) {
            telnetClient.println("Status: " + data.status_code);
          }
          if (data.identifier.length() > 0) {
            telnetClient.println("Identificador: " + data.identifier);
          }
        } else {
          telnetClient.println("TIMEOUT: Sin respuesta del modulo OEMPARK");
        }
      } else {
        telnetClient.println("Error: Comando OEMPARK debe tener al menos 2 caracteres");
        telnetClient.println("Ejemplos: S0, A1, O1, K3, V0");
      }
      
    } else if (command == "test") {
      telnetClient.println("=== Ejecutando Tests ===");
      
      // Test 1: Comunicacion OEMPARK
      telnetClient.println("1. Test comunicacion OEMPARK...");
      quickCommand("S0");
      String response = readOEMPARKResponse(2000);
      if (response.length() > 0) {
        telnetClient.println("   ✓ OEMPARK responde: " + response);
      } else {
        telnetClient.println("   ✗ OEMPARK no responde");
      }
      
      // Test 2: Test version
      telnetClient.println("2. Test version firmware OEMPARK...");
      quickCommand("V0");
      response = readOEMPARKResponse(2000);
      if (response.length() > 0) {
        telnetClient.println("   ✓ Version: " + response);
      } else {
        telnetClient.println("   ✗ Sin respuesta de version");
      }
      
      // Test 3: API externa
      telnetClient.println("3. Test API externa...");
      if (isAPIConfigured()) {
        bool apiOk = testAPIConnection();
        telnetClient.println("   " + String(apiOk ? "✓" : "✗") + " API: " + config.api_url);
      } else {
        telnetClient.println("   - API no configurada");
      }
      
      telnetClient.println("Tests completados.");
      
    } else if (command == "api") {
      telnetClient.println("=== Estado API Externa ===");
      if (isAPIConfigured()) {
        telnetClient.println("URL: " + config.api_url);
        telnetClient.println("Tipo: " + String(config.api_url.startsWith("https://") ? "HTTPS" : "HTTP"));
        
        // Test de conectividad
        telnetClient.println("Probando conectividad...");
        bool connected = testAPIConnection();
        telnetClient.println("Estado: " + String(connected ? "✓ Conectada" : "✗ Error de conexion"));
        
        // Mostrar ultimo buffer
        String lastResponse = getLastAPIResponse();
        if (lastResponse.length() > 0) {
          telnetClient.println("Ultima respuesta:");
          telnetClient.println(lastResponse.substring(0, 200) + "...");
        }
      } else {
        telnetClient.println("API externa no configurada");
        telnetClient.println("Configure via interfaz web: http://" + ETH.localIP().toString());
      }
      
    } else if (command == "tcp") {
      telnetClient.println("=== Estado TCP Bridge ===");
      printTCPStats();
      
    } else if (command == "reboot") {
      telnetClient.println("Reiniciando gateway en 3 segundos...");
      telnetClient.println("Conexion se cerrara automaticamente.");
      telnetClient.stop();
      delay(3000);
      ESP.restart();
      
    } else if (command == "help") {
      telnetClient.println("=== Comandos Disponibles ===");
      telnetClient.println("status               - Estado completo del gateway");
      telnetClient.println("config               - Configuracion actual");
      telnetClient.println("oempark <cmd>        - Enviar comando a OEMPARK");
      telnetClient.println("test                 - Ejecutar tests de sistema");
      telnetClient.println("api                  - Estado de API externa");
      telnetClient.println("tcp                  - Estado de TCP Bridge");
      telnetClient.println("reboot               - Reiniciar gateway");
      telnetClient.println("help                 - Mostrar esta ayuda");
      telnetClient.println("");
      telnetClient.println("=== Comandos OEMPARK Comunes ===");
      telnetClient.println("S0  - Status del modulo");
      telnetClient.println("A1  - Leer direccion");
      telnetClient.println("V0  - Version firmware");
      telnetClient.println("O1  - Abrir barrera");
      telnetClient.println("R1  - Cerrar barrera");
      telnetClient.println("O3  - Pulso barrera");
      telnetClient.println("O5  - Activar molinete");
      telnetClient.println("K3  - Autorizar entrada");
      telnetClient.println("K2  - Denegar acceso");
      telnetClient.println("D1  - Leer fecha/hora");
      
    } else if (command.length() > 0) {
      telnetClient.println("Comando no reconocido: " + command);
      telnetClient.println("Escriba 'help' para ver comandos disponibles");
    }
    
    telnetClient.print("admin# ");
  }
  
  // Mostrar datos de OEMPARK en tiempo real en Telnet
  if (hasResponse() && telnetClient && telnetClient.connected()) {
    String data = readOEMPARKResponse(50); // No bloqueante
    if (data.length() > 0) {
      telnetClient.println("[OEMPARK] " + data);
      telnetClient.print("admin# ");
    }
  }
  
  // Verificar desconexion
  if (telnetClient && !telnetClient.connected()) {
    telnetClient.stop();
    Serial.println("Telnet Admin: Cliente desconectado");
  }
}

#endif