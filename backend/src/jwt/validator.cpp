#include "./validator.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include <chrono>
#include <iostream>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>

namespace Crown {

using nlohmann::json;

static std::vector<uint8_t> base64urlDecode(const std::string &input) {
  std::string s = input;
  for (char &c : s) {
    if (c == '-')
      c = '+';
    if (c == '_')
      c = '/';
  }
  while (s.length() % 4 != 0) {
    s += '=';
  }

  size_t out_len = s.size() / 4 * 3;
  if (s.size() >= 2 && s[s.size() - 1] == '=')
    out_len--;
  if (s.size() >= 1 && s[s.size() - 2] == '=')
    out_len--;

  std::vector<uint8_t> out(out_len);
  std::vector<unsigned char> temp(s.begin(), s.end());
  EVP_DecodeBlock(out.data(), temp.data(), temp.size());
  out.resize(out_len);
  return out;
}

static bool splitJwt(const std::string &token, std::string &header,
                     std::string &payload, std::string &signature) {
  size_t first_dot = token.find('.');
  if (first_dot == std::string::npos)
    return false;

  size_t second_dot = token.find('.', first_dot + 1);
  if (second_dot == std::string::npos)
    return false;

  header = token.substr(0, first_dot);
  payload = token.substr(first_dot + 1, second_dot - first_dot - 1);
  signature = token.substr(second_dot + 1);
  return true;
}

static std::string base64UrlToString(const std::string &b64url) {
  auto bytes = base64urlDecode(b64url);
  return std::string(bytes.begin(), bytes.end());
}

static bool verifySignature(const std::string &signed_data,
                            const std::string &signature_b64,
                            const std::string &n_b64,
                            const std::string &e_b64) {
  auto n_bytes = base64urlDecode(n_b64);
  auto e_bytes = base64urlDecode(e_b64);
  auto sig_bytes = base64urlDecode(signature_b64);

  BIGNUM *bn_n = BN_bin2bn(n_bytes.data(), n_bytes.size(), nullptr);
  BIGNUM *bn_e = BN_bin2bn(e_bytes.data(), e_bytes.size(), nullptr);

  if (!bn_n || !bn_e) {
    BN_free(bn_n);
    BN_free(bn_e);
    return false;
  }

  OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
  OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, bn_n);
  OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, bn_e);

  OSSL_PARAM *params = OSSL_PARAM_BLD_to_param(bld);

  EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
  EVP_PKEY *pkey = nullptr;
  EVP_PKEY_fromdata_init(pctx);
  EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params);

  BN_free(bn_n);
  BN_free(bn_e);
  OSSL_PARAM_BLD_free(bld);
  OSSL_PARAM_free(params);
  EVP_PKEY_CTX_free(pctx);

  if (!pkey) {
    return false;
  }

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey);
  EVP_DigestVerifyUpdate(mdctx, signed_data.c_str(), signed_data.size());
  int result = EVP_DigestVerifyFinal(mdctx, sig_bytes.data(), sig_bytes.size());

  EVP_MD_CTX_free(mdctx);
  EVP_PKEY_free(pkey);

  return result == 1;
}

JwtValidator &JwtValidator::instance() {
  static JwtValidator inst;
  return inst;
}

JwtValidator::JwtValidator()
    : lastFetch_(std::chrono::steady_clock::now() -
                 std::chrono::seconds(KEY_TTL_SECONDS + 1)) {}

bool JwtValidator::needsRefresh() const {
  auto elapsed = std::chrono::steady_clock::now() - lastFetch_;
  return elapsed >= std::chrono::seconds(KEY_TTL_SECONDS);
}

