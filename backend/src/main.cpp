#include "./server/server.h"
#include "./utils/dotenv.h"
#include <curl/curl.h>
#include <exception>
#include <iostream>

int main() {
  Crown::LoadEnv("backend/.env");

  curl_global_init(CURL_GLOBAL_DEFAULT);

  try {
    Crown::CrownServer server;
    server.init(50051);
  } catch (const std::exception &e) {
    std::cerr << "[FATAL] " << e.what() << "\n";
    curl_global_cleanup();
    return 1;
  }

  curl_global_cleanup();
  return 0;
}
