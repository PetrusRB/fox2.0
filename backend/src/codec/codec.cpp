#include "./codec.h"
#include <algorithm>
#include <functional>
#include <handleapi.h>
#include <minwinbase.h>
#include <processthreadsapi.h>
#include <string>
#include <unordered_map>
#include <vips/vips8>

#include <cstring>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Crown {

constexpr int JPG_QUALITY = 60;
constexpr int WEBP_QUALITY = 70;
constexpr int GIF_QUALITY = 65;

CodecShit::CodecShit() { vips_init("fweb2"); }

CodecShit::~CodecShit() { vips_shutdown(); }

CodecShit &CodecShit::instance() {
  static CodecShit inst;
  return inst;
}

static const std::unordered_map<std::string, std::function<std::string(int)>>
    kSuffixBuilders = {
        {"png", [](int) { return std::string(".png[compression=9,strip]"); }},
        {"webp",
         [](int q) {
           return ".webp[Q=" + std::to_string(q > 0 ? q : WEBP_QUALITY) +
                  ",strip]";
         }},
        {"jpg",
         [](int q) {
           return ".jpg[Q=" + std::to_string(q > 0 ? q : JPG_QUALITY) +
                  ",strip]";
         }},
        {"jpeg",
         [](int q) {
           return ".jpg[Q=" + std::to_string(q > 0 ? q : JPG_QUALITY) +
                  ",strip]";
         }},
        {"gif",
         [](int q) {
           return ".webp[Q=" + std::to_string(q > 0 ? q : GIF_QUALITY) +
                  ",strip]";
         }},
        {"avif",
         [](int q) {
           return ".avif[Q=" + std::to_string(q > 0 ? q : WEBP_QUALITY) +
                  ",strip]";
         }},
        {"heic",
         [](int q) {
           return ".jpg[Q=" + std::to_string(q > 0 ? q : JPG_QUALITY) +
                  ",strip]";
         }},
        {"bmp",
         [](int q) {
           return ".jpg[Q=" + std::to_string(q > 0 ? q : JPG_QUALITY) +
                  ",strip]";
         }},
        {"tiff",
         [](int q) {
           return ".jpg[Q=" + std::to_string(q > 0 ? q : JPG_QUALITY) +
                  ",strip]";
         }},
};

CodecShit::Result CodecShit::compressImage(const std::string &input,
                                           const std::string &ext, int quality,
                                           int maxDimension) {
  Result r;

  try {
    vips::VImage img =
        vips::VImage::new_from_buffer(input.data(), input.size(), "");

    if (img.width() > maxDimension || img.height() > maxDimension) {
      double scale = static_cast<double>(maxDimension) /
                     std::max(img.width(), img.height());
      img = img.resize(scale);
    }
    auto it = kSuffixBuilders.find(ext);
    std::string suffix =
        (it != kSuffixBuilders.end())
            ? it->second(quality)
            : ".jpg[Q=" + std::to_string(JPG_QUALITY) + ",strip]";

    void *buf = nullptr;
    size_t len = 0;
    img.write_to_buffer(suffix.c_str(), &buf, &len, nullptr);

    r.data.assign(static_cast<char *>(buf), len);
    g_free(buf);

    if (r.data.size() < input.size()) {
      r.ok = true;
    } else {
      r.data = input;
      r.ok = true;
    }
  } catch (const vips::VError &e) {
    r.error = e.what();
  }
  return r;
}

