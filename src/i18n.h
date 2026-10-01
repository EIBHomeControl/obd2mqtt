#pragma once
// Einfache Lokalisierung der Gerätemeldungen: T("Deutsch", "English")
// g_lang: 0 = Deutsch, 1 = English (gesetzt aus cfg.lang)
extern volatile int g_lang;
inline const char* T(const char* de, const char* en) { return g_lang == 1 ? en : de; }
