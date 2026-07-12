#pragma once

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/interceptor.h>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace Crown {

struct AuthenticatedUser {
  std::string id;
  std::string email;
  std::string name;
  std::string picture;
};

class AuthMiddleware {
public:
  static std::optional<AuthenticatedUser> GetUser(grpc::ServerContext *context);
  static void SetUser(grpc::ServerContext *context, AuthenticatedUser user);

  static std::pair<grpc::Status, AuthenticatedUser>
  ValidateGoogleToken(const std::string &access_token);

  static std::pair<grpc::Status, std::pair<std::string, std::string>>
  ExchangeCodeForToken(const std::string &authorization_code,
                       const std::string &redirect_uri);

  static std::pair<grpc::Status, std::string>
  RefreshAccessToken(const std::string &refresh_token);

private:
  static std::unordered_map<grpc::ServerContext *, AuthenticatedUser> users_;
  static std::mutex mutex_;
};

class AuthInterceptor : public grpc::experimental::Interceptor {
public:
  explicit AuthInterceptor(grpc::experimental::ServerRpcInfo *info);

  void Intercept(grpc::experimental::InterceptorBatchMethods *methods) override;

private:
  bool isPublicMethod(const std::string &method) const;
  void validateToken(
      std::multimap<grpc::string_ref, grpc::string_ref> *recv_metadata);

  grpc::experimental::ServerRpcInfo *info_;
  bool token_valid_ = true;
};

class AuthInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
public:
  grpc::experimental::Interceptor *
  CreateServerInterceptor(grpc::experimental::ServerRpcInfo *info) override;
};

} // namespace Crown
