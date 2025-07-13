#ifndef OEMPARK_PROTOCOL_H
#define OEMPARK_PROTOCOL_H

#include <Arduino.h>
#include "config.h"

// Constantes del protocolo OEMPARK
#define STX 0x02  // Start of Text
#define SIB 0x0F  // End marker (según documentación)
#define ACK_STR "ACK"
#define NAK_STR "NAK"

// Buffer para comandos y respuestas
static String responseBuffer = "";
static unsigned long lastCommandTime = 0;

// Formatear comando OEMPARK completo con STX y SIB
String formatOEMPARKCommand(const String& address, const String& command, const String& params) {
  String formatted = "";
  
  // STX + ADDRESS + COMMAND + PARAMS + SIB
  formatted += char(STX);
  formatted += address;
  formatted += command;
  if (params.length() > 0) {
    formatted += params;
  }
  formatted += char(SIB);
  
  return formatted;
}

// Enviar comando OEMPARK via Serial2
void sendOEMPARKCommand(const String& command) {
  if (Serial2) {
    Serial2.print(command);
    Serial2.flush(); // Asegurar envio completo
    lastCommandTime = millis();
    Serial.println("→ OEMPARK: " + command);
  } else {
    Serial.println("ERROR: Serial2 no inicializado");
  }
}

// Leer respuesta OEMPARK con timeout (no bloqueante si timeout = 0)
String readOEMPARKResponse(unsigned long timeoutMs = 0) {
  String response = "";
  unsigned long startTime = millis();
  bool foundSTX = false;
  
  if (timeoutMs == 0) {
    // Modo no bloqueante - solo lee lo que esta disponible
    while (Serial2.available()) {
      char c = Serial2.read();
      
      if (c == STX) {
        foundSTX = true;
        response = "";
        continue;
      }
      
      if (foundSTX) {
        if (c == SIB) {
          // Fin del mensaje
          break;
        } else {
          response += c;
        }
      }
    }
  } else {
    // Modo bloqueante con timeout
    while (millis() - startTime < timeoutMs) {
      if (Serial2.available()) {
        char c = Serial2.read();
        
        if (c == STX) {
          foundSTX = true;
          response = "";
          continue;
        }
        
        if (foundSTX) {
          if (c == SIB) {
            // Fin del mensaje
            break;
          } else {
            response += c;
          }
        }
      }
      delay(1); // Pequeña pausa para no saturar CPU
    }
  }
  
  // Verificar respuestas especiales
  if (response == ACK_STR || response == NAK_STR) {
    // Respuesta de confirmacion simple
    if (response.length() > 0) {
      Serial.println("← OEMPARK: " + response);
    }
    return response;
  }
  
  // Respuesta con datos
  if (response.length() > 0) {
    Serial.println("← OEMPARK: " + response);
    return response;
  }
  
  return ""; // Sin respuesta o timeout
}

// Parsear respuesta OEMPARK a JSON
String parseOEMPARKResponseToJSON(const String& response) {
  String json = "{";
  json += "\"raw\":\"" + response + "\",";
  json += "\"timestamp\":" + String(millis()) + ",";
  
  if (response == ACK_STR) {
    json += "\"type\":\"acknowledgment\",";
    json += "\"status\":\"OK\",";
    json += "\"success\":true";
  } else if (response == NAK_STR) {
    json += "\"type\":\"negative_ack\",";
    json += "\"status\":\"ERROR\",";
    json += "\"success\":false";
  } else if (response.length() >= 4) {
    // Respuesta con datos: ADDRESS + COMMAND + DATA
    String address = response.substring(0, 2);
    String command = response.substring(2, 4);
    String data = response.substring(4);
    
    json += "\"address\":\"" + address + "\",";
    json += "\"command\":\"" + command + "\",";
    json += "\"data\":\"" + data + "\",";
    
    // Interpretacion segun comando
    if (command == "S0") {
      json += "\"type\":\"status_response\",";
      if (data.length() >= 4) {
        String status = data.substring(0, 4);
        json += "\"status_code\":\"" + status + "\",";
        if (data.length() > 4) {
          json += "\"identifier\":\"" + data.substring(4) + "\",";
          json += "\"event_type\":\"card_read\",";
        } else {
          json += "\"event_type\":\"status_only\",";
        }
      }
    } else if (command == "A1") {
      json += "\"type\":\"address_response\",";
      json += "\"device_address\":\"" + data + "\",";
    } else if (command == "V0") {
      json += "\"type\":\"version_response\",";
      json += "\"firmware_version\":\"" + data + "\",";
    } else if (command == "D1") {
      json += "\"type\":\"datetime_response\",";
      if (data.length() >= 12) {
        // DDMMYYHHMMSS format
        json += "\"day\":\"" + data.substring(0, 2) + "\",";
        json += "\"month\":\"" + data.substring(2, 4) + "\",";
        json += "\"year\":\"" + data.substring(4, 6) + "\",";
        json += "\"hour\":\"" + data.substring(6, 8) + "\",";
        json += "\"minute\":\"" + data.substring(8, 10) + "\",";
        json += "\"second\":\"" + data.substring(10, 12) + "\",";
      }
    } else {
      json += "\"type\":\"other_response\",";
    }
    
    json += "\"success\":true";
  } else if (response.length() > 0) {
    // Respuesta corta no estándar
    json += "\"type\":\"short_response\",";
    json += "\"success\":true";
  } else {
    // Sin respuesta o timeout
    json += "\"type\":\"timeout\",";
    json += "\"status\":\"NO_RESPONSE\",";
    json += "\"success\":false";
  }
  
  json += "}";
  return json;
}

