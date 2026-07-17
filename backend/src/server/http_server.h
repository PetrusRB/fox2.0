#pragma once

#include "../app.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include <chrono>
#include <cstdlib>
#include <imagekit/imagekit_signature.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace Crown {

constexpr uint64_t SIGNATURE_TTL_SECONDS = 55 * 60; // 55 minutitos

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
        app.db().from("image_hashes").eq("hash", hash).limit(1).execute();

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

    int status = app.db().insert("image_hashes", row.dump(), true);

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
