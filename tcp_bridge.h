#ifndef TCP_BRIDGE_H
#define TCP_BRIDGE_H

#include <WiFi.h>
#include <ETH.h>
#include "config.h"
#include "oempark_protocol.h"

WiFiServer tcpServer(23);
WiFiClient tcpClient;

void setupTCPBridge() {
  // Configurar Serial2 para TTL con pines específicos del WT32-ETH01
  Serial2.begin(config.baud_rate, SERIAL_8N1, 16, 17); // RX=16, TX=17
  delay(100);
  
  // Verificar si Serial2 se inicializó correctamente
  if (!Serial2) {
    Serial.println("ERROR: No se pudo inicializar Serial2");
    return;
  }
  
  // Limpiar buffer al inicio
  clearSerialBuffer();
  
  // Iniciar servidor TCP en puerto configurado
  tcpServer.begin(config.tcp_port);
  Serial.println("TCP Bridge iniciado en puerto " + String(config.tcp_port));
  Serial.println("Serial2 TTL configurado: " + String(config.baud_rate) + " bps (GPIO16/17)");
  
  // Comando de prueba inicial
  delay(500);
  quickCommand("S0"); // Solicitar status del módulo OEMPARK
}

void handleTCPBridge() {
  // Verificar nuevas conexiones TCP
  if (tcpServer.hasClient()) {
    if (tcpClient && tcpClient.connected()) {
      Serial.println("TCP: Desconectando cliente anterior");
      tcpClient.stop();
    }
    tcpClient = tcpServer.available();
    if (tcpClient) {
      Serial.println("TCP: Cliente conectado desde " + tcpClient.remoteIP().toString());
      
      // Mensaje de bienvenida
      tcpClient.println("=== WT32-ETH01 OEMPARK Gateway ===");
      tcpClient.println("Conectado al módulo OEMPARK dirección: " + config.oempark_address);
      tcpClient.println("Formato: COMANDO (ej: S0, A1, O1, K3)");
      tcpClient.println("El gateway agregará STX, dirección y SIB automáticamente");
      tcpClient.println("===========================================");
    }
  }
  
  // Transferir respuestas de OEMPARK a TCP
  if (hasResponse() && tcpClient && tcpClient.connected()) {
    String response = readOEMPARKResponse(50); // Timeout corto no bloqueante
    if (response.length() > 0) {
      // Enviar respuesta cruda al cliente TCP
      tcpClient.println(response);
      Serial.println("OEMPARK->TCP: " + response);
    }
  }
  
  // Transferir comandos de TCP a OEMPARK
  if (tcpClient && tcpClient.connected() && tcpClient.available()) {
    String command = tcpClient.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.println("TCP->OEMPARK: " + command);
      
      // Verificar si es comando directo con dirección o solo comando
      if (command.length() >= 4 && isValidOEMPARKCommand(command)) {
        // Comando completo con dirección (ej: "02S0")
        String formatted = formatOEMPARKCommand(command.substring(0, 2), 
                                                command.substring(2, 4), 
                                                command.substring(4));
        sendOEMPARKCommand(formatted);
      } else if (command.length() >= 2) {
        // Solo comando, agregar dirección por defecto (ej: "S0")
        String cmd = command.substring(0, 2);
        String params = command.substring(2);
        String formatted = formatOEMPARKCommand(config.oempark_address, cmd, params);
        sendOEMPARKCommand(formatted);
      } else {
        tcpClient.println("ERROR: Comando inválido. Ejemplos: S0, A1, O1, K3");
        return; // CORREGIDO: Usar return en lugar de continue
      }
      
      // Esperar respuesta del OEMPARK con timeout
      unsigned long timeout = millis() + 3000; // 3 segundos
      String response = "";
      
      while (millis() < timeout) {
        response = readOEMPARKResponse(100);
        if (response.length() > 0) {
          tcpClient.println(response);
          Serial.println("OEMPARK->TCP: " + response);
          break;
        }
        delay(10);
      }
      
      if (response.length() == 0) {
        tcpClient.println("TIMEOUT: Sin respuesta del módulo OEMPARK");
        Serial.println("TCP: Timeout esperando respuesta OEMPARK");
      }
    }
  }
  
  // Verificar desconexión TCP
  if (tcpClient && !tcpClient.connected()) {
    tcpClient.stop();
    Serial.println("TCP: Cliente desconectado");
  }
}

// Obtener estado de conexión TCP
bool isTCPClientConnected() {
  return tcpClient && tcpClient.connected();
}

// Enviar mensaje a cliente TCP si está conectado
void sendToTCPClient(const String& message) {
  if (tcpClient && tcpClient.connected()) {
    tcpClient.println(message);
  }
}

// Estadísticas del bridge TCP
void printTCPStats() {
  Serial.println("=== TCP Bridge Stats ===");
  Serial.println("Puerto: " + String(config.tcp_port));
  Serial.println("Cliente conectado: " + String(isTCPClientConnected() ? "Sí" : "No"));
  if (isTCPClientConnected()) {
    Serial.println("IP Cliente: " + tcpClient.remoteIP().toString());
  }
  Serial.println("Baudrate TTL: " + String(config.baud_rate));
  Serial.println("Dirección OEMPARK: " + config.oempark_address);
}

#endif