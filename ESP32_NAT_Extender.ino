#include "esp_netif.h"
#include "esp_wifi.h"
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

// Check ESP-IDF version for strict NAT compatibility
#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "lwip/lwip_napt.h"
#else
#include "lwip/lwip_napt.h"
#endif

WebServer server(80);
Preferences prefs;

String sta_ssid, sta_pass, ap_ssid, ap_pass;
bool nat_enabled = false;

// ---------------------------------------------------------
// BEAUTIFUL PRO WEBPAGE (Stored in Flash)
// ---------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Pro Extender</title>
  <style>
    :root { --bg: #0f172a; --card: #1e293b; --text: #f8fafc; --primary: #3b82f6; --success: #10b981; --danger: #ef4444; }
    body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; min-height: 100vh; }
    .container { max-width: 500px; width: 100%; }
    .card { background: var(--card); border-radius: 16px; padding: 24px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #334155; margin-bottom: 20px; }
    h2 { margin: 0 0 20px 0; font-size: 24px; font-weight: 600; text-align: center; background: linear-gradient(to right, #3b82f6, #8b5cf6); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
    .stat-box { background: #0f172a; padding: 15px; border-radius: 12px; border: 1px solid #334155; text-align: center; }
    .stat-value { font-size: 20px; font-weight: bold; color: var(--primary); margin-top: 5px; }
    .stat-label { font-size: 12px; color: #94a3b8; text-transform: uppercase; letter-spacing: 1px; }
    .badge { display: inline-block; padding: 4px 12px; border-radius: 20px; font-size: 13px; font-weight: bold; }
    .bg-success { background: rgba(16, 185, 129, 0.2); color: var(--success); }
    .bg-danger { background: rgba(239, 68, 68, 0.2); color: var(--danger); }
    .form-group { margin-bottom: 15px; }
    label { display: block; font-size: 14px; color: #cbd5e1; margin-bottom: 8px; font-weight: 500; }
    input { width: 100%; padding: 12px; background: #0f172a; border: 1px solid #334155; color: #fff; border-radius: 8px; box-sizing: border-box; font-size: 15px; transition: 0.3s; }
    input:focus { outline: none; border-color: var(--primary); }
    button { width: 100%; padding: 14px; background: linear-gradient(to right, #3b82f6, #2563eb); color: white; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; transition: 0.2s; margin-top: 10px; }
    button:hover { opacity: 0.9; transform: translateY(-2px); }
    .divider { height: 1px; background: #334155; margin: 24px 0; }
  </style>
</head>
<body>
  <div class="container">
    <div class="card">
      <h2>ESP32 Pro Extender</h2>
      
      <div style="text-align: center; margin-bottom: 20px;">
        <span id="conn_status" class="badge bg-danger">Disconnected</span>
      </div>

      <div class="grid">
        <div class="stat-box">
          <div class="stat-label">WAN IP</div>
          <div class="stat-value" id="wan_ip">--</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Clients</div>
          <div class="stat-value" id="clients">0</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Signal</div>
          <div class="stat-value" id="rssi">-- dBm</div>
        </div>
        <div class="stat-box">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="uptime">0s</div>
        </div>
      </div>
    </div>

    <div class="card">
      <form id="configForm">
        <div class="stat-label" style="margin-bottom:15px; color:var(--primary);">1. Upstream Router (Internet Source)</div>
        <div class="form-group">
          <label>Router WiFi Name</label>
          <input type="text" id="sta_ssid" required placeholder="My Home WiFi">
        </div>
        <div class="form-group">
          <label>Router Password</label>
          <input type="password" id="sta_pass" placeholder="••••••••">
        </div>
        
        <div class="divider"></div>
        
        <div class="stat-label" style="margin-bottom:15px; color:var(--primary);">2. Extender Network (Local Settings)</div>
        <div class="form-group">
          <label>Extender WiFi Name</label>
          <input type="text" id="ap_ssid" required placeholder="ESP32_EXTENDER">
        </div>
        <div class="form-group">
          <label>Extender Password</label>
          <input type="password" id="ap_pass" minlength="8" required placeholder="Min 8 characters">
        </div>
        <button type="submit" id="saveBtn">Save & Apply Changes</button>
      </form>
    </div>
  </div>
  <script>
    function updateData() {
      fetch('/api/status').then(r=>r.json()).then(d=>{
        document.getElementById('wan_ip').innerText = d.wan_ip;
        document.getElementById('clients').innerText = d.clients;
        document.getElementById('rssi').innerText = d.rssi + ' dBm';
        
        let m = Math.floor(d.uptime / 60);
        let s = d.uptime % 60;
        document.getElementById('uptime').innerText = m + 'm ' + s + 's';
        
        let stat = document.getElementById('conn_status');
        if(d.wan_ip !== '0.0.0.0') {
          stat.className = 'badge bg-success';
          stat.innerText = 'Internet Connected';
        } else {
          stat.className = 'badge bg-danger';
          stat.innerText = 'Disconnected';
        }
      }).catch(e=>{});
    }
    setInterval(updateData, 2000); updateData();

    document.getElementById('configForm').addEventListener('submit', function(e){
      e.preventDefault();
      let btn = document.getElementById('saveBtn');
      btn.innerText = 'Saving & Rebooting...';
      btn.style.opacity = '0.5';
      
      const params = new URLSearchParams();
      params.append('sta_ssid', document.getElementById('sta_ssid').value);
      params.append('sta_pass', document.getElementById('sta_pass').value);
      params.append('ap_ssid', document.getElementById('ap_ssid').value);
      params.append('ap_pass', document.getElementById('ap_pass').value);
      
      fetch('/api/config', {method:'POST', body:params}).then(()=>{
        setTimeout(()=> location.reload(), 10000);
      });
    });
  </script>
</body>
</html>
)rawliteral";

// ---------------------------------------------------------
// CORE SYSTEM SETUP
// ---------------------------------------------------------

void setup() {
  Serial.begin(115200);

  // FORCE MAXIMUM CPU SPEED FOR FASTER ROUTING
  setCpuFrequencyMhz(240);

  Serial.println("\n--- ESP32 Pro NAT Extender ---");

  // Load saved credentials
  prefs.begin("network", false);
  sta_ssid = prefs.getString("sta_ssid", "");
  sta_pass = prefs.getString("sta_pass", "");
  ap_ssid = prefs.getString("ap_ssid", "ESP32_EXTENDER");
  ap_pass = prefs.getString("ap_pass", "12345678");

  WiFi.mode(WIFI_AP_STA);

  // CRITICAL FIX 1: Turn off Wi-Fi Power Saving.
  // If this is on, the radio sleeps and drops internet packets.
  esp_wifi_set_ps(WIFI_PS_NONE);

  // CRITICAL FIX 2: Force Subnet to 192.168.14.1 to avoid router conflicts
  IPAddress apIP(192, 168, 14, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // Start Extender Network
  WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
  Serial.print("Extender Network: ");
  Serial.println(ap_ssid);

  // CRITICAL FIX 3: Push 8.8.8.8 DNS via DHCP using version-safe syntax
  esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap_netif) {
    esp_netif_dhcps_stop(ap_netif);

    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(IPAddress(8, 8, 8, 8));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    dns.ip.type = ESP_IPADDR_TYPE_V4;
    uint8_t opt_val = 1;
#else
    dns.ip.type = IPADDR_TYPE_V4;
    dhcps_offer_t opt_val = OFFER_DNS;
#endif

    esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET,
                           ESP_NETIF_DOMAIN_NAME_SERVER, &opt_val,
                           sizeof(opt_val));
    esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
    esp_netif_dhcps_start(ap_netif);
  }

  // Connect to Internet
  if (sta_ssid != "") {
    Serial.print("Connecting to Internet Source: ");
    Serial.println(sta_ssid);
    WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());
  }

  // Web Server Routing
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", INDEX_HTML); });

  server.on("/api/status", HTTP_GET, []() {
    String wan_ip =
        (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "0.0.0.0";
    int clients = WiFi.softAPgetStationNum();
    int rssi = WiFi.RSSI();
    unsigned long uptime = millis() / 1000;

    String json = "{";
    json += "\"wan_ip\":\"" + wan_ip + "\",";
    json += "\"clients\":" + String(clients) + ",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"uptime\":" + String(uptime);
    json += "}";

    server.send(200, "application/json", json);
  });

  server.on("/api/config", HTTP_POST, []() {
    prefs.putString("sta_ssid", server.arg("sta_ssid"));
    prefs.putString("sta_pass", server.arg("sta_pass"));
    prefs.putString("ap_ssid", server.arg("ap_ssid"));
    prefs.putString("ap_pass", server.arg("ap_pass"));
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.println("Web dashboard active at http://192.168.14.1");
}

void loop() {
  server.handleClient();

  // Enable NAT once internet is connected
  if (WiFi.status() == WL_CONNECTED && !nat_enabled) {
    Serial.print("\nInternet Connected! IP: ");
    Serial.println(WiFi.localIP());

// Trigger NAT activation
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) {
      esp_err_t err = esp_netif_napt_enable(netif);
      if (err == ESP_OK)
        Serial.println("NAT Enabled. Internet is now routing!");
      else
        Serial.printf(
            "NAT Failed (Error: %d). Core does not support IP_FORWARD.\n", err);
    }
#else
    ip_napt_enable(WiFi.softAPIP(), 1);
    Serial.println("NAT Enabled. Internet is now routing!");
#endif

    nat_enabled = true;
  }

  // Handle Disconnection
  if (WiFi.status() != WL_CONNECTED && nat_enabled) {
    Serial.println("Lost connection to internet.");
    nat_enabled = false;
  }
}