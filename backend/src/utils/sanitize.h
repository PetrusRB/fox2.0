#pragma once
#include <string>

namespace Crown {
std::string SanitizeString(const std::string &input);
std::string SanitizeUrl(const std::string &input);
std::string DecodeUnicodeEscapes(const std::string &input);
} // namespace Crown
