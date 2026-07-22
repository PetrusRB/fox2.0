#pragma once

#include "../app.h"
#include "../codec/codec.h"
#include "../crypto/cryptbaby.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include "../libs/miniocpp/client.h"
#include "../utils/logger.h"
#include "../worker/worker.h"
#include <absl/strings/str_format.h>
#include <algorithm>
#include <atomic>
#include <bcrypt.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

namespace Crown {

inline CryptBaby babycrypt;
inline std::mutex g_babycryptMutex;

inline bool SecureRandomBytes(uint8_t *buf, size_t len) {
  return BCRYPT_SUCCESS(BCryptGenRandom(nullptr, buf, static_cast<ULONG>(len),
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG));
}

inline std::string SafeBase64Decode(const std::string &input) {
  std::lock_guard<std::mutex> lock(g_babycryptMutex);
  return babycrypt.Base64Decode(input);
}

inline std::string GetFileExtension(const std::string &fileName) {
  auto dot = fileName.rfind('.');
  if (dot == std::string::npos || dot == 0)
    return "";
  std::string ext = fileName.substr(dot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return ext;
}

// ----- MEDIA -----
enum class MediaKind { None, Image, Video };

struct MediaInfo {
  MediaKind kind;
  std::string contentType;
};

inline const std::unordered_map<std::string, MediaInfo> &MediaTable() {
  static const std::unordered_map<std::string, MediaInfo> table = {
      {"png", {MediaKind::Image, "image/png"}},
      {"jpg", {MediaKind::Image, "image/jpeg"}},
      {"jpeg", {MediaKind::Image, "image/jpeg"}},
      {"webp", {MediaKind::Image, "image/webp"}},
      {"gif", {MediaKind::Image, "image/gif"}},
      {"mp4", {MediaKind::Video, "video/mp4"}},
      {"webm", {MediaKind::Video, "video/webm"}},
      {"mov", {MediaKind::Video, "video/quicktime"}},
      {"avi", {MediaKind::Video, "video/x-msvideo"}},
      {"mkv", {MediaKind::Video, "video/x-matroska"}},
      {"m4v", {MediaKind::Video, "video/x-m4v"}},
  };
  return table;
}

inline const MediaInfo *LookupMedia(const std::string &ext) {
  const auto &table = MediaTable();
  auto it = table.find(ext);
  return it == table.end() ? nullptr : &it->second;
}

inline bool IsImageExtension(const std::string &ext) {
  auto *info = LookupMedia(ext);
  return info && info->kind == MediaKind::Image;
}

inline bool IsVideoExtension(const std::string &ext) {
  auto *info = LookupMedia(ext);
  return info && info->kind == MediaKind::Video;
}

inline size_t GetMaxSizeForExtension(const std::string &ext,
                                     const AppConfig &config) {
  auto *info = LookupMedia(ext);
  if (!info) {
    return 0;
  }
  return info->kind == MediaKind::Video ? config.limitVideoBytes
                                        : config.limitImageBytes;
}

inline std::string GetMediaTypeLabel(const std::string &ext) {
  auto *info = LookupMedia(ext);
  if (!info) {
    return "arquivo";
  }
  return info->kind == MediaKind::Video ? "video" : "imagem";
}

inline std::string GetContentType(const std::string &ext) {
  auto *info = LookupMedia(ext);
  return info ? info->contentType : "application/octet-stream";
}

inline std::string GenerateRandomHex(size_t len) {
  uint8_t buf[32];
  if (!SecureRandomBytes(buf, len > 32 ? 32 : len))
    return "";
  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(len * 2);
  for (size_t i = 0; i < len && i < 32; ++i) {
    result += hex[buf[i] >> 4];
    result += hex[buf[i] & 0x0F];
  }
  return result;
}

// Reutilizavel em qualquer situação onde precisa do aws s3.
struct S3ClientBundle {
  minio::creds::StaticProvider provider;
  minio::s3::BaseUrl base_url;
  std::shared_ptr<minio::s3::Client> client;

  S3ClientBundle(const AppConfig &cfg)
      : provider(cfg.cdnAccessKey, cfg.cdnSecretKey),
        base_url(cfg.cdnBaseUrl, false, "us-east-1"),
        client(std::make_shared<minio::s3::Client>(base_url, &provider)) {}
};

// para operações pesadas por causa incluição do WorkerPool (para filas
// encarreiradas). Por exemplo: rotas onde possam vim muitos requests de ações
// como deletar um post, criar posts e etc.
struct S3Context {
  S3ClientBundle bundle;
  WorkerPool pool;

  explicit S3Context(const AppConfig &cfg, size_t threads = 4)
      : bundle(cfg), pool(threads) {}
};

inline bool EnsureBucket(minio::s3::Client &client, const std::string &bucket) {
  minio::s3::BucketExistsArgs exists_args;
  exists_args.bucket = bucket;
  auto exists_resp = client.BucketExists(exists_args);
  if (!exists_resp) {
    std::cerr << "BucketExists: " << exists_resp.error() << "\n";
    return false;
  }
  if (exists_resp->exist)
    return true;

  minio::s3::MakeBucketArgs make_args;
  make_args.bucket = bucket;
  auto make_resp = client.MakeBucket(make_args);
  if (!make_resp) {
    std::cerr << "MakeBucket: " << make_resp.error() << "\n";
    return false;
  }
  return true;
}

inline std::expected<void, std::string>
S3PutObject(minio::s3::Client &client, const std::string &bucket,
            const std::string &objectKey, const std::string &contentType,
            const std::string &fileData) {
  if (fileData.empty()) {
    return std::unexpected("S3PutObject: o filedata esta vazio.");
  }

  std::istringstream stream(fileData);

  minio::s3::PutObjectArgs args(stream, fileData.size(), 16 * 1024 * 1024UL);
  args.bucket = bucket;
  args.object = objectKey;
  args.content_type = contentType;

  auto resp = client.PutObject(args);
  if (!resp) {
    return std::unexpected("S3PutObject error: " + resp.error().String());
  }
  return {};
}

inline std::expected<void, std::string>
S3DeleteObject(minio::s3::Client &client, const std::string &bucket,
               const std::string &objectKey) {
  minio::s3::RemoveObjectArgs args;
  args.bucket = bucket;
  args.object = objectKey;

  auto resp = client.RemoveObject(args);
  if (!resp) {
    return std::unexpected("RemoveObject Error: " + resp.error().String());
  }
  return {};
}

// ---------------------------------------------------------------------------------
// Servidor CDN (S3 + MinIO)
// ---------------------------------------------------------------------------------

class CdnServer {
public:
  CdnServer() = default;
  ~CdnServer() { shutdown(); }

  CdnServer(const CdnServer &) = delete;
  CdnServer &operator=(const CdnServer &) = delete;

  bool start(uint16_t port, AppContext &app) {
    app_ = &app;
    const auto &config = app.config();

    try {
      s3_ = std::make_unique<S3Context>(config);
    } catch (const std::exception &e) {
      std::cerr << "[CDN] Falha ao criar S3Context: " << e.what() << "\n";
      return false;
    }

    const std::string bucket = config.cdnBucket;
    std::cerr << "[CDN] Bucket: " << bucket << "\n";

    if (!EnsureBucket(*s3_->bundle.client, bucket)) {
      std::cerr << "[CDN] WARN: Bucket '" << bucket
                << "' nao esta disponivel (sera criado no primeiro upload se "
                   "possivel)\n";
    } else {
      std::cerr << "[CDN] Bucket '" << bucket << "' OK\n";
    }

    svr_ = std::make_unique<httplib::Server>();

    svr_->set_default_headers(
        {{"Access-Control-Allow-Origin", config.corsOrigin},
         {"Access-Control-Allow-Methods", "GET, POST, OPTIONS, DELETE"},
         {"Access-Control-Allow-Headers", "Content-Type"}});

    svr_->Options(".*", [](const httplib::Request &, httplib::Response &res) {
      res.status = 204;
    });

    registerRoutes(config, bucket);

    thread_ = std::thread([this, port]() {
      std::cerr << "[CDN] CDN server comecou em http://localhost:" << port
                << "\n";
      svr_->listen("0.0.0.0", port);
    });

    running_ = true;
    return true;
  }

  void shutdown() {
    if (!running_)
      return;
    running_ = false;

    if (svr_) {
      svr_->stop();
    }
    if (thread_.joinable()) {
      thread_.join();
    }
    svr_.reset();
    s3_.reset();
  }

  void
  deleteObjectAsync(const std::string &bucket, const std::string &objectKey,
                    const std::function<void(const std::string &)> &onError,
                    const std::function<void()> &onSuccess) {
    if (!s3_)
      return;
    s3_->pool.submit(
        [client = s3_->bundle.client, bucket, objectKey, onError, onSuccess]() {
          auto result = S3DeleteObject(*client, bucket, objectKey);
          if (!result) {
            if (onError) {
              onError(result.error());
            }
          } else {
            if (onSuccess) {
              onSuccess();
            }
          }
        });
  }

  bool isRunning() const { return running_; }

private:
  void registerRoutes(const AppConfig &config, const std::string &bucket) {

    svr_->Get("/api/cdn/check", [this](const httplib::Request &req,
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
          app_->db().from("midia_hashes").eq("hash", hash).limit(1).execute();

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
        std::string dbUrl = row["url"].get<std::string>();
        if (!dbUrl.empty() && dbUrl[0] == '/')
          dbUrl = dbUrl.substr(1);
        std::string host = req.get_header_value("Host");
        resp["url"] = "http://" + host + "/api/cdn/serve/" + dbUrl;
        resp["file_id"] = row["file_id"];
        res.set_content(resp.dump(), "application/json");
      } catch (const std::exception &e) {
        res.status = 500;
        res.set_content(
            std::string(R"({"error":"Falha ao consultar banco: ")") + e.what() +
                R"(","found":false})",
            "application/json");
      }
    });

