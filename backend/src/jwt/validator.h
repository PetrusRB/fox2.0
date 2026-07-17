#pragma once
#include "../middlewares/authMiddle.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Crown {

struct PublicKey {
  std::string kid;
  std::string n;
  std::string e;
};

class JwtValidator {
public:
  static JwtValidator &instance();

  std::optional<AuthenticatedUser> validateToken(const std::string &token);

private:
  JwtValidator();
  void fetchGoogleKeys();
  std::optional<PublicKey> findKey(const std::string &kid);
  bool needsRefresh() const;

  std::vector<PublicKey> keys_;
  std::mutex mutex_;
  std::chrono::steady_clock::time_point lastFetch_;
  static constexpr int KEY_TTL_SECONDS = 3600;
};

} // namespace Crown
