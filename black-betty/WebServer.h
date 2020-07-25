#pragma once

#include <Arduino.h>

class WebServer {
public:
    WebServer();
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    bool connect(const char* hostname, const char* ssid, const char* password, int timeout);

    void serve();

    void get_ip(char* output, size_t size) const;

private:
    static void on_serve_index();
    static void on_serve_status();
    static void on_serve_command();
    static void on_serve_not_found();

    static void create_status_json(char* output, size_t size);
    
    // This will be injected by the index/html/js/css script into the "WebServer_index.cpp" file
    static const __FlashStringHelper* webpage_index_content;
};

// Use this function to get the webserver instance it MUST be a singleton since the serving methods rely on that
WebServer& get_webserver();
