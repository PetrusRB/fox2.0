#pragma once

#include "../app.h"
#include "../crypto/cryptbaby.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <imagekit/imagekit_signature.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace Crown {

constexpr uint64_t SIGNATURE_TTL_SECONDS = 55 * 60; // 55 minutitos
CryptBaby babycrypt;

inline std::string GetFileExtension(const std::string &fileName) {
  auto dot = fileName.rfind('.');
  if (dot == std::string::npos || dot == 0)
    return "";
  std::string ext = fileName.substr(dot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext;
}

inline bool IsImageExtension(const std::string &ext) {
  return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" ||
         ext == "gif";
}

inline bool IsVideoExtension(const std::string &ext) {
  return ext == "mp4" || ext == "webm" || ext == "mov" || ext == "avi" ||
         ext == "mkv" || ext == "m4v";
}

inline size_t GetMaxSizeForExtension(const std::string &ext,
                                     const AppConfig &config) {
  if (IsVideoExtension(ext))
    return config.limitVideoBytes;
  if (IsImageExtension(ext))
    return config.limitImageBytes;
  return 0;
}

inline std::string GetMediaTypeLabel(const std::string &ext) {
  if (ext == "mp4" || ext == "webm" || ext == "mov" || ext == "avi" ||
      ext == "mkv" || ext == "m4v")
    return "video";
  if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp" ||
      ext == "gif")
    return "imagem";
  return "arquivo";
}

inline std::string GenerateToken() {
  uint8_t buf[32];
  if (!SecureRandomBytes(buf, sizeof(buf))) {
    return "";
  }
  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(64);
  for (auto byte : buf) {
    result += hex[byte >> 4];
    result += hex[byte & 0x0F];
  }
  return result;
}

