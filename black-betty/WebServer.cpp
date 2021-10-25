#define DEBUG_MODE 1
#include "WebServer.h"

#include <sys/pgmspace.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "util.h"
#include "Settings.h"
#include "Status.h"
#include "HeaterPID.h"
#include "CommandParser.h"

// The server itself needs to be a global variable for some reasons
ESP8266WebServer server(80);

WebServer::WebServer() {
}

bool WebServer::connect(const char* hostname, const char* ssid, const char* password, int timeout) {
    if (ssid == nullptr || *ssid == 0x00 || password == nullptr || *password == 0x00) {
        Serial.println(F("No ssid/password set, webserver will not be started"));
        return false;
    }

    // Set the timeout from relative to absolute
    int start = millis();
    timeout += start;

    // Change the wifi to client only mode (avoid creating a public access point)
    WiFi.mode(WIFI_STA, false);

    // Begin with the WiFi
    WiFi.hostname(hostname);
    WiFi.begin(ssid, password);
    
    for (wl_status_t status = WL_DISCONNECTED; status != WL_CONNECTED; status = WiFi.status()) {
        if (status == WL_NO_SSID_AVAIL) {
            Serial.println(F("WebServer::connect Unable to connect, ssid not found"));
            return false;
        }

        if (timeout < millis()) {
            Serial.println(F("WebServer::connect Unable to connect to wlan, timed out"));
            return false;
        }
        
        delay(1000);
    }

    // Setup server
    server.on("/", on_serve_index);
    server.on("/status", on_serve_status);
    server.on("/command", on_serve_command);
    server.onNotFound(on_serve_not_found);
    server.begin();

    char ip[20];
    this->get_ip(ip, array_size(ip));
    Serial.printf("Connected with ip %s in %d ms\n", ip, (millis() - start));

    return true;
}

void WebServer::serve() {
    if (WiFi.status() == WL_CONNECTED) {
      server.handleClient();
    }
}

