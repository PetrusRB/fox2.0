#pragma once
#include <string>

namespace Crown {

class CryptBaby {
public:
  std::string generateSecId();
  std::string Base64Decode(const std::string &input);

private:
};
} // namespace Crown
