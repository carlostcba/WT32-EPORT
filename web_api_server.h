#ifndef WEB_API_SERVER_H
#define WEB_API_SERVER_H

#include <WebServer.h>
#include <Update.h>
#include "config.h"
#include "tcp_bridge.h"
#include "oempark_protocol.h"

// SERVIDOR WEB UNIFICADO - Maneja tanto API REST como Interfaz Web
WebServer server(80);

// HTML de la interfaz web
const char* HTML_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WT32-ETH01 OEMPARK Gateway</title>
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; margin: 20px; background: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input, select, button { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 3px; }
        button { background: #4CAF50; color: white; border: none; cursor: pointer; }
        button:hover { background: #45a049; }
        .btn-secondary { background: #2196F3; }
        .btn-secondary:hover { background: #1976D2; }
        .btn-danger { background: #f44336; }
        .btn-danger:hover { background: #d32f2f; }
        .status { background: #e8f5e9; padding: 15px; margin-bottom: 20px; border-radius: 5px; border-left: 4px solid #4CAF50; }
        .console { background: #1e1e1e; color: #00ff00; padding: 15px; height: 300px; overflow-y: scroll; font-family: 'Courier New', monospace; font-size: 12px; border-radius: 3px; }
        .command-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; margin: 15px 0; }
        .cmd-btn { background: #607d8b; margin: 5px; }
        .cmd-btn:hover { background: #546e7a; }
        h2 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 5px; }
        h3 { color: #555; }
    </style>
</head>
<body>
    <div class="container">
        <h1>WT32-ETH01 OEMPARK Gateway</h1>
        
        <div class="status">
            <h3>Estado del Sistema</h3>
            <p><strong>IP:</strong> <span id="ip">-</span></p>
            <p><strong>Puerto TCP:</strong> <span id="tcp_port">-</span></p>
            <p><strong>Velocidad TTL:</strong> <span id="baud_rate">-</span> bps</p>
            <p><strong>Cliente TCP:</strong> <span id="tcp_connected">-</span></p>
            <p><strong>Memoria libre:</strong> <span id="free_heap">-</span> bytes</p>
            <p><strong>Uptime:</strong> <span id="uptime">-</span> segundos</p>
        </div>

        <h2>Configuracion</h2>
        <form id="configForm">
            <div class="form-group">
                <label for="tcp_port">Puerto TCP:</label>
                <input type="number" id="tcp_port" name="tcp_port" value="23" min="1" max="65535">
            </div>
            
            <div class="form-group">
                <label for="baud_rate">Velocidad TTL:</label>
                <select id="baud_rate" name="baud_rate">
                    <option value="9600" selected>9600</option>
                    <option value="19200">19200</option>
                    <option value="38400">38400</option>
                    <option value="57600">57600</option>
                    <option value="115200">115200</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="api_url">URL API Externa:</label>
                <input type="text" id="api_url" name="api_url" placeholder="http://ejemplo.com/api">
            </div>
            
            <div class="form-group">
                <label for="oempark_addr">Direccion OEMPARK:</label>
                <input type="text" id="oempark_addr" name="oempark_addr" value="02" maxlength="2">
            </div>
            
            <button type="submit">Guardar Configuracion</button>
        </form>

        <h2>Comandos OEMPARK</h2>
        <div class="command-grid">
            <button class="cmd-btn" onclick="sendOEMCommand('S0')">Status (S0)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('A1')">Leer Dir (A1)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('V0')">Version (V0)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('O1')">Abrir Barrera (O1)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('R1')">Cerrar Barrera (R1)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('O3')">Pulso Barrera (O3)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('O5')">Molinete (O5)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('K3')">Autorizar (K3)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('K2')">Denegar (K2)</button>
            <button class="cmd-btn" onclick="sendOEMCommand('D1')">Fecha/Hora (D1)</button>
        </div>
        
        <div style="margin-top: 15px;">
            <label for="customCommand">Comando Personalizado:</label>
            <input type="text" id="customCommand" placeholder="Ej: S0, A1, O3, etc..." style="width: 70%; display: inline-block;">
            <button onclick="sendCustomCommand()" style="width: 25%; display: inline-block; margin-left: 5px;">Enviar</button>
        </div>

        <h2>Herramientas</h2>
        <button class="btn-secondary" onclick="testSerial()">Test Serial</button>
        <button class="btn-secondary" onclick="queryStatus()">Consultar Estado OEMPARK</button>
        <button class="btn-danger" onclick="rebootDevice()">Reiniciar Gateway</button>

        <h2>Consola en Tiempo Real</h2>
        <div id="console" class="console"></div>
        <button onclick="clearConsole()" style="margin-top: 10px;">Limpiar Consola</button>

        <h2>Actualizacion OTA</h2>
        <form method="POST" action="/update" enctype="multipart/form-data">
            <div class="form-group">
                <label for="firmware">Archivo Firmware (.bin):</label>
                <input type="file" id="firmware" name="firmware" accept=".bin">
            </div>
            <button type="submit" class="btn-secondary">Actualizar Firmware</button>
        </form>
    </div>

    <script>
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('ip').textContent = data.ip || '-';
                    document.getElementById('tcp_port').textContent = data.tcp_port || '-';
                    document.getElementById('baud_rate').textContent = data.baud_rate || '-';
                    document.getElementById('tcp_connected').textContent = data.tcp_connected ? 'Conectado' : 'Desconectado';
                    document.getElementById('free_heap').textContent = data.free_heap || '-';
                    document.getElementById('uptime').textContent = data.uptime || '-';
                })
                .catch(err => addToConsole('Error actualizando estado: ' + err));
        }

        function sendOEMCommand(command) {
            fetch('/api/ttl', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: command})
            })
            .then(response => response.json())
            .then(data => {
                addToConsole('Comando enviado: ' + command);
                addToConsole('Respuesta OEMPARK: ' + JSON.stringify(data));
            })
            .catch(err => addToConsole('Error: ' + err));
        }

        function sendCustomCommand() {
            let command = document.getElementById('customCommand').value.trim();
            if (command.length >= 1) {
                fetch('/api/ttl', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({command: command})
                })
                .then(response => response.json())
                .then(data => {
                    addToConsole('Comando personalizado: ' + command);
                    addToConsole('Respuesta: ' + JSON.stringify(data));
                    document.getElementById('customCommand').value = '';
                })
                .catch(err => addToConsole('Error: ' + err));
            } else {
                addToConsole('Ingrese un comando valido');
            }
        }

        function testSerial() {
            fetch('/test_serial', {method: 'POST'})
                .then(response => response.text())
                .then(data => addToConsole('Test Serial: ' + data));
        }

        function queryStatus() {
            fetch('/api/ttl/query?cmd=S0')
                .then(response => response.json())
                .then(data => {
                    addToConsole('Estado OEMPARK: ' + JSON.stringify(data));
                })
                .catch(err => addToConsole('Error consultando: ' + err));
        }

        function rebootDevice() {
            if(confirm('Reiniciar el gateway WT32-ETH01?')) {
                fetch('/reboot', {method: 'POST'})
                    .then(() => addToConsole('Reiniciando gateway...'));
            }
        }

        function addToConsole(message) {
            const console = document.getElementById('console');
            const timestamp = new Date().toLocaleTimeString();
            console.innerHTML += '[' + timestamp + '] ' + message + '\n';
            console.scrollTop = console.scrollHeight;
        }

        function clearConsole() {
            document.getElementById('console').innerHTML = '';
        }

        // Configuracion
        document.getElementById('configForm').addEventListener('submit', function(e) {
            e.preventDefault();
            const formData = new FormData(this);
            const data = Object.fromEntries(formData);
            
            fetch('/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: new URLSearchParams(data)
            })
            .then(response => response.text())
            .then(result => {
                addToConsole(result);
                setTimeout(updateStatus, 1000);
            })
            .catch(err => addToConsole('Error guardando config: ' + err));
        });

        // Actualizacion automatica cada 5 segundos
        setInterval(updateStatus, 5000);
        
        // Polling de datos TTL cada 2 segundos
        setInterval(function() {
            fetch('/api/ttl')
                .then(response => {
                    if (response.status === 200) {
                        return response.json();
                    }
                    return null;
                })
                .then(data => {
                    if (data && data.raw && data.raw !== 'timeout') {
                        addToConsole('OEMPARK: ' + data.raw);
                    }
                })
                .catch(() => {}); // Silenciar errores de polling
        }, 2000);

        // Cargar estado inicial
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

void setupWebAPIServer() {
  // CORS headers para desarrollo
  server.onNotFound([]() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    if (server.method() == HTTP_OPTIONS) {
      server.send(200);
      return;
    }
    server.send(404, "text/plain", "Not Found");
  });
  
  // ========== INTERFAZ WEB ==========
  // Pagina principal
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", HTML_PAGE);
  });

  // Estado del sistema para interfaz web
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"ip\":\"" + ETH.localIP().toString() + "\",";
    json += "\"tcp_port\":" + String(config.tcp_port) + ",";
    json += "\"baud_rate\":" + String(config.baud_rate) + ",";
    json += "\"tcp_connected\":" + String(isTCPClientConnected() ? "true" : "false") + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Configuracion
  server.on("/config", HTTP_POST, []() {
    bool changed = false;
    
    if (server.hasArg("tcp_port")) {
      int newPort = server.arg("tcp_port").toInt();
      if (newPort != config.tcp_port && newPort > 0 && newPort <= 65535) {
        config.tcp_port = newPort;
        changed = true;
      }
    }
    
    if (server.hasArg("baud_rate")) {
      long newBaud = server.arg("baud_rate").toInt();
      if (newBaud != config.baud_rate) {
        config.baud_rate = newBaud;
        Serial2.end();
        delay(100);
        Serial2.begin(config.baud_rate, SERIAL_8N1, 16, 17);
        Serial.println("Serial2 reconfigurado a " + String(config.baud_rate) + " bps");
        changed = true;
      }
    }
    
    if (server.hasArg("api_url")) {
      String newURL = server.arg("api_url");
      if (newURL != config.api_url) {
        config.api_url = newURL;
        changed = true;
      }
    }
    
    if (server.hasArg("oempark_addr")) {
      String newAddr = server.arg("oempark_addr");
      if (newAddr != config.oempark_address) {
        config.oempark_address = newAddr;
        changed = true;
      }
    }
    
    if (changed) {
      saveConfiguration();
      server.send(200, "text/plain", "Configuracion guardada correctamente");
    } else {
      server.send(200, "text/plain", "Sin cambios en la configuracion");
    }
  });

  // Test serial
  server.on("/test_serial", HTTP_POST, []() {
    String testCmd = formatOEMPARKCommand(config.oempark_address, "S0", "");
    sendOEMPARKCommand(testCmd);
    server.send(200, "text/plain", "Comando de prueba enviado: " + testCmd);
  });

  // Reiniciar
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/plain", "Reiniciando gateway en 3 segundos...");
    delay(3000);
    ESP.restart();
  });

  // ========== API REST ==========
  // GET /api/ttl - Leer datos TTL no bloqueante
  server.on("/api/ttl", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String response = readOEMPARKResponse(100); // Timeout 100ms no bloqueante
    if (response.length() > 0) {
      String json = parseOEMPARKResponseToJSON(response);
      server.send(200, "application/json", json);
      Serial.println("API GET->OEMPARK: " + response);
    } else {
      server.send(204, "application/json", "{\"status\":\"no_data\",\"raw\":\"timeout\"}");
    }
  });

  // POST /api/ttl - Enviar comando OEMPARK
  server.on("/api/ttl", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String address = config.oempark_address; // Por defecto
    String command = "";
    String params = "";
    
    // Parsear desde form data o JSON
    if (server.hasArg("address")) address = server.arg("address");
    if (server.hasArg("command")) command = server.arg("command");
    if (server.hasArg("params")) params = server.arg("params");
    
    // Parsear JSON si viene en body
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      if (body.indexOf("{") >= 0) {
        // Parseo manual basico de JSON
        if (body.indexOf("\"address\"") >= 0) {
          int start = body.indexOf("\"address\":\"") + 11;
          int end = body.indexOf("\"", start);
          if (end > start) address = body.substring(start, end);
        }
        if (body.indexOf("\"command\"") >= 0) {
          int start = body.indexOf("\"command\":\"") + 11;
          int end = body.indexOf("\"", start);
          if (end > start) command = body.substring(start, end);
        }
        if (body.indexOf("\"params\"") >= 0) {
          int start = body.indexOf("\"params\":\"") + 10;
          int end = body.indexOf("\"", start);
          if (end > start) params = body.substring(start, end);
        }
      }
    }
    
    if (command.length() > 0) {
      String formatted = formatOEMPARKCommand(address, command, params);
      sendOEMPARKCommand(formatted);
      
      Serial.println("API POST->OEMPARK: " + formatted);
      
      // Esperar respuesta con timeout
      String response = readOEMPARKResponse(3000); // 3 segundos timeout
      
      if (response.length() > 0) {
        String json = parseOEMPARKResponseToJSON(response);
        server.send(200, "application/json", json);
      } else {
        server.send(200, "application/json", "{\"status\":\"sent\",\"timeout\":true,\"command\":\"" + formatted + "\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"command field required\"}");
    }
  });

  // GET /api/ttl/query - Consulta con comando especifico
  server.on("/api/ttl/query", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String address = server.hasArg("addr") ? server.arg("addr") : config.oempark_address;
    String query = server.hasArg("cmd") ? server.arg("cmd") : "S0";
    
    String fullCommand = formatOEMPARKCommand(address, query, "");
    sendOEMPARKCommand(fullCommand);
    Serial.println("API Query->OEMPARK: " + fullCommand);
    
    // Esperar respuesta
    String response = readOEMPARKResponse(3000);
    
    if (response.length() > 0) {
      String json = parseOEMPARKResponseToJSON(response);
      server.send(200, "application/json", json);
    } else {
      server.send(408, "application/json", "{\"error\":\"timeout\",\"command\":\"" + fullCommand + "\"}");
    }
  });

  // POST /api/send - Envio directo (compatibilidad)
  server.on("/api/send", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (server.hasArg("plain")) {
      String data = server.arg("plain");
      sendOEMPARKCommand(data);
      server.send(200, "text/plain", "Sent to OEMPARK: " + data);
      Serial.println("API Send->OEMPARK: " + data);
    } else {
      server.send(400, "text/plain", "No data provided");
    }
  });

  // OPTIONS para CORS
  server.on("/api/ttl", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200);
  });

  // ========== OTA UPDATES ==========
  // Actualizacion OTA
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  Serial.println("Servidor Web/API unificado iniciado en puerto 80");
}

void handleWebAPIServer() {
  server.handleClient();
}

#endif