    svr_->Get("/api/cdn/serve/(.*)", [bucket, this](const httplib::Request &req,
                                                    httplib::Response &res) {
      if (!s3_) {
        res.status = 500;
        res.set_content(R"({"error":"Servico CDN nao inicializado."})",
                        "application/json");
        return;
      }

      std::string objectKey = req.matches[1];
      if (objectKey.empty()) {
        res.status = 400;
        return;
      }

      minio::s3::GetObjectArgs args;
      args.bucket = bucket;
      args.object = objectKey;

      std::ostringstream body;
      args.datafunc = [&body](minio::http::DataFunctionArgs a) -> bool {
        body << a.datachunk;
        return true;
      };

      auto resp = s3_->bundle.client->GetObject(args);
      if (!resp) {
        res.status = 404;
        res.set_content(R"({"error":"Arquivo nao encontrado."})",
                        "application/json");
        return;
      }

      std::string ext = GetFileExtension(objectKey);
      res.set_content(body.str(), GetContentType(ext));
    });

    svr_->Post("/api/cdn/upload", [bucket, this](const httplib::Request &req,
                                                 httplib::Response &res) {
      try {
        if (!s3_) {
          std::cerr << "[CDN] s3 not initialized\n";
          res.status = 500;
          res.set_content(R"({"error":"Servico CDN nao inicializado."})",
                          "application/json");
          return;
        }
        auto &s3client = s3_->bundle.client;
        const auto &config = app_->config();

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
          res.set_content(
              R"({"error":"Campos 'file' e 'fileName' obrigatorios."})",
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

        size_t maxSize = GetMaxSizeForExtension(ext, config);
        if (maxSize == 0) {
          res.status = 415;
          res.set_content(
              R"({"error":"Formato nao suportado. Aceitos: PNG, JPG, WEBP, GIF, MP4, WEBM, MOV, AVI, MKV, M4V."})",
              "application/json");
          return;
        }

        std::string fileData = babycrypt.Base64Decode(fileBase64);
        std::cerr << "[CDN] decode OK, size=" << fileData.size() << "\n";
        auto &codec = CodecShit::instance();
        std::string finalExt = ext;

        if (IsImageExtension(ext)) {
          std::cerr << "[CDN] comprimindo imagem...\n";
          auto result = codec.compressImage(fileData, ext);
          if (result.ok) {
            fileData = std::move(result.data);
          }
          std::cerr << "[CDN] compressao OK, size=" << fileData.size() << "\n";
        } else if (IsVideoExtension(ext)) {
          std::cerr << "[CDN] comprimindo video...\n";
          auto result = codec.compressVideo(fileData);
          if (result.ok) {
            fileData = std::move(result.data);
            finalExt = "mp4";
          }
          std::cerr << "[CDN] compressao OK, size=" << fileData.size() << "\n";
        }

        if (fileData.size() > maxSize) {
          double sizeMB =
              static_cast<double>(fileData.size()) / (1024.0 * 1024.0);
          double limitMB = static_cast<double>(maxSize) / (1024.0 * 1024.0);
          nlohmann::json err;
          err["error"] =
              "Arquivo muito grande. " + GetMediaTypeLabel(ext) + " " +
              std::to_string(sizeMB).substr(
                  0, std::to_string(sizeMB).find('.') + 2) +
              "MB, limite " + std::to_string(static_cast<long long>(limitMB)) +
              "MB.";
          res.status = 413;
          res.set_content(err.dump(), "application/json");
          return;
        }

        std::string folder = "posts";
        if (body.contains("folder") &&
            !body["folder"].get<std::string>().empty()) {
          folder = body["folder"].get<std::string>();
        }

        std::string objectKey =
            folder + "/" + GenerateRandomHex(16) + "." + finalExt;
        std::string contentType = GetContentType(finalExt);

        std::cerr << "[CDN] S3PutObject start, bucket=" << bucket
                  << " key=" << objectKey << " size=" << fileData.size()
                  << " type=" << contentType << "\n";

        auto resultado =
            S3PutObject(*s3client, bucket, objectKey, contentType, fileData);

        if (!resultado) {
          std::cerr << "[CDN] S3PutObject FAILED\n";
          res.status = 502;
          res.set_content(R"({"error":"Falha ao enviar arquivo para CDN."})",
                          "application/json");
          return;
        }

        std::cerr << "[CDN] S3PutObject OK\n";

        nlohmann::json result;
        std::string host = req.get_header_value("Host");
        result["url"] = "http://" + host + "/api/cdn/serve/" + objectKey;
        result["fileId"] = objectKey;
        result["name"] = fileName;
        res.set_content(result.dump(), "application/json");
      } catch (const std::exception &e) {
        std::cerr << "[CDN] EXCECAO: " << e.what() << "\n";
        res.status = 500;
        res.set_content(R"({"error":"Excecao interna no upload."})",
                        "application/json");
      } catch (...) {
        std::cerr << "[CDN] EXCECAO DESCONHECIDA\n";
        res.status = 500;
        res.set_content(R"({"error":"Erro desconhecido no upload."})",
                        "application/json");
      }
    });

    svr_->Post("/api/cdn/register", [this](const httplib::Request &req,
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

      int status = app_->db().insert("midia_hashes", row.dump(), true);

      if (status >= 200 && status < 300) {
        res.set_content(R"({"ok":true})", "application/json");
      } else {
        res.status = 500;
        res.set_content(R"({"error":"Falha ao registrar hash no banco."})",
                        "application/json");
      }
    });
  }

  std::unique_ptr<httplib::Server> svr_;
  std::unique_ptr<S3Context> s3_;
  std::thread thread_;
  std::atomic<bool> running_{false};
  AppContext *app_ = nullptr;
};

inline CdnServer *g_cdn = nullptr;
inline void S3DeleteAsync(const std::string &bucket,
                          const std::string &objectKey) {
  if (g_cdn)
    g_cdn->deleteObjectAsync(
        bucket, objectKey,
        [](std::string erro) {
          std::cerr << "Erro async foi detectado: " << erro
                    << " Você tomou no olho da jaca." << "\n";
        },
        []() {
          std::cout
              << "O objeto foi deletado (EeEeEeEeE, GRAÇAS A DEEEEEEUSS)\n";
        });
}
} // namespace Crown
