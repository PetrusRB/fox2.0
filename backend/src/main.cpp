#include "./app.h"
#include "./server/http_server.h"
#include "./server/server.h"
#include "./utils/dotenv.h"
#include <curl/curl.h>
#include <exception>
#include <iostream>

#define SIGNATUROS 8081
#define CROWN_SERVEROS 50051

int main() {
  Crown::LoadEnv("backend/.env");

  curl_global_init(CURL_GLOBAL_DEFAULT);

  try {
    Crown::AppContext app(Crown::AppConfig::FromEnv());

    Crown::StartSignatureServer(SIGNATUROS, app);

    Crown::CrownServer server;
    server.init(CROWN_SERVEROS, app);
  } catch (const std::exception &e) {
    std::cerr << "[FATAL] " << e.what() << "\n";
    curl_global_cleanup();
    return 1;
  }

  curl_global_cleanup();
  return 0;
}
