#pragma once

#include <chrono>
#include <deque>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/interceptor.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Crown {

struct RouteConfig {
  std::string method;
  int max_requests;
  std::chrono::seconds window;
};

class RateLimiter {
public:
  RateLimiter(const std::vector<RouteConfig> &routes);

  bool allowRequest(const std::string &method, const std::string &client_ip);

private:
  std::vector<RouteConfig> routes_;
  std::unordered_map<std::string,
                     std::unordered_map<std::string, std::deque<int64_t>>>
      usage_;
  std::mutex mutex_;

  const RouteConfig *findRoute(const std::string &method) const;
};

class RatelimitIntercept : public grpc::experimental::Interceptor {
public:
  RatelimitIntercept(grpc::experimental::ServerRpcInfo *info,
                     RateLimiter *limiter);

  void Intercept(grpc::experimental::InterceptorBatchMethods *methods) override;

private:
  grpc::experimental::ServerRpcInfo *info_;
  RateLimiter *limiter_;
  bool rejected_ = false;
};

class RatelimitInterceptFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
public:
  RatelimitInterceptFactory(const std::vector<RouteConfig> &routes);

  grpc::experimental::Interceptor *
  CreateServerInterceptor(grpc::experimental::ServerRpcInfo *info) override;

private:
  RateLimiter limiter_;
};

} // namespace Crown
