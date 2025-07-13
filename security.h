#ifndef SECURITY_H
#define SECURITY_H

#include <WiFiClientSecure.h>
#include "config.h"

// Cliente HTTPS para conexiones seguras
WiFiClientSecure secureClient;

void setupSSL() {
  // Configuracion basica SSL para desarrollo
  // En produccion usar certificados validos
  secureClient.setInsecure();
  Serial.println("SSL: Configuracion basica habilitada");
}

// Funcion basica para validar URLs
bool isValidURL(const String& url) {
  return url.startsWith("http://") || url.startsWith("https://");
}

// Sanitizar comandos OEMPARK para evitar inyecciones
String sanitizeOEMPARKCommand(const String& command) {
  String sanitized = "";
  
  // Solo permitir caracteres alfanumericos y algunos simbolos
  for (int i = 0; i < command.length() && i < 50; i++) {
    char c = command.charAt(i);
    if (isAlphaNumeric(c) || c == ',' || c == ':' || c == '-') {
      sanitized += c;
    }
  }
  
  return sanitized;
}

// Log de accesos para auditoria basica
void logAccess(const String& source, const String& command) {
  Serial.printf("[AUDIT] %s: %s -> OEMPARK\n", source.c_str(), command.c_str());
}

// Verificacion basica de rate limiting
static unsigned long lastCommand = 0;
static int commandCount = 0;

bool checkRateLimit() {
  unsigned long now = millis();
  
  // Reset contador cada 10 segundos
  if (now - lastCommand > 10000) {
    commandCount = 0;
    lastCommand = now;
    return true;
  }
  
  // Maximo 20 comandos por 10 segundos
  if (commandCount >= 20) {
    Serial.println("[SECURITY] Rate limit exceeded");
    return false;
  }
  
  commandCount++;
  return true;
}

#endif