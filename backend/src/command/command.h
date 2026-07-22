#pragma once
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../app.h"

namespace Crown {

void RequestShutdown();

std::unordered_map<std::string, std::function<void()>> commands = {
    {"exit", []() { RequestShutdown(); }}};
inline void StartConsoleReader(AppConfig *cfg) {
  std::thread([cfg] {
    std::string line;
    while (std::getline(std::cin, line)) {
      line.erase(0, line.find_first_not_of(" \t\n\r"));
      line.erase(line.find_last_not_of(" \t\n\r") + 1);

      if (line.empty())
        return;

      if (!cfg->serverPrefix.empty() && line.rfind(cfg->serverPrefix, 0) == 0) {
        std::string command = line.substr(cfg->serverPrefix.length());
        command.erase(0, command.find_first_not_of(" \t\n\r"));
        std::transform(command.begin(), command.end(), command.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        auto it = commands.find(command);
        if (it != commands.end()) {
          it->second();
        } else {
          std::cerr << "[CMD] Comando desconhecido: " << line << "\n";
        }
      }
    }
  }).detach();
};
} // namespace Crown
