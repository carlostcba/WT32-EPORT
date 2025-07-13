#include "config.h"

Config config;

void loadConfiguration() {
  // Valores por defecto para gateway OEMPARK
  config.tcp_port = 23;
  config.telnet_port = 2323;
  config.baud_rate = 9600; // Baudrate del modulo OEMPARK
  config.api_url = ""; // Sin API por defecto
  config.ssl_enabled = false;
  config.aes_enabled = false;
  config.device_name = "WT32-ETH01-OEMPARK-Gateway";
  config.oempark_address = "00"; // Direccion por defecto del modulo OEMPARK
  
  // Cargar configuracion desde SPIFFS si existe
  if (SPIFFS.begin(true)) {
    Serial.println("SPIFFS iniciado correctamente");
    
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        if (size > 0 && size < 2048) { // Verificar tamaño razonable
          DynamicJsonDocument doc(2048);
          DeserializationError error = deserializeJson(doc, configFile);
          
          if (!error) {
            config.tcp_port = doc["tcp_port"] | config.tcp_port;
            config.telnet_port = doc["telnet_port"] | config.telnet_port;
            config.baud_rate = doc["baud_rate"] | config.baud_rate;
            config.api_url = doc["api_url"] | config.api_url;
            config.ssl_enabled = doc["ssl_enabled"] | config.ssl_enabled;
            config.aes_enabled = doc["aes_enabled"] | config.aes_enabled;
            config.device_name = doc["device_name"] | config.device_name;
            config.oempark_address = doc["oempark_address"] | config.oempark_address;
            
            Serial.println("Configuracion cargada desde SPIFFS");
          } else {
            Serial.println("Error parseando JSON de configuracion");
          }
        }
        configFile.close();
      }
    } else {
      Serial.println("Archivo config.json no existe, usando valores por defecto");
    }
  } else {
    Serial.println("Error iniciando SPIFFS");
  }
  
  // Mostrar configuracion cargada
  Serial.println("\n=== Configuracion Actual ===");
  Serial.println("Device Name: " + config.device_name);
  Serial.println("TCP Port: " + String(config.tcp_port));
  Serial.println("Telnet Port: " + String(config.telnet_port));
  Serial.println("Baud Rate: " + String(config.baud_rate));
  Serial.println("API URL: " + (config.api_url.length() > 0 ? config.api_url : "(no configurada)"));
  Serial.println("OEMPARK Address: " + config.oempark_address);
  Serial.println("SSL Enabled: " + String(config.ssl_enabled ? "Si" : "No"));
}

void saveConfiguration() {
  if (SPIFFS.begin(true)) {
    File configFile = SPIFFS.open("/config.json", "w");
    if (configFile) {
      DynamicJsonDocument doc(2048);
      
      doc["tcp_port"] = config.tcp_port;
      doc["telnet_port"] = config.telnet_port;
      doc["baud_rate"] = config.baud_rate;
      doc["api_url"] = config.api_url;
      doc["ssl_enabled"] = config.ssl_enabled;
      doc["aes_enabled"] = config.aes_enabled;
      doc["device_name"] = config.device_name;
      doc["oempark_address"] = config.oempark_address;
      
      if (serializeJson(doc, configFile) > 0) {
        Serial.println("Configuracion guardada en SPIFFS");
      } else {
        Serial.println("Error escribiendo configuracion");
      }
      configFile.close();
    } else {
      Serial.println("Error abriendo archivo de configuracion para escritura");
    }
  }
}

void setupEthernet() {
  Serial.println("Configurando Ethernet WT32-ETH01...");
  
  // Configuracion especifica para WT32-ETH01
  ETH.begin();
  
  // Esperar conexion Ethernet con timeout
  Serial.print("Conectando");
  int timeout = 0;
  while (!ETH.linkUp() && timeout < 30) {
    delay(1000);
    Serial.print(".");
    timeout++;
  }
  Serial.println();
  
  if (ETH.linkUp()) {
    Serial.println("✅ Ethernet conectado exitosamente");
    Serial.println("📍 Configuracion de red:");
    Serial.println("   IP: " + ETH.localIP().toString());
    Serial.println("   Gateway: " + ETH.gatewayIP().toString());
    Serial.println("   DNS: " + ETH.dnsIP().toString());
    Serial.println("   MAC: " + ETH.macAddress());
    
    // Verificar conectividad basica
    Serial.println("🔍 Verificando conectividad...");
    if (ETH.gatewayIP() != IPAddress(0, 0, 0, 0)) {
      Serial.println("✅ Gateway accesible");
    } else {
      Serial.println("⚠️  Sin gateway configurado");
    }
  } else {
    Serial.println("❌ ERROR: No se pudo establecer conexion Ethernet");
    Serial.println("🔧 Verificar:");
    Serial.println("   - Cable Ethernet conectado");
    Serial.println("   - Switch/Router funcionando");
    Serial.println("   - Alimentacion 3.3V estable");
    Serial.println("   - Conexiones del WT32-ETH01");
  }
}