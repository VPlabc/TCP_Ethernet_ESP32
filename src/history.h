#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <Wire.h>
#include <RTClib.h>

#define LOG_FILE "/log.bin"
#define LOG_RECORD_SIZE 16
// Maximum number of records to keep (adjust as needed)
#define LOG_MAX_RECORDS 1024

RTC_DS3231 rtc;

// Action codes
enum Action : uint8_t {
  LOGIN = 1,
  LOGOUT = 2,
  CLOSE = 3,
  OPEN = 4,
  CONFIG = 5,
  RESET = 6,
  RESET_FROM = 7
};

struct LogRecord {
  uint32_t epoch;      // 4 bytes
  char user_id[6];     // 6 bytes
  uint8_t action;      // 1 byte
  char extra[5];       // 5 bytes (for future use or reset_from string)
};

bool initLog() {
  if (!LittleFS.begin(true)) return false;
  if (!rtc.begin()) return false;
  return true;
}

uint32_t getEpoch() {
  DateTime now = rtc.now();
  return now.unixtime();
}

// Helper: Get total records in log
size_t logRecordCount() {
    File f = LittleFS.open(LOG_FILE, FILE_READ);
    if (!f) return 0;
    size_t count = f.size() / LOG_RECORD_SIZE;
    f.close();
    return count;
}

// Helper: Read the oldest record (for backup/check)
bool readOldestLog(LogRecord &rec) {
    File f = LittleFS.open(LOG_FILE, FILE_READ);
    if (!f) return false;
    if (f.read((uint8_t*)&rec, sizeof(rec)) != sizeof(rec)) {
        f.close();
        return false;
    }
    f.close();
    return true;
}

// Rotate log if full: remove oldest record
void rotateLogIfNeeded() {
    size_t count = logRecordCount();
    if (count < LOG_MAX_RECORDS) return;

    // Open original log for reading, temp for writing
    File fin = LittleFS.open(LOG_FILE, FILE_READ);
    File fout = LittleFS.open("/log_tmp.bin", FILE_WRITE);
    if (!fin || !fout) {
        if (fin) fin.close();
        if (fout) fout.close();
        return;
    }
    // Skip the oldest record
    fin.seek(LOG_RECORD_SIZE, SeekSet);
    uint8_t buf[64];
    size_t n;
    while ((n = fin.read(buf, sizeof(buf))) > 0) {
        fout.write(buf, n);
    }
    fin.close();
    fout.close();
    LittleFS.remove(LOG_FILE);
    LittleFS.rename("/log_tmp.bin", LOG_FILE);
}

bool writeLog(const char* user_id, Action action, const char* extra = "") {
    rotateLogIfNeeded();
    LogRecord rec;
    rec.epoch = getEpoch();
    strncpy(rec.user_id, user_id, 6);
    rec.action = (uint8_t)action;
    memset(rec.extra, 0, sizeof(rec.extra));
    if (extra) strncpy(rec.extra, extra, sizeof(rec.extra)-1);

    File f = LittleFS.open(LOG_FILE, FILE_APPEND);
    if (!f) return false;
    size_t written = f.write((uint8_t*)&rec, sizeof(rec));
    f.close();
    return written == sizeof(rec);
}

// Helper: Convert action code to string
const char* actionToStr(uint8_t action) {
  switch (action) {
    case LOGIN: return "login";
    case LOGOUT: return "logout";
    case CLOSE: return "close";
    case OPEN: return "open";
    case CONFIG: return "config";
    case RESET: return "reset";
    case RESET_FROM: return "reset_from";
    default: return "unknown";
  }
}

// Helper: Time ago string
String timeAgo(uint32_t epoch) {
  uint32_t now = getEpoch();
  uint32_t diff = now > epoch ? now - epoch : 0;
  if (diff < 60) return String(diff) + "s ago";
  if (diff < 3600) return String(diff/60) + "m ago";
  if (diff < 86400) return String(diff/3600) + "h ago";
  return String(diff/86400) + "d ago";
}

// Web: /history?user=xxxxxx&from=0
void handleHistory(AsyncWebServerRequest *request) {
  String user = request->getParam("user", false) ? request->getParam("user")->value() : "";
  int from = request->getParam("from", false) ? request->getParam("from")->value().toInt() : 0;
  File f = LittleFS.open(LOG_FILE, FILE_READ);
  if (!f) {
    request->send(500, "text/plain", "Log file error");
    return;
  }
  size_t total = f.size() / LOG_RECORD_SIZE;
  int count = 0, sent = 0;
  String html = "<table border=1><tr><th>Time</th><th>User</th><th>Action</th><th>Time Ago</th></tr>";
  if (from < 0) from = 0;
  if (from > (int)total-1) from = total-1;
  int start = total - 1 - from;
  for (int i = start; i >= 0 && sent < 50; --i) {
    f.seek(i * LOG_RECORD_SIZE, SeekSet);
    LogRecord rec;
    if (f.read((uint8_t*)&rec, sizeof(rec)) != sizeof(rec)) break;
    if (user.length() > 0 && String(rec.user_id) != user) continue;
    html += "<tr>";
    html += "<td>" + String(rec.epoch) + "</td>";
    html += "<td>" + String(rec.user_id) + "</td>";
    html += "<td>" + String(actionToStr(rec.action));
    if (rec.action == RESET_FROM) html += " (" + String(rec.extra) + ")";
    html += "</td>";
    html += "<td>" + timeAgo(rec.epoch) + "</td>";
    html += "</tr>";
    sent++;
  }
  html += "</table>";
  html += "<form method='get'><input name='user' placeholder='User'><input name='from' type='number' min='0' value='0'><button>Filter</button></form>";
  f.close();
  request->send(200, "text/html", html);
}

// Add to webserver setup
// server.on("/history", HTTP_GET, handleHistory);

// Example usage in your code:
// writeLog("USR001", LOGIN);
// writeLog("USR002", LOGOUT);
// writeLog("USR003", RESET_FROM, "web");