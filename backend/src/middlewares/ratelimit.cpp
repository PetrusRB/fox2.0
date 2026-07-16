#include "./ratelimit.h"
#include <iostream>

namespace Crown {

static int64_t nowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

RateLimiter::RateLimiter(const std::vector<RouteConfig> &routes)
    : routes_(routes) {}

const RouteConfig *RateLimiter::findRoute(const std::string &method) const {
  for (const auto &route : routes_) {
    if (method.find(route.method) != std::string::npos) {
      return &route;
    }
  }
  return nullptr;
}

bool RateLimiter::allowRequest(const std::string &method,
                               const std::string &client_ip) {
  const RouteConfig *route = findRoute(method);
  if (!route)
    return true;

  std::lock_guard<std::mutex> lock(mutex_);

  auto &route_usage = usage_[client_ip];
  auto &timestamps = route_usage[method];

  int64_t cutoff = nowSeconds() - route->window.count();

  while (!timestamps.empty() && timestamps.front() <= cutoff) {
    timestamps.pop_front();
  }

  if (static_cast<int>(timestamps.size()) >= route->max_requests) {
    return false;
  }

  timestamps.push_back(nowSeconds());
  return true;
}

RatelimitIntercept::RatelimitIntercept(
    grpc::experimental::ServerRpcInfo *info, RateLimiter *limiter)
    : info_(info), limiter_(limiter) {}

static std::string getClientIp(
    grpc::experimental::InterceptorBatchMethods *methods) {
  auto *metadata = methods->GetRecvInitialMetadata();
  if (!metadata)
    return "unknown";

  auto it = metadata->find("x-forwarded-for");
  if (it != metadata->end()) {
    std::string ip(it->second.data(), it->second.size());
    auto comma = ip.find(',');
    if (comma != std::string::npos)
      ip = ip.substr(0, comma);
    return ip;
  }

  auto peer_it = metadata->find(":authority");
  if (peer_it != metadata->end()) {
    std::string peer(peer_it->second.data(), peer_it->second.size());
    return peer;
  }

  return "unknown";
}

void RatelimitIntercept::Intercept(
    grpc::experimental::InterceptorBatchMethods *methods) {

  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::
              POST_RECV_INITIAL_METADATA)) {

    std::string method(info_->method());
    std::string ip = getClientIp(methods);

    if (!limiter_->allowRequest(method, ip)) {
      rejected_ = true;
    }
  }

  if (methods->QueryInterceptionHookPoint(
          grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
    if (rejected_) {
      methods->ModifySendStatus(grpc::Status(
          grpc::StatusCode::RESOURCE_EXHAUSTED,
          "Rate limit exceeded. Try again later."));
    }
  }

  methods->Proceed();
}

RatelimitInterceptFactory::RatelimitInterceptFactory(
    const std::vector<RouteConfig> &routes)
    : limiter_(routes) {}

grpc::experimental::Interceptor *RatelimitInterceptFactory::CreateServerInterceptor(
    grpc::experimental::ServerRpcInfo *info) {
  return new RatelimitIntercept(info, &limiter_);
}

} // namespace Crown