inline void StartSignatureServer(uint16_t port, AppContext &app) {
  auto svr = std::make_unique<httplib::Server>();

  svr->set_default_headers(
      {{"Access-Control-Allow-Origin", app.config().corsOrigin},
       {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
       {"Access-Control-Allow-Headers", "Content-Type"}});

  svr->Options(".*", [](const httplib::Request &, httplib::Response &res) {
    res.status = 204;
  });

  svr->Get("/api/imagekit/signature", [&app](const httplib::Request &,
                                             httplib::Response &res) {
    std::string privKey = app.config().imagekitPrivateKey;
    if (privKey.empty()) {
      res.status = 500;
      res.set_content(R"({"error":"CDN_IMAGEKIT_PRIVATE nao foi setado."})",
                      "application/json");
      return;
    }

    auto now = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    uint64_t expire = now + SIGNATURE_TTL_SECONDS;

    std::string token = GenerateToken();
    if (token.empty()) {
      res.status = 500;
      res.set_content(R"({"error":"Falha ao gerar token."})",
                      "application/json");
      return;
    }

    std::string signature = HmacSha1(privKey, token + std::to_string(expire));

    if (signature.empty()) {
      res.status = 500;
      res.set_content(R"({"error":"Falha ao gerar assinatura."})",
                      "application/json");
      return;
    }

    nlohmann::json resp;
    resp["signature"] = signature;
    resp["token"] = token;
    resp["expire"] = expire;

    res.set_content(resp.dump(), "application/json");
  });

  svr->Get("/api/imagekit/check", [&app](const httplib::Request &req,
                                         httplib::Response &res) {
    auto hashIt = req.params.find("hash");
    if (hashIt == req.params.end() || hashIt->second.empty()) {
      res.status = 400;
      res.set_content(R"({"error":"Parametro 'hash' obrigatorio."})",
                      "application/json");
      return;
    }

    std::string hash = hashIt->second;
    std::string result =
        app.db().from("midia_hashes").eq("hash", hash).limit(1).execute();

    if (result.empty() || result == "[]") {
      res.set_content(R"({"found":false})", "application/json");
      return;
    }

    try {
      auto rows = nlohmann::json::parse(result);
      if (!rows.is_array() || rows.empty()) {
        res.set_content(R"({"found":false})", "application/json");
        return;
      }
      auto &row = rows[0];
      if (!row.contains("url") || !row.contains("file_id")) {
        res.status = 500;
        res.set_content(
            R"({"error":"Registro incompleto no banco.","found":false})",
            "application/json");
        return;
      }
      nlohmann::json resp;
      resp["found"] = true;
      resp["url"] = row["url"];
      resp["file_id"] = row["file_id"];
      res.set_content(resp.dump(), "application/json");
      return;
    } catch (const std::exception &e) {
      res.status = 500;
      res.set_content(std::string(R"({"error":"Falha ao consultar banco: ")") +
                          e.what() + R"(","found":false})",
                      "application/json");
      return;
    }
  });
  svr->Post("/api/imagekit/upload", [&app](const httplib::Request &req,
                                           httplib::Response &res) {
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body);
    } catch (...) {
      res.status = 400;
      res.set_content(R"({"error":"JSON invalido."})", "application/json");
      return;
    }

    if (!body.contains("file") || !body.contains("fileName")) {
      res.status = 400;
      res.set_content(R"({"error":"Campos 'file' e 'fileName' obrigatorios."})",
                      "application/json");
      return;
    }

    std::string fileBase64 = body["file"].get<std::string>();
    std::string fileName = body["fileName"].get<std::string>();

    std::string ext = GetFileExtension(fileName);
    if (ext.empty()) {
      res.status = 400;
      res.set_content(R"({"error":"Extensao do arquivo invalida."})",
                      "application/json");
      return;
    }

    size_t maxSize = GetMaxSizeForExtension(ext, app.config());
    if (maxSize == 0) {
      res.status = 415;
      res.set_content(
          R"({"error":"Formato nao suportado. Aceitos: PNG, JPG, WEBP, GIF, MP4, WEBM, MOV, AVI, MKV, M4V."})",
          "application/json");
      return;
    }

    std::string fileData = babycrypt.Base64Decode(fileBase64);
    if (fileData.size() > maxSize) {
      double sizeMB = static_cast<double>(fileData.size()) / (1024.0 * 1024.0);
      double limitMB = static_cast<double>(maxSize) / (1024.0 * 1024.0);
      std::string label = GetMediaTypeLabel(ext);
      char buf[256];
      snprintf(
          buf, sizeof(buf),
          "{\"error\":\"Arquivo muito grande. %s %.1fMB, limite %.0fMB.\"}",
          label.c_str(), sizeMB, limitMB);
      res.status = 413;
      res.set_content(buf, "application/json");
      return;
    }

    std::string privKey = app.config().imagekitPrivateKey;
    if (privKey.empty()) {
      res.status = 500;
      res.set_content(R"({"error":"CDN_IMAGEKIT_PRIVATE nao foi setado."})",
                      "application/json");
      return;
    }

    std::string folder = "";
    if (body.contains("folder")) {
      folder = body["folder"].get<std::string>();
    }

    std::vector<std::string> tags;
    if (body.contains("tags") && body["tags"].is_array()) {
      for (auto &t : body["tags"]) {
        tags.push_back(t.get<std::string>());
      }
    }

    httplib::SSLClient cli("upload.imagekit.io");
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    cli.enable_server_certificate_verification(true);
#endif

    std::string boundary =
        "----CrownUploadBoundary" + GenerateToken().substr(0, 16);

    std::string mpBody;
    auto addField = [&](const std::string &name, const std::string &value) {
      mpBody += "--" + boundary + "\r\n";
      mpBody += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
      mpBody += value + "\r\n";
    };

    addField("fileName", fileName);
    addField("useUniqueFileName", "true");
    if (!folder.empty()) {
      addField("folder", folder);
    }
    if (!tags.empty()) {
      std::string tagStr;
      for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0)
          tagStr += ",";
        tagStr += tags[i];
      }
      addField("tags", tagStr);
    }

    std::string mimeTypes[] = {
        "image/png",        "image/jpeg", "image/webp",      "image/gif",
        "video/mp4",        "video/webm", "video/quicktime", "video/x-msvideo",
        "video/x-matroska", "video/x-m4v"};
    std::string extMimeMap[] = {"png",  "jpg", "webp", "gif", "mp4",
                                "webm", "mov", "avi",  "mkv", "m4v"};
    std::string contentType = "application/octet-stream";
    for (int i = 0; i < 10; ++i) {
      if (ext == extMimeMap[i]) {
        contentType = mimeTypes[i];
        break;
      }
    }

    mpBody += "--" + boundary + "\r\n";
    mpBody += "Content-Disposition: form-data; name=\"file\"; filename=\"" +
              fileName + "\"\r\n";
    mpBody += "Content-Type: " + contentType + "\r\n\r\n";
    mpBody += fileData;
    mpBody += "\r\n--" + boundary + "--\r\n";

    std::string authValue = "Basic ";
    {
      static const char table[] =
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
          "0123456789+/";
      std::string raw = privKey + ":";
      authValue.reserve(7 + ((raw.size() + 2) / 3) * 4);
      auto bytes = reinterpret_cast<const unsigned char *>(raw.data());
      for (size_t i = 0; i < raw.size(); i += 3) {
        unsigned int n = static_cast<unsigned int>(bytes[i]) << 16;
        if (i + 1 < raw.size())
          n |= static_cast<unsigned int>(bytes[i + 1]) << 8;
        if (i + 2 < raw.size())
          n |= static_cast<unsigned int>(bytes[i + 2]);
        authValue.push_back(table[(n >> 18) & 0x3F]);
        authValue.push_back(table[(n >> 12) & 0x3F]);
        authValue.push_back((i + 1 < raw.size()) ? table[(n >> 6) & 0x3F]
                                                 : '=');
        authValue.push_back((i + 2 < raw.size()) ? table[n & 0x3F] : '=');
      }
    }

    httplib::Headers headers = {{"Authorization", authValue}};

    auto response = cli.Post("/api/v1/files/upload", headers, mpBody,
                             "multipart/form-data; boundary=" + boundary);

    if (!response) {
      res.status = 502;
      res.set_content(R"({"error":"Falha ao conectar com ImageKit."})",
                      "application/json");
      return;
    }

    if (response->status < 200 || response->status >= 300) {
      std::string errMsg = "Erro ao enviar arquivo para ImageKit.";
      try {
        auto errBody = nlohmann::json::parse(response->body);
        if (errBody.contains("message")) {
          errMsg = errBody["message"].get<std::string>();
        }
      } catch (...) {
      }
      char buf[512];
      snprintf(buf, sizeof(buf), "{\"error\":\"%s\",\"status\":%d}",
               errMsg.c_str(), response->status);
      res.status = response->status;
      res.set_content(buf, "application/json");
      return;
    }

    try {
      auto ikResp = nlohmann::json::parse(response->body);
      nlohmann::json result;
      result["url"] = ikResp.value("url", "");
      result["fileId"] = ikResp.value("fileId", "");
      result["filePath"] = ikResp.value("filePath", "");
      result["name"] = ikResp.value("name", fileName);
      if (ikResp.contains("width"))
        result["width"] = ikResp["width"];
      if (ikResp.contains("height"))
        result["height"] = ikResp["height"];
      if (ikResp.contains("size"))
        result["size"] = ikResp["size"];
      res.set_content(result.dump(), "application/json");
    } catch (...) {
      res.status = 500;
      res.set_content(R"({"error":"Resposta invalida do ImageKit."})",
                      "application/json");
    }
  });

  svr->Post("/api/imagekit/register", [&app](const httplib::Request &req,
                                             httplib::Response &res) {
    nlohmann::json body;
    try {
      body = nlohmann::json::parse(req.body);
    } catch (...) {
      res.status = 400;
      res.set_content(R"({"error":"JSON invalido."})", "application/json");
      return;
    }

    if (!body.contains("hash") || !body.contains("url") ||
        !body.contains("file_id")) {
      res.status = 400;
      res.set_content(
          R"({"error":"Campos 'hash', 'url' e 'file_id' obrigatorios."})",
          "application/json");
      return;
    }

    std::string hash = body["hash"].get<std::string>();
    std::string url = body["url"].get<std::string>();
    std::string fileId = body["file_id"].get<std::string>();

    nlohmann::json row;
    row["hash"] = hash;
    row["url"] = url;
    row["file_id"] = fileId;

    int status = app.db().insert("midia_hashes", row.dump(), true);

    if (status >= 200 && status < 300) {
      res.set_content(R"({"ok":true})", "application/json");
    } else {
      res.status = 500;
      res.set_content(R"({"error":"Falha ao registrar hash no banco."})",
                      "application/json");
    }
  });

  auto *raw = svr.release();
  std::thread([raw, port]() {
    std::cout << "Signature CDN Image comecou em http://localhost:" << port
              << "\n";
    raw->listen("0.0.0.0", port);
  }).detach();
}

} // namespace Crown
