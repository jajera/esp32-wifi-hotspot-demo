#ifndef BLE_TRANSPORT

#include "captive_portal.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "logging.h"
#include "validation.h"

namespace {
DNSServer dnsServer;
WebServer webServer(80);
PortalCredentials pending;
bool running = false;

void redirectToRoot() {
    webServer.sendHeader("Location", String("http://") + CAPTIVE_PORTAL_IP.toString() + "/", true);
    webServer.send(302, "text/plain", "");
}

void handleRoot() {
    webServer.send(200, "text/html", CaptivePortal::portalHtml());
}

void handleScan() {
    int count = WiFi.scanNetworks(false, true);
    String json = "[";
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            json += ",";
        }
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                ",\"enc\":" + String(WiFi.encryptionType(i)) + "}";
    }
    json += "]";
    webServer.send(200, "application/json", json);
}

void handleSave() {
    if (!webServer.hasArg("ssid")) {
        logCredentialResult(false, "missing_ssid", 0);
        webServer.send(400, "text/html", "<h1>Missing SSID</h1><a href='/'>Back</a>");
        return;
    }

    String ssid = webServer.arg("ssid");
    String password = webServer.hasArg("password") ? webServer.arg("password") : "";

    auto validation = validateCredentials(ssid, password);
    if (!validation.valid) {
        logCredentialResult(false, validation.reason, ssid.length());
        webServer.send(400, "text/html", String("<h1>Invalid credentials: ") + validation.reason +
                                              "</h1><a href='/'>Back</a>");
        return;
    }

    pending.received = true;
    pending.ssid = ssid;
    pending.password = password;
    logCredentialResult(true, validation.reason, ssid.length());
    webServer.send(200, "text/html",
                   "<h1>Saved</h1><p>Connecting to WiFi. You may close this page.</p>");
}
}  // namespace

namespace CaptivePortal {

const char* portalHtml() {
    return R"HTML(<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>ESP Setup</title></head><body><h1>WiFi Setup</h1><p>2.4 GHz networks only.</p><form method='POST' action='/save'><label>SSID<br><input name='ssid' required></label><br><br><label>Password<br><input name='password' type='password'></label><br><br><button type='submit'>Connect</button></form><p><a href='/scan'>Scan networks</a></p></body></html>)HTML";
}

void start(const String& apSsid) {
    WiFi.mode(WIFI_AP);
#ifdef SETUP_AP_PASSWORD
    WiFi.softAP(apSsid.c_str(), SETUP_AP_PASSWORD);
#else
    WiFi.softAP(apSsid.c_str());
#endif
    WiFi.softAPConfig(CAPTIVE_PORTAL_IP, CAPTIVE_PORTAL_IP, IPAddress(255, 255, 255, 0));

    dnsServer.start(53, "*", CAPTIVE_PORTAL_IP);

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/scan", HTTP_GET, handleScan);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.on("/generate_204", HTTP_GET, redirectToRoot);
    webServer.on("/hotspot-detect.html", HTTP_GET, redirectToRoot);
    webServer.on("/connecttest.txt", HTTP_GET, redirectToRoot);
    webServer.onNotFound(redirectToRoot);
    webServer.begin();

    pending = PortalCredentials{};
    running = true;
}

void stop() {
    if (!running) {
        return;
    }
    webServer.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    running = false;
}

void loop() {
    if (!running) {
        return;
    }
    dnsServer.processNextRequest();
    webServer.handleClient();
}

bool hasNewCredentials() { return pending.received; }

PortalCredentials getCredentials() {
    PortalCredentials creds = pending;
    pending.received = false;
    return creds;
}

}  // namespace CaptivePortal

#endif
