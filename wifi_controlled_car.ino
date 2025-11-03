/* ==========================
   ESP8266 Wi-Fi Robot Car
   Components:
   - ESP8266 (NodeMCU/Wemos D1 Mini)
   - L298N Motor Driver
   - 4 DC Motors (2 left, 2 right)
   - Web-based control interface
   ========================== 
   Somethibg chabged in the control
   right -> forward
   left -> backward
   */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------- CONFIG ----------
#define USE_AP_MODE true   // true = ESP creates its own hotspot
const char* STA_SSID = "HOGWART";
const char* STA_PASS = "dracarys!!";

const char* AP_SSID = "Rigel";
const char* AP_PASS = "Rigel1234"; // must be 8+ characters

// ---------- MOTOR PINS ----------
const uint8_t ENA = 5;   // D1 - Left motor speed
const uint8_t IN1 = 14;  // D5
const uint8_t IN2 = 12;  // D6
const uint8_t ENB = 4;   // D2 - Right motor speed
const uint8_t IN3 = 13;  // D7
const uint8_t IN4 = 16;  // D0

// ---------- PWM SETTINGS ----------
const int PWM_FREQ = 1000;
const int PWM_RANGE = 1023;
int currentSpeed = 700;

ESP8266WebServer server(80);

// ---------- MOTOR FUNCTIONS ----------
void setSpeed(int s) {
  if (s < 0) s = 0;
  if (s > PWM_RANGE) s = PWM_RANGE;
  currentSpeed = s;
  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed);
}

void motorsStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void motorsBrake() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, PWM_RANGE);
  analogWrite(ENB, PWM_RANGE);
}

void forwardCmd() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setSpeed(currentSpeed);
}

void backwardCmd() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setSpeed(currentSpeed);
}

void leftCmd() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setSpeed(currentSpeed);
}

void rightCmd() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setSpeed(currentSpeed);
}

// ---------- WEB PAGE ----------
void handleRoot() {
  String page = R"rawliteral(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP Robot Control</title>
<style>
body{font-family:Arial;margin:0;padding:10px;background:#f0f0f0}
.container{max-width:480px;margin:auto;background:#fff;padding:12px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.2)}
h2{text-align:center;color:#333}
button{min-width:100px;padding:12px;font-size:18px;border:none;border-radius:8px;margin:6px;cursor:pointer}
.forward{background:#4CAF50;color:white}
.back{background:#f44336;color:white}
.left{background:#2196F3;color:white}
.right{background:#FF9800;color:white}
.stop{background:#555;color:white}
input[type=range]{width:100%}
.label{text-align:center;font-weight:bold}
footer{text-align:center;font-size:12px;color:#777;margin-top:12px}
</style>
</head>
<body>
<div class="container">
<h2>Your Buddy | RIGEL</h2>
<div style="text-align:center">
<button class="forward" onclick="send('right')">Forward</button><br>
<button class="left" onclick="send('forward')">Left</button>
<button class="right" onclick="send('back')">Right</button>
<button class="back" onclick="send('left')">Backward</button><br>
<button class="stop" onclick="send('stop')">Stop</button>
</div>
<div class="label">Speed: <span id="spdVal">700</span></div>
<input type="range" min="0" max="1023" value="700" id="speed" oninput="updateSpeed(this.value)">
<footer>Connected to <span id="ip">...</span></footer>
</div>
<script>
function send(cmd){fetch('/move?dir='+cmd).catch(()=>alert('Command failed'));}
function updateSpeed(v){document.getElementById('spdVal').innerText=v;fetch('/speed?val='+v);}
fetch('/whoami').then(r=>r.text()).then(t=>document.getElementById('ip').innerText=t);
document.addEventListener('keydown',e=>{
  const k=e.key.toLowerCase();
  if(k=='w')send('forward');
  if(k=='s')send('back');
  if(k=='a')send('left');
  if(k=='d')send('right');
  if(k==' ')send('stop');
});
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", page);
}

void handleWhoami() {
  String ip = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  server.send(200, "text/plain", ip);
}

void handleMove() {
  if (!server.hasArg("dir")) { server.send(400, "text/plain", "missing dir"); return; }
  String dir = server.arg("dir");
  if (dir == "forward") forwardCmd();
  else if (dir == "back") backwardCmd();
  else if (dir == "left") leftCmd();
  else if (dir == "right") rightCmd();
  else if (dir == "stop") motorsStop();
  else if (dir == "brake") motorsBrake();
  else { server.send(400, "text/plain", "bad dir"); return; }
  server.send(200, "text/plain", "ok");
}

void handleSpeed() {
  if (!server.hasArg("val")) { server.send(400, "text/plain", "missing val"); return; }
  int v = server.arg("val").toInt();
  setSpeed(v);
  server.send(200, "text/plain", String(currentSpeed));
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== ESP8266 ROBOT START ===");

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  analogWriteFreq(PWM_FREQ);
  analogWriteRange(PWM_RANGE);
  motorsStop();

  if (USE_AP_MODE) {
    Serial.println("Starting Access Point mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP SSID: "); Serial.println(AP_SSID);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  } else {
    Serial.print("Connecting to "); Serial.println(STA_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected! IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Failed, switching to AP mode...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASS);
      Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    }
  }

  server.on("/", handleRoot);
  server.on("/whoami", handleWhoami);
  server.on("/move", handleMove);
  server.on("/speed", handleSpeed);
  server.begin();

  Serial.println("HTTP server started");
  Serial.println("=========================\n");
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();
}