void WebServer::get_ip(char* output, size_t size) const {
    output[0] = output[size - 1] = 0x00;
    if (WiFi.status() == WL_CONNECTED) {
        uint32_t ip = WiFi.localIP().v4();
        snprintf(output, size - 1, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
    } else {
        strncpy(output, reinterpret_cast<const char*>(F("0.0.0.0")), size);
    }
}

void WebServer::on_serve_index() {
    server.send(200, F("text/html; charset=utf-8"), WebServer::webpage_index_content);
}

void WebServer::on_serve_status() {
  const Settings& settings = get_settings();

    // Only allow CORS in debug mode
    if (settings.is_debug()) {
      server.sendHeader(F("Access-Control-Allow-Origin"), F("*")); // DEBUG, DEBUG, DEBUG!
    }

    char buffer[2048];
    buffer[0] = buffer[array_size(buffer) - 1] = 0x00;
 
    // Extracting the json creation to another method will allow the stack to discard json helper structures when calling send
    create_status_json(buffer, array_size(buffer));
    Serial.printf("WebServer::on_serve_status -> Buffer usage: %d / %d\n", strlen(buffer), array_size(buffer));
    server.send(200, F("application/json"), buffer);
}

void WebServer::on_serve_command() {
  const Settings& settings = get_settings();

    // Only allow CORS in debug mode
    if (settings.is_debug()) {
        Serial.println("WebServer::on_serve_command CORS preflight incomming");
        server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
        server.sendHeader(F("Access-Control-Allow-Allow-Method"), F("POST, OPTIONS"));
        server.sendHeader(F("Access-Control-Allow-Headers"), F("X-Requested-With, Content-Type"));
    }
  
    if (server.method() == HTTPMethod::HTTP_OPTIONS) {
        server.send(204);
        return;
    }

    // Create command buffer and copy the body into it. Note that the command buffer needs to be longer than the output buffer because it will be reused for json output
    char command[256];
    command[0] = command[array_size(command) - 1] = 0x00;
    strncpy(command, server.arg("plain").c_str(), array_size(command) - 1);

    char output[128];
    output[0] = output[array_size(output) - 1] = 0x00;
    
    Serial.println(F("Executing web command:"));
    Serial.println(command);
    bool success = get_command_parser().execute(command, true, output, array_size(output));
    
    // Build json from result (reuse command buffer)
    char* pos = command;
    pos = json_add(pos, F("{"));
    pos = json_add_property(pos, F("success"), success, true);
    pos = json_add_property(pos, F("message"), output, false);
    pos = json_add(pos, F("}"));
    *pos = 0x00;

    Serial.println(command);
    
    server.send(success ? 200 : 400, "application/json", command);
    Serial.println("send done");
}

void WebServer::on_serve_not_found() {
    server.send(404, "text/plain", "Not found");
}

void WebServer::create_status_json(char* output, size_t size) {
    const Settings& settings = get_settings();
    const Status& status = get_status();
    const HeaterPID& heater = get_heater();
    int token = get_command_parser().get_security_token();

    char* pos = output;
    pos = json_add(pos, F("{"));
    pos = json_add_property(pos, F("id"), settings.device_id, true);
    pos = json_add_property(pos, F("token"), token, true);
    pos = json_add_property(pos, F("isDebug"), settings.is_debug(), true);
    pos = json_add_property(pos, F("isCountdownMode"), settings.is_countdown_mode(), false);
    pos = json_add(pos, F(",\"temperature\":{"));
    pos = json_add_property(pos, F("current"), status.temperature, true);
    pos = json_add_property(pos, F("target"), heater.get_setpoint(), true);
    pos = json_add_property(pos, F("low"), settings.heater_temperature_low, true);
    pos = json_add_property(pos, F("high"), settings.heater_temperature_high, false);
    pos = json_add(pos, F("},\"pid\":{"));
    pos = json_add_property(pos, F("kp"), heater.get_kp(), true);
    pos = json_add_property(pos, F("ki"), heater.get_ki(), true);
    pos = json_add_property(pos, F("kd"), heater.get_kd(), true);
    pos = json_add_property(pos, F("input"), heater.get_input(), true);
    pos = json_add_property(pos, F("output"), heater.get_output(), true);
    pos = json_add_property(pos, F("setpoint"), heater.get_setpoint(), false);
    pos = json_add(pos, F("},\"heater\":{"));
    pos = json_add_property(pos, F("mode"), status.get_heater_mode(), true);
    pos = json_add_property(pos, F("active"), heater.is_active(), false);
   
    pos = json_add(pos, F("},\"history\": {"));
    pos = json_add_property(pos, F("window"), HISTORY_SLOT_TIME, true);
    
    pos = json_add(pos, F("\"sequence\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
      pos = json_add_array_item(pos, status.get_history(index).sequence, index == 0);
    }

    pos = json_add(pos, F("],\"samples\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
      pos = json_add_array_item(pos, status.get_history(index).samples, index == 0);
    }

    pos = json_add(pos, F("],\"temperature\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
        const StatusHistoryItem& item = status.get_history(index);
        pos = json_add_counter(pos, item.temperature, index == 0, static_cast<double>(item.samples));
    }

    pos = json_add(pos, F("],\"output\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
        const StatusHistoryItem& item = status.get_history(index);
        pos = json_add_counter(pos, item.output, index == 0, static_cast<double>(item.samples));
    }

    pos = json_add(pos, F("],\"heater\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
        const StatusHistoryItem& item = status.get_history(index);
        pos = json_add_counter(pos, item.heater, index == 0, static_cast<double>(item.samples));
    }

    pos = json_add(pos, F("],\"health\":["));
    for (int index = 0; index < HISTORY_SIZE; index++) {
        const StatusHistoryItem& item = status.get_history(index);
        pos = json_add_counter(pos, item.health, index == 0, static_cast<double>(item.samples));
    }

    pos = json_add(pos, F("]}}"));
    *pos = 0x00;
}



WebServer& get_webserver() {
    static WebServer instance;
    return instance;
}
