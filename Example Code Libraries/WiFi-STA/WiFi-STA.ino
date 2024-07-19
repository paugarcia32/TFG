#include "config.h"
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

WebServer server(80);
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // Modo Station
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() { server.handleClient(); }

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML("Hola a todos"));
}

void handle_NotFound() { server.send(404, "text/plain", "No hay respuesta"); }

String SendHTML(String S) {
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>Pau Garcia</title>";
  ptr +=
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<link "
         "href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' "
         "rel='stylesheet'>";
  ptr += "<style>";
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: "
         "0px auto; text-align: center;color: #444444;}";
  ptr += "body{margin: 0px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: "
         "relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";

  ptr += ".superscript{font-size: 17px;font-weight: 600;position: "
         "absolute;top: 10px;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:65px}";
  ptr += "</style>";
  ptr += "</head>";
  ptr += "<body>";
  ptr += "<h1>Modo STA</h1>";
  ptr += "<div class='container'>";
  // ptr +="<div class='side-by-side text'>S</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += S; //.................................
  ptr += "</div>";
  ptr += "</div>";
  ptr += "</div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}
