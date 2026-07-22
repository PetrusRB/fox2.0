#pragma once

#include "./db/supabase.h"
#include "./utils/dotenv.h"
#include <string>

namespace Crown {
// --------------------  EXPORTADAS  --------------------
constexpr int ID_ACCESS_TOKEN_EXPIRES_SECONDS = 3600;
constexpr int ID_REFRESH_TOKEN_EXPIRES_SECONDS = 604800;

// ------------------------------------------------------
struct AppConfig {
  std::string supabaseHost;
  std::string supabaseKey;
  std::string corsOrigin;
  std::string redisUrl;
  std::string redisToken;

  std::string cdnBaseUrl;
  std::string cdnBucket;
  std::string cdnAccessKey;
  std::string cdnSecretKey;

  const std::string serverPrefix = "/";

  size_t limitImageBytes = 10 * 1024 * 1024; // 10MB
  size_t limitVideoBytes = 25 * 1024 * 1024; // 25MB

  static AppConfig FromEnv() {
    return {GetEnv("DATABASE_HOST"),
            GetEnv("DATABASE_ANON_KEY"),
            GetEnv("CORS_ORIGIN", "http://localhost:8080"),
            GetEnv("UPSTASH_REDIS_REST_URL"),
            GetEnv("UPSTASH_REDIS_REST_TOKEN"),
            GetEnv("CDN_BASE_URL"),
            GetEnv("CDN_BUCKET"),
            GetEnv("CDN_ACCESS_KEY"),
            GetEnv("CDN_SECRET_KEY")};
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
