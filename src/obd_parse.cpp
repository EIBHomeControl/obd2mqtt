#include "obd_parse.h"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "tinyexpr.h"
#include "i18n.h"

namespace ObdParse {

ErrKind lastErrKind = ERR_NONE;

static std::string upper(std::string s) {
  for (auto& c : s) c = (char)toupper((unsigned char)c);
  return s;
}

static std::string hexOnly(const std::string& s) {
  std::string o;
  for (char c : s) if (isxdigit((unsigned char)c)) o += (char)toupper((unsigned char)c);
  return o;
}

std::string expectedPrefix(const std::string& cmdIn) {
  std::string cmd = hexOnly(cmdIn);
  if (cmd.size() < 2) return "";
  unsigned svc = strtoul(cmd.substr(0, 2).c_str(), nullptr, 16) + 0x40;
  char b[3];
  snprintf(b, sizeof(b), "%02X", svc & 0xFF);
  return std::string(b) + cmd.substr(2);
}

bool parseResponse(const std::string& rawIn, const std::string& cmd,
                   std::vector<uint8_t>& bytes, std::string& err) {
  bytes.clear();
  err.clear();
  lastErrKind = ERR_NONE;
  ErrKind kind = ERR_OTHER;
  std::string raw = upper(rawIn);

  // Zeilen aufteilen
  std::vector<std::string> lines;
  std::string cur;
  for (char c : raw) {
    if (c == '\r' || c == '\n') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
    else cur += c;
  }
  if (!cur.empty()) lines.push_back(cur);

  std::vector<std::string> messages;   // Einzel-Frames bzw. zusammengesetzte Multi-Frames
  std::string multi;
  size_t multiLen = 0;                 // angekündigte Länge (z.B. "03E" = 62 Bytes)
  for (auto& l0 : lines) {
    std::string l = l0;
    // Leerzeichen am Rand entfernen
    while (!l.empty() && l.front() == ' ') l.erase(l.begin());
    while (!l.empty() && l.back() == ' ') l.pop_back();
    if (l.empty() || l.find("SEARCHING") != std::string::npos || l == "OK") continue;
    if (l.find("NO DATA") != std::string::npos) { err = T("NO DATA (ECU antwortet nicht – Auto schläft?)", "NO DATA (ECU not responding – car asleep?)"); kind = ERR_NO_DATA; continue; }
    if (l == "?") { err = T("Befehl vom Dongle nicht verstanden (?)", "Command not understood by dongle (?)"); continue; }
    if (l.find("ERROR") != std::string::npos || l.find("UNABLE") != std::string::npos ||
        l.find("STOPPED") != std::string::npos || l.find("BUFFER FULL") != std::string::npos) {
      err = l;
      if (l.find("CAN ERROR") != std::string::npos || l.find("BUS") != std::string::npos) kind = ERR_CAN;
      continue;
    }
    size_t colon = l.find(':');
    if (colon != std::string::npos && colon <= 2) {          // "0:", "1:", ... "F:"
      multi += hexOnly(l.substr(colon + 1));
      continue;
    }
    std::string h = hexOnly(l);
    if (h.size() == 3) {           // Längenangabe vor Multi-Frame, z.B. "03E"
      multiLen = strtoul(h.c_str(), nullptr, 16);
      continue;
    }
    if (h.size() >= 2) messages.push_back(h);
  }
  if (!multi.empty()) messages.insert(messages.begin(), multi);

  std::string prefix = expectedPrefix(cmd);
  for (auto& m : messages) {
    std::string msg = m;
    // "Response pending" (7F xx 78) vorn abschneiden
    while (msg.size() > 6 && msg.compare(0, 2, "7F") == 0 && msg.compare(4, 2, "78") == 0) msg = msg.substr(6);
    if (msg.compare(0, prefix.size(), prefix) != 0) continue;
    for (size_t i = 0; i + 1 < msg.size(); i += 2)
      bytes.push_back((uint8_t)strtoul(msg.substr(i, 2).c_str(), nullptr, 16));
    if (m == multi && multiLen) {
      if (bytes.size() < multiLen) {   // Folge-Frames fehlen (Timeout im Dongle / BLE)
        err = std::string(T("Unvollständige Antwort (", "Incomplete response (")) + std::to_string(bytes.size()) +
              T(" von ", " of ") + std::to_string(multiLen) + " Bytes)";
        lastErrKind = ERR_INCOMPLETE;
        return false;
      }
      bytes.resize(multiLen);          // Füllbytes (AA) am Ende abschneiden
    }
    err.clear();
    return true;
  }
  for (auto& m : messages) {
    if (m.size() >= 6 && m.compare(0, 2, "7F") == 0) {
      err = std::string(T("Negative Antwort, NRC 0x", "Negative response, NRC 0x")) + m.substr(4, 2);
      lastErrKind = ERR_NRC;
      return false;
    }
  }
  if (err.empty()) err = messages.empty() ? std::string(T("Leere Antwort", "Empty response"))
                                         : std::string(T("Unerwartete Antwort: ", "Unexpected response: ")) + messages[0].substr(0, 40);
  lastErrKind = kind;
  return false;
}

// ---------- Formeln ----------

static double fnU16(double hi, double lo) { return (double)(((unsigned)hi & 0xFF) << 8 | ((unsigned)lo & 0xFF)); }
static double fnS16(double hi, double lo) { return (double)(int16_t)(((unsigned)hi & 0xFF) << 8 | ((unsigned)lo & 0xFF)); }
static double fnS8(double x) { return (double)(int8_t)((unsigned)x & 0xFF); }
static double fnBit(double x, double n) { return (double)((((unsigned long)x) >> (unsigned)n) & 1UL); }
static double fnLt(double a, double b) { return a < b ? 1 : 0; }
static double fnGt(double a, double b) { return a > b ? 1 : 0; }
static double fnMin(double a, double b) { return a < b ? a : b; }
static double fnMax(double a, double b) { return a > b ? a : b; }

static double gCap = 0, gCons = 0;
void setBattery(double capKwh, double consKwh100) { gCap = capKwh; gCons = consKwh100; }
void getBattery(double& capKwh, double& consKwh100) { capKwh = gCap; consKwh100 = gCons; }

bool evalFormula(const std::string& formula, const std::vector<uint8_t>& bytes,
                 double& value, std::string& err) {
  const size_t n = bytes.size() > 250 ? 250 : bytes.size();
  std::vector<std::string> names(n);
  std::vector<double> vals(n);
  std::vector<te_variable> vars;
  vars.reserve(n + 10);
  double cap = gCap, cons = gCons;
  vars.push_back({"CAP", &cap, TE_VARIABLE, nullptr});
  vars.push_back({"CONS", &cons, TE_VARIABLE, nullptr});
  vars.push_back({"u16", (const void*)fnU16, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"s16", (const void*)fnS16, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"s8", (const void*)fnS8, TE_FUNCTION1 | TE_FLAG_PURE, nullptr});
  vars.push_back({"bit", (const void*)fnBit, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"lt", (const void*)fnLt, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"gt", (const void*)fnGt, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"min", (const void*)fnMin, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  vars.push_back({"max", (const void*)fnMax, TE_FUNCTION2 | TE_FLAG_PURE, nullptr});
  for (size_t i = 0; i < n; i++) {
    names[i] = "B" + std::to_string(i);
    vals[i] = bytes[i];
    vars.push_back({names[i].c_str(), &vals[i], TE_VARIABLE, nullptr});
  }
  int pos = 0;
  te_expr* e = te_compile(formula.c_str(), vars.data(), (int)vars.size(), &pos);
  if (!e) {
    err = std::string(T("Formelfehler an Position ", "Formula error at position ")) + std::to_string(pos) +
          T(" (Byte-Index zu groß? Antwort hat ", " (byte index too large? response has ") + std::to_string(n) + " Bytes: B0..B" +
          std::to_string(n ? n - 1 : 0) + ")";
    return false;
  }
  value = te_eval(e);
  te_free(e);
  if (std::isnan(value) || std::isinf(value)) { err = T("Ergebnis ungültig", "Invalid result"); return false; }
  if (value == 0) value = 0;   // -0 → 0
  return true;
}

std::string toHex(const std::vector<uint8_t>& b, bool spaces) {
  std::string o;
  char t[4];
  for (size_t i = 0; i < b.size(); i++) {
    snprintf(t, sizeof(t), "%02X", b[i]);
    if (spaces && i) o += ' ';
    o += t;
  }
  return o;
}

}  // namespace ObdParse
