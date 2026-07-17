#include "authMiddle.h"
#include "../jwt/validator.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include "../server/server.h"
#include "../utils/sanitize.h"

#include <iostream>

namespace Crown {

using nlohmann::json;

std::unordered_map<grpc::ServerContext *, AuthenticatedUser>
    AuthMiddleware::users_;
std::mutex AuthMiddleware::mutex_;

std::optional<AuthenticatedUser>
AuthMiddleware::GetUser(grpc::ServerContext *context) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = users_.find(context);
  if (it != users_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void AuthMiddleware::SetUser(grpc::ServerContext *context,
                             AuthenticatedUser user) {
  std::lock_guard<std::mutex> lock(mutex_);
  users_[context] = user;
}

std::pair<grpc::Status, AuthenticatedUser>
AuthMiddleware::ValidateIdToken(const std::string &id_token) {

  auto &validator = JwtValidator::instance();
  auto user = validator.validateToken(id_token);

  if (!user.has_value()) {
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                         "Invalid or expired token"),
            {}};
  }

  return {grpc::Status::OK, user.value()};
}

std::pair<grpc::Status, std::pair<std::string, std::string>>
AuthMiddleware::ExchangeCodeForToken(const std::string &authorization_code,
                                     const std::string &redirect_uri) {
  const char *client_id = std::getenv("GOOGLE_CLIENT_ID");
  const char *client_secret = std::getenv("GOOGLE_CLIENT_SECRET");

  if (!client_id || !client_secret) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "OAuth not configured"),
            {"", ""}};
  }

  httplib::Client req("https://oauth2.googleapis.com");

  json body;
  body["code"] = authorization_code;
  body["client_id"] = std::string(client_id);
  body["client_secret"] = std::string(client_secret);
  body["redirect_uri"] = redirect_uri;
  body["grant_type"] = "authorization_code";

  httplib::Headers headers = {{"Content-Type", "application/json"}};

  auto resposta = req.Post("/token", headers, body.dump(), "application/json");

  if (!resposta) {
    return {grpc::Status(grpc::StatusCode::UNAVAILABLE,
                         "Failed to connect to Google"),
            {"", ""}};
  }

  if (resposta->status != 200) {
    json erro;
    try {
      erro = json::parse(resposta->body);
    } catch (...) {
      erro = {};
    }
    std::string erro_msg = erro.value("error_description",
                                      "Failed to exchange authorization code");
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED, erro_msg),
            {"", ""}};
  }

  json dados;
  try {
    dados = json::parse(resposta->body);
  } catch (const json::parse_error &e) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "Invalid token response"),
            {"", ""}};
  }

  std::string access_token = dados.value("access_token", "");
  std::string refresh_token = dados.value("refresh_token", "");
  std::string id_token = dados.value("id_token", "");

  if (id_token.empty()) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "No id_token in response"),
            {"", ""}};
  }

  return {grpc::Status::OK, {id_token, refresh_token}};
}

std::pair<grpc::Status, std::string>
AuthMiddleware::RefreshAccessToken(const std::string &refresh_token) {
  const char *client_id = std::getenv("GOOGLE_CLIENT_ID");
  const char *client_secret = std::getenv("GOOGLE_CLIENT_SECRET");

  if (!client_id || !client_secret) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "OAuth not configured"),
            ""};
  }

  httplib::Client req("https://oauth2.googleapis.com");

  json body;
  body["client_id"] = std::string(client_id);
  body["client_secret"] = std::string(client_secret);
  body["refresh_token"] = refresh_token;
  body["grant_type"] = "refresh_token";

  httplib::Headers headers = {{"Content-Type", "application/json"}};

  auto resposta = req.Post("/token", headers, body.dump(), "application/json");

  if (!resposta) {
    return {grpc::Status(grpc::StatusCode::UNAVAILABLE,
                         "Failed to connect to Google"),
            ""};
  }

  if (resposta->status != 200) {
    json erro;
    try {
      erro = json::parse(resposta->body);
    } catch (...) {
      erro = {};
    }
    std::string erro_msg =
        erro.value("error_description", "Failed to refresh access token");
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED, erro_msg), ""};
  }

  json dados;
  try {
    dados = json::parse(resposta->body);
  } catch (const json::parse_error &e) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "Invalid token response"),
            ""};
  }

  std::string id_token = dados.value("id_token", "");

  if (id_token.empty()) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "No id_token in response"),
            ""};
  }

  return {grpc::Status::OK, id_token};
}

AuthInterceptor::AuthInterceptor(grpc::experimental::ServerRpcInfo *info)
    : info_(info) {}

bool AuthInterceptor::isPublicMethod(const std::string &method) const {
  return method.find("Login") != std::string::npos ||
         method.find("RefreshAccessToken") != std::string::npos ||
         method.find("GetPost") != std::string::npos ||
         method.find("GetProfile") != std::string::npos ||
         method.find("SearchUsers") != std::string::npos ||
         method.find("GetFollowers") != std::string::npos ||
         method.find("GetFollowing") != std::string::npos ||
         method.find("ListLikes") != std::string::npos ||
         method.find("ListComments") != std::string::npos ||
         method.find("ListNotifications") != std::string::npos;
}

static std::string ExtractCookie(const std::string &cookie_header,
                                 const std::string &name) {
  std::string search = name + "=";
  auto pos = cookie_header.find(search);
  if (pos == std::string::npos)
    return "";

  pos += search.size();
  auto end = cookie_header.find(';', pos);
  if (end == std::string::npos)
    end = cookie_header.size();

  return cookie_header.substr(pos, end - pos);
}

void AuthInterceptor::validateToken(
    std::multimap<grpc::string_ref, grpc::string_ref> *recv_metadata) {

  std::string method(info_->method());

  if (isPublicMethod(method))
    return;

  std::string token;

  auto it = recv_metadata->find("authorization");
  if (it != recv_metadata->end()) {
    std::string raw(it->second.data(), it->second.size());
    if (raw.size() > 7 && raw.substr(0, 7) == "Bearer ") {
      token = raw.substr(7);
    } else {
      token = raw;
    }
  }

  if (token.empty()) {
    auto cookie_it = recv_metadata->find("cookie");
    if (cookie_it != recv_metadata->end()) {
      std::string cookie_str(cookie_it->second.data(),
                             cookie_it->second.size());
      token = ExtractCookie(cookie_str, "id_token");
    }
  }

  if (token.empty()) {
    token_valid_ = false;
    return;
  }

  auto [status, user] = AuthMiddleware::ValidateIdToken(token);
  if (!status.ok()) {
    token_valid_ = false;
    return;
  }

  auto *ctx = static_cast<grpc::ServerContext *>(info_->server_context());
  AuthMiddleware::SetUser(ctx, user);
}

void AuthInterceptor::Intercept(
    grpc::experimental::InterceptorBatchMethods *methods) {

  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::
              POST_RECV_INITIAL_METADATA)) {
    auto *recv_metadata = methods->GetRecvInitialMetadata();
    if (recv_metadata) {
      validateToken(recv_metadata);
    }
  }

  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
    if (!token_valid_) {
      methods->ModifySendStatus(grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                                             "Invalid or missing token"));
    }
  }

  methods->Proceed();
}

grpc::experimental::Interceptor *
AuthInterceptorFactory::CreateServerInterceptor(
    grpc::experimental::ServerRpcInfo *info) {
  return new AuthInterceptor(info);
}

} // namespace Crown
