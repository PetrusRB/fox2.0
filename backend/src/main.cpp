#include "./app.h"
#include "./command/command.h"
#include "./server/http_server.h"
#include "./server/server.h"
#include "./utils/dotenv.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

#define CROWN_SERVEROS 50051
#define SIGNATUROS 8081

// ------------------------------------------------------
// -- helpers para o shutdown do servidor
// ------------------------------------------------------

namespace Crown {
inline std::atomic<bool> g_shuttingDown{false};
inline std::atomic<bool> g_shutdownComplete{false};
inline std::mutex g_shutdownMutex;
inline std::condition_variable g_shutdownCv;

inline void RequestShutdown() {
  g_shuttingDown = true;
  g_shutdownCv.notify_all();
}

inline void WaitForShutdownSignal() {
  std::unique_lock<std::mutex> lock(g_shutdownMutex);
  g_shutdownCv.wait(lock, [] { return g_shuttingDown.load(); });
}
} // namespace Crown

int main() {
  Crown::LoadEnv("backend/.env");

  try {
    Crown::AppContext app(Crown::AppConfig::FromEnv());

    Crown::AppConfig cfg;
    Crown::StartConsoleReader(&cfg);

    Crown::CrownServer server;
    Crown::CdnServer cdn;

    std::thread watcher([&server, &cdn] {
      Crown::WaitForShutdownSignal();
      std::cerr << "[MAIN] Encerrando os servidores...\n";
      cdn.shutdown();
      server.shutdown();
    });

    if (!cdn.start(SIGNATUROS, app)) {
      std::cerr << "[MAIN] WARN: CDN server falhou ao iniciar, continuando "
                   "sem CDN.\n";
    } else {
      Crown::g_cdn = &cdn;
    }

    if (!server.init(CROWN_SERVEROS, app)) {
      std::cerr << "[MAIN] FATAL: O servidor GRPC não conseguiu ligar.\n";
    }

    if (watcher.joinable()) {
      watcher.join();
    }

    Crown::g_cdn = nullptr;
    cdn.shutdown();
    Crown::g_shutdownComplete = true;
    Crown::g_shutdownCv.notify_all();
  } catch (const std::exception &e) {
    std::cerr << "[FATAL] " << e.what() << "\n";
    return 1;
  }

  return 0;
}
