#include "./cryptbaby.h"
#include <sodium.h>

namespace Crown {
std::string CryptBaby::generateSecId() {
  static constexpr char ALPHABET[] = "0123456789"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz";

  const size_t LENGTH = 27;

  std::string id;
  id.reserve(LENGTH);

  while (id.size() < LENGTH) {
    unsigned char b;
    randombytes_buf(&b, sizeof(b));

    if (b >= 248) {
      continue;
    }

    id.push_back(ALPHABET[b % 62]);
  }

  return id;
}
std::string CryptBaby::GenerateRandomHex(size_t len) {

  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(len * 2);

  for (size_t i = 0; i < len; i++) {
    unsigned char b;
    randombytes_buf(&b, sizeof(b));
    result.push_back(hex[b >> 4]);
    result.push_back(hex[b & 0x0F]);
  }
  return result;
}
std::string CryptBaby::Base64Decode(const std::string &input) {
  static const unsigned char lookup[256] = {
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57,
      58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,
      7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
      25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
      37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64};
  std::string output;
  output.reserve(((input.size() + 3) / 4) * 3);
  unsigned int val = 0;
  int bits = -8;
  for (char c : input) {
    if (c == '=' || c == '\n' || c == '\r')
      continue;
    unsigned char idx = static_cast<unsigned char>(c);
    if (idx >= 256 || lookup[idx] == 64)
      continue;
    val = (val << 6) | lookup[idx];
    bits += 6;
    if (bits >= 0) {
      output.push_back(static_cast<char>((val >> bits) & 0xFF));
      bits -= 8;
    }
  }
  return output;
}
} // namespace Crown
