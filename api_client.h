#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "oempark_protocol.h"

HTTPClient http;
WiFiClientSecure httpsClient;

// Buffer para datos de API externa
String externalAPIBuffer = "";

// Forward declaration
void processAPIResponse(const String& apiResponse);

// Formatear datos OEMPARK para API externa
String formatOEMPARKForAPI(const OEMPARKData& data) {
  String json = "{";
  json += "\"device_id\":\"" + data.device_id + "\",";
  json += "\"timestamp\":\"" + data.timestamp + "\",";
  json += "\"address\":\"" + data.address + "\",";
  json += "\"command\":\"" + data.command + "\",";
  json += "\"status_code\":\"" + data.status_code + "\",";
  json += "\"identifier\":\"" + data.identifier + "\",";
  json += "\"raw_response\":\"" + data.raw_response + "\",";
  json += "\"event_type\":\"" + data.event_type + "\",";
  json += "\"success\":" + String(data.success ? "true" : "false");
  json += "}";
  return json;
}

// Enviar datos OEMPARK a API externa
bool sendOEMPARKDataToAPI(const String& oemResponse) {
  if (config.api_url.length() == 0) {
    Serial.println("API Client: No hay URL configurada");
    return false;
  }
  
  OEMPARKData data = parseOEMPARKToStruct(oemResponse);
  String jsonPayload = formatOEMPARKForAPI(data);
  
  bool isHTTPS = config.api_url.startsWith("https://");
  
  if (isHTTPS) {
    httpsClient.setInsecure(); // Para desarrollo, usar certificados en producción
    http.begin(httpsClient, config.api_url);
  } else {
    http.begin(config.api_url);
  }
  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "WT32-ETH01-OEMPARK-Gateway/1.0");
  http.addHeader("X-Device-ID", config.device_name);
  http.setTimeout(5000); // 5 segundos timeout
  
  int httpCode = http.POST(jsonPayload);
  String response = "";
  
  if (httpCode > 0) {
    response = http.getString();
  }
  
  http.end();
  
  Serial.printf("OEMPARK->API: %s (HTTP %d)\n", jsonPayload.c_str(), httpCode);
  
  if (httpCode == 200 && response.length() > 0) {
    processAPIResponse(response);
    return true;
  } else if (httpCode != 200) {
    Serial.printf("API Error: HTTP %d\n", httpCode);
  }
  
  return false;
}

// Procesar respuesta de API externa
void processAPIResponse(const String& apiResponse) {
  // Buscar comandos en la respuesta JSON
  if (apiResponse.indexOf("\"command\"") >= 0) {
    String command = "";
    int start = apiResponse.indexOf("\"command\":\"") + 11;
    int end = apiResponse.indexOf("\"", start);
    if (end > start) {
      command = apiResponse.substring(start, end);
      
      // Validar y enviar comando OEMPARK
      if (command.length() >= 2) {
        if (isValidOEMPARKCommand(config.oempark_address + command)) {
          quickCommand(command);
          Serial.println("API->OEMPARK command: " + command);
        } else {
          Serial.println("API: Comando inválido recibido: " + command);
        }
      }
    }
  }
  
  // Buscar acciones específicas en respuesta de API
  if (apiResponse.indexOf("\"action\"") >= 0) {
    String action = "";
    int start = apiResponse.indexOf("\"action\":\"") + 10;
    int end = apiResponse.indexOf("\"", start);
    if (end > start) {
      action = apiResponse.substring(start, end);
      
      // Mapear acciones a comandos OEMPARK
      String oemCommand = "";
      if (action == "open_barrier") {
        oemCommand = "O1"; // Activar barrera por estado
      } else if (action == "close_barrier") {
        oemCommand = "R1"; // Desactivar barrera
      } else if (action == "pulse_barrier") {
        oemCommand = "O3"; // Pulso en barrera
      } else if (action == "activate_turnstile") {
        oemCommand = "O5"; // Activar molinete
      } else if (action == "authorize_entry") {
        oemCommand = "K3"; // Autorización de ingreso
      } else if (action == "deny_access") {
        oemCommand = "K2"; // Identificación no encontrada
      } else if (action == "get_status") {
        oemCommand = "S0"; // Solicitar status
      } else if (action == "get_version") {
        oemCommand = "V0"; // Solicitar versión
      }
      
      if (oemCommand.length() > 0) {
        quickCommand(oemCommand);
        Serial.println("API Action->OEMPARK: " + action + " -> " + oemCommand);
      } else {
        Serial.println("API: Acción desconocida: " + action);
      }
    }
  }
  
  // Almacenar respuesta para consultas
  externalAPIBuffer = apiResponse;
}

