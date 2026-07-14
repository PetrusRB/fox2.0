#pragma once

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define SET_ENV(key, val) _putenv_s(key, val)
#else
#define SET_ENV(key, val) setenv(key, val, 1)
#endif

namespace Crown {

inline std::string GetEnv(const std::string &key,
                          const std::string &fallback = "") {
  const char *val = std::getenv(key.c_str());
  return val ? std::string(val) : fallback;
}

inline void LoadEnv(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cout << "[ENV] Arquivo " << path << " nao encontrado, usando env do "
                 "sistema.\n";
    return;
  }

  std::string line;
  int loaded = 0;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;

    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);

    // Remove aspas
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
      value = value.substr(1, value.size() - 2);
    }

    // Só seta se a variável não existe no sistema
    if (std::getenv(key.c_str()) == nullptr) {
      SET_ENV(key.c_str(), value.c_str());
      loaded++;
    }
  }

  std::cout << "[ENV] " << loaded << " variaveis carregadas de " << path
            << "\n";
}

} // namespace Crown