// Validar formato de comando OEMPARK
bool isValidOEMPARKCommand(const String& command) {
  // Verificar longitud mínima (address + command = 4 chars mínimo)
  if (command.length() < 4) return false;
  
  // Verificar que la dirección sea numérica
  String address = command.substring(0, 2);
  for (int i = 0; i < address.length(); i++) {
    if (!isDigit(address.charAt(i))) return false;
  }
  
  // Verificar comando válido (letras y números)
  String cmd = command.substring(2, 4);
  for (int i = 0; i < cmd.length(); i++) {
    char c = cmd.charAt(i);
    if (!isAlphaNumeric(c)) return false;
  }
  
  return true;
}

// Comandos OEMPARK predefinidos más comunes
namespace OEMPARKCommands {
  String status(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "S0", "");
  }
  
  String readAddress(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "A1", "");
  }
  
  String getVersion(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "V0", "");
  }
  
  String openBarrier(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "O1", "");
  }
  
  String closeBarrier(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "R1", "");
  }
  
  String pulseBarrier(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "O3", "");
  }
  
  String activateTurnstile(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "O5", "");
  }
  
  String authorizeEntry(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "K3", "");
  }
  
  String denyAccess(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "K2", "");
  }
  
  String getDateTime(const String& addr = "02") {
    return formatOEMPARKCommand(addr, "D1", "");
  }
  
  String setAddress(const String& addr, const String& newAddr) {
    return formatOEMPARKCommand(addr, "A0", newAddr);
  }
  
  String setDateTime(const String& addr, const String& datetime) {
    // Format: DDMMYYHHMMSS
    return formatOEMPARKCommand(addr, "D0", datetime);
  }
}

// Estructura para datos parseados de OEMPARK
struct OEMPARKData {
  String device_id;
  String timestamp;
  String address;
  String command;
  String status_code;
  String identifier;
  String raw_response;
  String event_type;
  bool success;
};

// Parsear respuesta a estructura de datos
OEMPARKData parseOEMPARKToStruct(const String& response) {
  OEMPARKData data;
  data.device_id = config.device_name;
  data.timestamp = String(millis() / 1000);
  data.raw_response = response;
  data.success = false;
  
  if (response == ACK_STR) {
    data.event_type = "acknowledgment";
    data.status_code = "OK";
    data.success = true;
  } else if (response == NAK_STR) {
    data.event_type = "negative_ack";
    data.status_code = "ERROR";
    data.success = false;
  } else if (response.length() >= 4) {
    data.address = response.substring(0, 2);
    data.command = response.substring(2, 4);
    
    String responseData = response.substring(4);
    
    if (data.command == "S0") {
      data.event_type = "status_response";
      if (responseData.length() >= 4) {
        data.status_code = responseData.substring(0, 4);
        if (responseData.length() > 4) {
          data.identifier = responseData.substring(4);
          data.event_type = "card_read";
        }
      }
      data.success = true;
    } else if (data.command == "A1") {
      data.event_type = "address_response";
      data.status_code = responseData;
      data.success = true;
    } else if (data.command == "V0") {
      data.event_type = "version_response";
      data.status_code = responseData;
      data.success = true;
    } else {
      data.event_type = "other_response";
      data.status_code = responseData;
      data.success = true;
    }
  }
  
  return data;
}

// Función para envío rápido de comandos comunes
void quickCommand(const String& cmd) {
  String address = config.oempark_address;
  String fullCmd = formatOEMPARKCommand(address, cmd, "");
  sendOEMPARKCommand(fullCmd);
}

// Debug: mostrar comando en formato hexadecimal
void debugCommand(const String& command) {
  Serial.print("HEX: ");
  for (int i = 0; i < command.length(); i++) {
    Serial.printf("%02X ", command.charAt(i));
  }
  Serial.println();
}

// Verificar si hay respuesta pendiente
bool hasResponse() {
  return Serial2.available() > 0;
}

// Limpiar buffer de Serial2
void clearSerialBuffer() {
  while (Serial2.available()) {
    Serial2.read();
  }
}

#endif