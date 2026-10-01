#pragma once
// Plattformunabhängig (auch auf dem PC testbar): ELM327-Antwort parsen + Formel auswerten
#include <string>
#include <vector>
#include <cstdint>

namespace ObdParse {

enum ErrKind { ERR_NONE, ERR_NO_DATA, ERR_INCOMPLETE, ERR_CAN, ERR_NRC, ERR_OTHER };
extern ErrKind lastErrKind;          // Art des letzten Fehlers von parseResponse()

// Erwartetes Antwort-Präfix: 1. Byte + 0x40, Rest gleich ("220105" → "620105", "015B" → "415B")
std::string expectedPrefix(const std::string& cmd);

// Wandelt die ELM-Rohantwort (ATH0, ATCAF1, ATS0 oder ATS1) in Bytes um.
// Unterstützt Single-Frame, Multi-Frame ("03E / 0: .. / 1: ..") und mehrere antwortende ECUs.
// bytes beginnt mit dem Service-Byte (z.B. 0x62).
bool parseResponse(const std::string& raw, const std::string& cmd,
                   std::vector<uint8_t>& bytes, std::string& err);

// Formel mit Variablen B0..Bn und Funktionen u16(hi,lo), s16(hi,lo), s8(x), bit(x,n)
bool evalFormula(const std::string& formula, const std::vector<uint8_t>& bytes,
                 double& value, std::string& err);

std::string toHex(const std::vector<uint8_t>& bytes, bool spaces = true);

}  // namespace ObdParse
