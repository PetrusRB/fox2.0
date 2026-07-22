#pragma once
#include <string>

namespace Crown {

class CryptBaby {
public:
  std::string generateSecId();
  std::string Base64Decode(const std::string &input);
  std::string GenerateRandomHex(size_t len);

private:
};
} // namespace Crown
