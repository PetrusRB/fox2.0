#pragma once

#include "../db/supabase.h"
#include "../utils/dotenv.h"
#include <string>

namespace Crown {

struct AppConfig {
  std::string supabaseHost;
  std::string supabaseKey;
  std::string imagekitPrivateKey;
  std::string corsOrigin;

  static AppConfig FromEnv() {
    return {
        GetEnv("DATABASE_HOST"),
        GetEnv("DATABASE_ANON_KEY"),
        GetEnv("CDN_IMAGEKIT_PRIVATE"),
        GetEnv("CORS_ORIGIN", "http://localhost:9000"),
    };
  }
};

class AppContext {
public:
  explicit AppContext(const AppConfig &config) : config_(config) {
    if (!config_.supabaseHost.empty() && !config_.supabaseKey.empty()) {
      db_.begin(config_.supabaseHost, config_.supabaseKey);
    }
  }

  Supabase::Client &db() { return db_; }
  const AppConfig &config() const { return config_; }

private:
  AppConfig config_;
  Supabase::Client db_;
};

} // namespace Crown