CodecShit::Result
#ifdef _WIN32
CodecShit::runffmpegpipe(const std::string &input,
                         const std::vector<std::string> &args) {
  Result r;
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE childStdinRead = nullptr, childStdinWrite = nullptr;
  HANDLE childStdoutRead = nullptr, childStdoutWrite = nullptr;

  if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0) ||
      !SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
    r.error = "Falha ao criar pipe stdin";
    return r;
  }
  if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0) ||
      !SetHandleInformation(childStdoutWrite, HANDLE_FLAG_INHERIT, 0)) {
    r.error = "Falha ao criar pipe stdout";
    return r;
  }

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdInput = childStdinRead;
  si.hStdOutput = childStdoutRead;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

  PROCESS_INFORMATION pi{};

  std::wstring cmdLine = L"ffmpeg.exe";

  for (auto &a : args) {
    std::wstring wa(a.begin(), a.end());
    cmdLine += L" ";
    bool needsQuotes = wa.find(L' ') != std::wstring::npos;
    if (needsQuotes)
      cmdLine += L"\"";
    cmdLine += wa;
    if (needsQuotes)
      cmdLine += L"\"";
  }

  std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
  cmdBuf.push_back(L'\0');

  BOOL ok = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                           CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

  CloseHandle(childStdinRead);
  CloseHandle(childStdoutWrite);

  if (!ok) {
    CloseHandle(childStdinWrite);
    CloseHandle(childStdoutRead);
    r.error = "Falha ao iniciar o ffmpeg.exe";
    return r;
  }

  std::thread writer([&]() {
    const char *data = input.data();
    size_t remaining = input.size();
    while (remaining > 0) {
      DWORD written = 0;
      if (!WriteFile(childStdinWrite, data, static_cast<DWORD>(remaining),
                     &written, nullptr) ||
          written == 0) {
        break;
      }
      data += written;
      remaining -= written;
    }
    CloseHandle(childStdinWrite);
  });

  std::string output;
  char buf[65536];
  DWORD bytesRead = 0;

  while (ReadFile(childStdoutRead, buf, sizeof(buf), &bytesRead, nullptr) &&
         bytesRead > 0) {
    output.append(buf, bytesRead);
  }

  CloseHandle(childStdoutRead);
  writer.join();

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  if (exitCode == 0 && !output.empty()) {
    r.ok = true;
    if (output.size() > input.size()) {
      r.data = input;
    } else {
      r.data = std::move(output);
    }
  } else {
    r.error = "ffmpeg falhou (exit " + std::to_string(exitCode) + ").";
  }
  return r;
}
#else
CodecShit::runffmpegpipe(const std::string &input,
                         const std::vector<std::string> &args) {
  Result r;
  int inPipe[2], outPipe[2];
  if (::pipe(inPipe) != 0 || ::pipe(outPipe) != 0) {
    r.error = "Falha ao criar pipes.";
    return r;
  }

  pid_t pid = fork();
  if (pid < 0) {
    r.error = "Falha ao criar processo ffmpeg.";
    return r;
  }

  if (pid == 0) {
    dup2(inPipe[0], STDIN_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    close(inPipe[0]);
    close(inPipe[1]);
    close(outPipe[0]);
    close(outPipe[1]);

    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0)
      dup2(devnull, STDERR_FILENO);

    std::vector<char *> cargs;
    cargs.push_back(const_cast<char *>("ffmpeg"));
    for (auto &a : args)
      cargs.push_back(const_cast<char *>(a.c_str()));
    cargs.push_back(nullptr);
    execvp("ffmpeg", cargs.data());
    _exit(127);
  }
  close(inPipe[0]);
  close(outPipe[1]);

  std::thread writer([&]() {
    size_t off = 0;
    while (off < input.size()) {
      ssize_t n = write(inPipe[1], input.data() + off, input.size() - off);
      if (n <= 0)
        break;
      off += static_cast<size_t>(n);
    }
    close(inPipe[1]);
  });

  std::string output;
  char buf[65536];
  ssize_t n;
  while ((n = read(outPipe[0], buf, sizeof(buf))) > 0) {
    output.append(buf, static_cast<size_t>(n));
  }
  close(outPipe[0]);
  writer.join();

  int status = 0;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status) && WEXITSTATUS(status) == 0 && !output.empty()) {
    r.ok = true;
    r.data = std::move(output);
  } else {
    r.error = "ffmpeg falhou (exit " +
              std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1) +
              ").";
  }
  return r;
}
#endif
CodecShit::Result CodecShit::compressVideo(const std::string &input, int crf,
                                           int maxHeight) {
  std::vector<std::string> args = {"-y",
                                   "-i",
                                   "pipe:0",
                                   "-vf",
                                   "scale=-2:'min(" +
                                       std::to_string(maxHeight) + ",ih)'",
                                   "-c:v",
                                   "libx264",
                                   "-crf",
                                   std::to_string(crf),
                                   "-preset",
                                   "veryfast",
                                   "-c:a",
                                   "aac",
                                   "-b:a",
                                   "96k",
                                   "-movflags",
                                   "frag_keyframe+empty_moov+default_base_moof",
                                   "-f",
                                   "mp4",
                                   "pipe:1"};
  return runffmpegpipe(input, args);
}
} // namespace Crown