void JwtValidator::fetchGoogleKeys() {
  httplib::SSLClient req("www.googleapis.com");
  req.set_connection_timeout(10);
  req.set_read_timeout(10);
  req.enable_server_certificate_verification(false);

  auto resposta = req.Get("/oauth2/v3/certs");

  if (!resposta) {
    std::cerr << "Falha na conexao com Google JWKS\n";
    return;
  }

  if (resposta->status != 200) {
    std::cerr << "Erro HTTP: " << resposta->status << "\n";
    return;
  }

  try {
    json dados = json::parse(resposta->body);

    if (!dados.contains("keys") || !dados["keys"].is_array()) {
      std::cerr << "Resposta JWKS invalida\n";
      return;
    }

    std::vector<PublicKey> novas_chaves;

    for (const auto &chave : dados["keys"]) {
      std::string kid = chave.value("kid", "");
      std::string n = chave.value("n", "");
      std::string e = chave.value("e", "");

      if (!kid.empty() && !n.empty() && !e.empty()) {
        novas_chaves.push_back({kid, n, e});
      }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    keys_ = std::move(novas_chaves);
    lastFetch_ = std::chrono::steady_clock::now();

    std::cout << "JWKS: " << keys_.size() << " chaves carregadas\n";

  } catch (const json::parse_error &e) {
    std::cerr << "Erro ao parsear JWKS: " << e.what() << "\n";
  }
}

std::optional<PublicKey> JwtValidator::findKey(const std::string &kid) {
  for (const auto &key : keys_) {
    if (key.kid == kid) {
      return key;
    }
  }
  return std::nullopt;
}

std::optional<AuthenticatedUser>
JwtValidator::validateToken(const std::string &token) {
  std::string header_b64, payload_b64, signature_b64;

  if (!splitJwt(token, header_b64, payload_b64, signature_b64)) {
    std::cerr << "JWT: formato invalido\n";
    return std::nullopt;
  }

  json header;
  try {
    header = json::parse(base64UrlToString(header_b64));
  } catch (...) {
    std::cerr << "JWT: header invalido\n";
    return std::nullopt;
  }

  std::string kid = header.value("kid", "");
  std::string alg = header.value("alg", "");

  if (alg != "RS256") {
    std::cerr << "JWT: algoritmo nao suportado: " << alg << "\n";
    return std::nullopt;
  }

  if (kid.empty()) {
    std::cerr << "JWT: kid ausente no header\n";
    return std::nullopt;
  }

  if (needsRefresh()) {
    fetchGoogleKeys();
  }

  auto key = findKey(kid);
  if (!key.has_value()) {
    std::cerr << "JWT: chave nao encontrada para kid=" << kid
              << " (chaves: " << keys_.size() << ")\n";
    fetchGoogleKeys();
    key = findKey(kid);
    if (!key.has_value()) {
      std::cerr << "JWT: chave ainda nao apos refresh\n";
      return std::nullopt;
    }
  }

  std::string signed_data = header_b64 + "." + payload_b64;
  if (!verifySignature(signed_data, signature_b64, key->n, key->e)) {
    std::cerr << "JWT: assinatura invalida\n";
    return std::nullopt;
  }

  json payload;
  try {
    payload = json::parse(base64UrlToString(payload_b64));
  } catch (...) {
    std::cerr << "JWT: payload invalido\n";
    return std::nullopt;
  }

  std::string iss = payload.value("iss", "");
  if (iss != "https://accounts.google.com" && iss != "accounts.google.com") {
    std::cerr << "JWT: issuer invalido: " << iss << "\n";
    return std::nullopt;
  }

  auto now = std::chrono::system_clock::now();
  auto exp = std::chrono::seconds(payload.value("exp", 0));
  auto now_secs =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

  if (now_secs >= exp) {
    std::cerr << "JWT: token expirado\n";
    return std::nullopt;
  }

  AuthenticatedUser user;
  user.id = payload.value("sub", "");
  user.email = payload.value("email", "");
  user.name = payload.value("given_name", "");
  user.picture = payload.value("picture", "");

  if (user.id.empty()) {
    std::cerr << "JWT: sub ausente\n";
    return std::nullopt;
  }

  return user;
}

} // namespace Crown
