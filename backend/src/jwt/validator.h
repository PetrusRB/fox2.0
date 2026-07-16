#pragma once
#include "../middlewares/authMiddle.h"
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
  void fetchGoogleKeys();
  std::optional<PublicKey> findKey(const std::string &kid);
  std::optional<AuthenticatedUser> validateToken(const std::string &token);

private:
  std::vector<PublicKey> keys_;
  std::mutex mutex_;
};

} // namespace Crown
