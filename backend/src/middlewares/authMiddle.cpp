#include "authMiddle.h"
#include "../server/server.h"
#include "../utils/sanitize.h"

#include <cstdlib>
#include <curl/curl.h>
#include <iostream>

namespace Crown {

std::unordered_map<grpc::ServerContext *, AuthenticatedUser>
    AuthMiddleware::users_;
std::mutex AuthMiddleware::mutex_;

static size_t CurlWriteCallback(void *contents, size_t size, size_t nmemb,
                                std::string *userp) {
  userp->append(static_cast<char *>(contents), size * nmemb);
  return size * nmemb;
}

static std::string ExtractJsonString(const std::string &json,
                                     const std::string &key) {
  std::string search = "\"" + key + "\"";
  auto pos = json.find(search);
  if (pos == std::string::npos)
    return "";

  pos = json.find("\"", pos + search.size());
  if (pos == std::string::npos)
    return "";
  pos++;

  auto end = json.find("\"", pos);
  if (end == std::string::npos)
    return "";

  return json.substr(pos, end - pos);
}

static int ExtractJsonInt(const std::string &json, const std::string &key) {
  std::string search = "\"" + key + "\"";
  auto pos = json.find(search);
  if (pos == std::string::npos)
    return 0;

  pos = json.find(":", pos + search.size());
  if (pos == std::string::npos)
    return 0;
  pos++;

  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
    pos++;

  auto end = pos;
  while (end < json.size() && json[end] >= '0' && json[end] <= '9')
    end++;

  if (end == pos)
    return 0;
  return std::stoi(json.substr(pos, end - pos));
}

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
AuthMiddleware::ValidateGoogleToken(const std::string &access_token) {
  // Uma chamada só: userinfo retorna sub, email, name, picture
  CURL *curl = curl_easy_init();
  if (!curl) {
    return {grpc::Status(grpc::StatusCode::INTERNAL,
                         "Failed to initialize HTTP client"),
            {}};
  }

  std::string response;
  struct curl_slist *headers = nullptr;
  std::string auth_header = "Authorization: Bearer " + access_token;
  headers = curl_slist_append(headers, auth_header.c_str());

  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://www.googleapis.com/oauth2/v3/userinfo");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  if (res != CURLE_OK) {
    return {
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to validate token"),
        {}};
  }

  if (http_code != 200) {
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                         "Invalid or expired token"),
            {}};
  }

  AuthenticatedUser user;
  user.id = ExtractJsonString(response, "sub");
  user.email = ExtractJsonString(response, "email");
  user.name = ExtractJsonString(response, "given_name");
  user.picture = DecodeUnicodeEscapes(ExtractJsonString(response, "picture"));

  if (user.id.empty()) {
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                         "Invalid token response"),
            {}};
  }

  return {grpc::Status::OK, user};
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

  CURL *curl = curl_easy_init();
  if (!curl) {
    return {grpc::Status(grpc::StatusCode::INTERNAL,
                         "Failed to initialize HTTP client"),
            {"", ""}};
  }

  std::string post_fields =
      "code=" + authorization_code + "&client_id=" + std::string(client_id) +
      "&client_secret=" + std::string(client_secret) +
      "&redirect_uri=" + redirect_uri + "&grant_type=authorization_code";

  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return {
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "Failed to exchange code"),
        {"", ""}};
  }

  if (http_code != 200) {
    std::string error = ExtractJsonString(response, "error_description");
    if (error.empty())
      error = "Failed to exchange authorization code";
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED, error), {"", ""}};
  }

  std::string access_token = ExtractJsonString(response, "access_token");
  std::string refresh_token = ExtractJsonString(response, "refresh_token");

  if (access_token.empty()) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "Invalid token response"),
            {"", ""}};
  }

  return {grpc::Status::OK, {access_token, refresh_token}};
}

std::pair<grpc::Status, std::string>
AuthMiddleware::RefreshAccessToken(const std::string &refresh_token) {
  const char *client_id = std::getenv("GOOGLE_CLIENT_ID");
  const char *client_secret = std::getenv("GOOGLE_CLIENT_SECRET");

  if (!client_id || !client_secret) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "OAuth not configured"),
            ""};
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    return {grpc::Status(grpc::StatusCode::INTERNAL,
                         "Failed to initialize HTTP client"),
            ""};
  }

  std::string post_fields =
      "client_id=" + std::string(client_id) +
      "&client_secret=" + std::string(client_secret) +
      "&refresh_token=" + refresh_token + "&grant_type=refresh_token";

  std::string response;

  curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return {grpc::Status(grpc::StatusCode::UNAVAILABLE,
                         "Failed to refresh token"),
            ""};
  }

  if (http_code != 200) {
    std::string error = ExtractJsonString(response, "error_description");
    if (error.empty())
      error = "Failed to refresh access token";
    return {grpc::Status(grpc::StatusCode::UNAUTHENTICATED, error), ""};
  }

  std::string access_token = ExtractJsonString(response, "access_token");

  if (access_token.empty()) {
    return {grpc::Status(grpc::StatusCode::INTERNAL, "Invalid token response"),
            ""};
  }

  return {grpc::Status::OK, access_token};
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

  // Tenta Authorization header
  auto it = recv_metadata->find("authorization");
  if (it != recv_metadata->end()) {
    std::string raw(it->second.data(), it->second.size());
    if (raw.size() > 7 && raw.substr(0, 7) == "Bearer ") {
      token = raw.substr(7);
    } else {
      token = raw;
    }
  }

  // Se não tem Authorization, tenta cookie access_token
  if (token.empty()) {
    auto cookie_it = recv_metadata->find("cookie");
    if (cookie_it != recv_metadata->end()) {
      std::string cookie_str(cookie_it->second.data(),
                             cookie_it->second.size());
      token = ExtractCookie(cookie_str, "access_token");
    }
  }

  if (token.empty()) {
    token_valid_ = false;
    return;
  }

  auto [status, user] = AuthMiddleware::ValidateGoogleToken(token);
  if (!status.ok()) {
    token_valid_ = false;
    return;
  }

  auto *ctx = static_cast<grpc::ServerContext *>(info_->server_context());
  AuthMiddleware::SetUser(ctx, user);
}

void AuthInterceptor::Intercept(
    grpc::experimental::InterceptorBatchMethods *methods) {

  // metadata do cliente recebida → valida o token
  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::
              POST_RECV_INITIAL_METADATA)) {
    auto *recv_metadata = methods->GetRecvInitialMetadata();
    if (recv_metadata) {
      validateToken(recv_metadata);
    }
  }

  // antes de enviar status → sobrescreve com UNAUTHENTICATED se inválido
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
