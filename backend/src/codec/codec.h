#pragma once
#include <string>
#include <vector>

namespace Crown {
class CodecShit {
public:
  struct Result {
    bool ok = false;
    std::string data;
    std::string error;
  };
  static CodecShit &instance();

  Result compressImage(const std::string &input, const std::string &ext,
                       int quality = 0, int maxDimension = 1920);
  Result compressVideo(const std::string &input, int crf = 28,
                       int maxHeight = 1080);
  Result runffmpegpipe(const std::string &input,
                       const std::vector<std::string> &args);

private:
  CodecShit();
  ~CodecShit();
};
} // namespace Crown
