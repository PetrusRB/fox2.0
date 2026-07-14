#pragma once

#include <bcrypt.h>
#include <cstdint>
#include <string>

#pragma comment(lib, "bcrypt.lib")

namespace Crown {

inline bool SecureRandomBytes(uint8_t *buf, size_t len) {
  return BCRYPT_SUCCESS(
      BCryptGenRandom(nullptr, buf, static_cast<ULONG>(len),
                      BCRYPT_USE_SYSTEM_PREFERRED_RNG));
}

inline std::string HmacSha1(const std::string &key,
                            const std::string &data) {
  BCRYPT_ALG_HANDLE hAlg = nullptr;
  BCRYPT_HASH_HANDLE hHash = nullptr;

  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &hAlg, BCRYPT_SHA1_ALGORITHM, nullptr,
          BCRYPT_ALG_HANDLE_HMAC_FLAG))) {
    return "";
  }

  if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, nullptr, 0,
                                       reinterpret_cast<PUCHAR>(const_cast<char *>(key.data())),
                                       static_cast<ULONG>(key.size()),
                                       BCRYPT_HASH_REUSABLE_FLAG))) {
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return "";
  }

  BCryptHashData(hHash,
                 reinterpret_cast<PUCHAR>(const_cast<char *>(data.data())),
                 static_cast<ULONG>(data.size()), 0);

  uint8_t hash[20];
  BCryptFinishHash(hHash, hash, sizeof(hash), 0);

  BCryptDestroyHash(hHash);
  BCryptCloseAlgorithmProvider(hAlg, 0);

  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(40);
  for (auto byte : hash) {
    result += hex[byte >> 4];
    result += hex[byte & 0x0F];
  }
  return result;
}

} // namespace Crown