// Consultar API externa con GET
bool queryExternalAPI(const String& endpoint, const String& params = "") {
  if (config.api_url.length() == 0) {
    Serial.println("API Client: No hay URL configurada para consulta");
    return false;
  }
  
  String fullURL = config.api_url;
  if (endpoint.length() > 0) {
    if (!fullURL.endsWith("/")) fullURL += "/";
    fullURL += endpoint;
  }
  if (params.length() > 0) {
    fullURL += "?" + params;
  }
  
  bool isHTTPS = fullURL.startsWith("https://");
  
  if (isHTTPS) {
    httpsClient.setInsecure();
    http.begin(httpsClient, fullURL);
  } else {
    http.begin(fullURL);
  }
  
  http.addHeader("User-Agent", "WT32-ETH01-OEMPARK-Gateway/1.0");
  http.addHeader("X-Device-ID", config.device_name);
  http.setTimeout(5000);
  
  int httpCode = http.GET();
  String response = "";
  
  if (httpCode > 0) {
    response = http.getString();
  }
  
  http.end();
  
  Serial.printf("API Query: %s (HTTP %d)\n", fullURL.c_str(), httpCode);
  
  if (httpCode == 200 && response.length() > 0) {
    processAPIResponse(response);
    externalAPIBuffer = response;
    return true;
  } else {
    Serial.printf("API Query Error: HTTP %d\n", httpCode);
  }
  
  return false;
}

void handleAPIClient() {
  // Leer respuestas del módulo OEMPARK y reenviar a API externa automáticamente
  if (hasResponse()) {
    String oemResponse = readOEMPARKResponse(50); // No bloqueante
    
    if (oemResponse.length() > 0 && !oemResponse.startsWith("API_")) {
      Serial.println("OEMPARK->API Processing: " + oemResponse);
      
      // Parsear respuesta para determinar si es importante
      OEMPARKData data = parseOEMPARKToStruct(oemResponse);
      
      // Enviar eventos importantes a la API externa
      if (data.event_type == "card_read" || 
          data.event_type == "status_response" || 
          data.event_type == "acknowledgment" ||
          data.identifier.length() > 0) {
        
        // Solo enviar si la API está configurada
        if (config.api_url.length() > 0) {
          sendOEMPARKDataToAPI(oemResponse);
        }
      } else {
        Serial.println("API Client: Respuesta no crítica, no enviada a API");
      }
    }
  }
  
  // Heartbeat periódico a API externa si está configurada
  static unsigned long lastHeartbeat = 0;
  if (config.api_url.length() > 0 && millis() - lastHeartbeat > 300000) { // 5 minutos
    // Enviar heartbeat con status del gateway
    String heartbeatData = "{";
    heartbeatData += "\"device_id\":\"" + config.device_name + "\",";
    heartbeatData += "\"timestamp\":\"" + String(millis() / 1000) + "\",";
    heartbeatData += "\"event_type\":\"heartbeat\",";
    heartbeatData += "\"status\":\"online\",";
    heartbeatData += "\"uptime\":" + String(millis() / 1000) + ",";
    heartbeatData += "\"free_heap\":" + String(ESP.getFreeHeap());
    heartbeatData += "}";
    
    // Enviar heartbeat (sin esperar respuesta crítica)
    bool isHTTPS = config.api_url.startsWith("https://");
    
    if (isHTTPS) {
      httpsClient.setInsecure();
      http.begin(httpsClient, config.api_url);
    } else {
      http.begin(config.api_url);
    }
    
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-ID", config.device_name);
    http.setTimeout(3000);
    
    int httpCode = http.POST(heartbeatData);
    if (httpCode == 200) {
      String response = http.getString();
      if (response.length() > 0) {
        processAPIResponse(response); // Procesar comandos de respuesta
      }
    }
    http.end();
    
    lastHeartbeat = millis();
    Serial.println("API: Heartbeat enviado");
  }
}

// Obtener último buffer de API (para interfaces web)
String getLastAPIResponse() {
  return externalAPIBuffer;
}

// Verificar si API externa está configurada
bool isAPIConfigured() {
  return config.api_url.length() > 0;
}

// Test de conectividad con API externa
bool testAPIConnection() {
  if (!isAPIConfigured()) {
    Serial.println("API: No configurada");
    return false;
  }
  
  String testData = "{\"device_id\":\"" + config.device_name + "\",\"event_type\":\"test\"}";
  
  bool isHTTPS = config.api_url.startsWith("https://");
  
  if (isHTTPS) {
    httpsClient.setInsecure();
    http.begin(httpsClient, config.api_url);
  } else {
    http.begin(config.api_url);
  }
  
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  
  int httpCode = http.POST(testData);
  http.end();
  
  Serial.printf("API Test: HTTP %d\n", httpCode);
  return httpCode == 200;
}

#endif