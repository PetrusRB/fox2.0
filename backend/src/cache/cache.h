#pragma once

#include "../libs/json.hpp"
#include <atomic>
#include <mutex>
#include <string>

#if __has_include("redisx.h")
#include "redisx.h"
#define REDISX_AVAILABLE 1
#else
#define REDISX_AVAILABLE 0
#endif

namespace Crown {

// TODO: gosto muito de minutos :3
constexpr int CACHE_TTL_POST = 300;       // 5 min
constexpr int CACHE_TTL_USER = 600;       // 10 min
constexpr int CACHE_TTL_FEED = 30;        // 30 seg
constexpr int CACHE_TTL_LIKE = 300;       // 5 min
constexpr int CACHE_TTL_POST_COUNT = 300; // 5 min

class CacheManager {
public:
  static CacheManager &instance() {
    static CacheManager inst;
    return inst;
  }

  bool init(const std::string &url) {
#if REDISX_AVAILABLE
    std::lock_guard<std::mutex> lock(mu_);
    redis_ = redisxInit(url.c_str());
    if (!redis_)
      return false;
    int status = redisxConnect(redis_, FALSE);
    connected_ = (status == 0);
    return connected_;
#else
    (void)url;
    return false;
#endif
  }

  bool is_connected() const { return connected_; }

  bool set(const std::string &key, const std::string &value,
           int ttl_seconds = 0) {
#if REDISX_AVAILABLE
    if (!connected_)
      return false;
    std::lock_guard<std::mutex> lock(mu_);
    int status = 0;
    if (ttl_seconds > 0) {
      auto *resp = redisxRequest(redis_, "SETEX", key.c_str(),
                                 std::to_string(ttl_seconds).c_str(),
                                 value.c_str(), &status);
      redisxDestroyRESP(resp);
    } else {
      auto *resp = redisxRequest(redis_, "SET", key.c_str(), value.c_str(),
                                 nullptr, &status);
      redisxDestroyRESP(resp);
    }
    return status == 0;
#else
    (void)key;
    (void)value;
    (void)ttl_seconds;
    return false;
#endif
  }

  bool set(const std::string &key, int value, int ttl_seconds = 0) {
    return set(key, std::to_string(value), ttl_seconds);
  }

  bool set_json(const std::string &key, const nlohmann::json &value,
                int ttl_seconds = 0) {
    return set(key, value.dump(), ttl_seconds);
  }

  std::string get(const std::string &key) {
#if REDISX_AVAILABLE
    if (!connected_)
      return "";
    std::lock_guard<std::mutex> lock(mu_);
    int status = 0;
    auto *resp =
        redisxRequest(redis_, "GET", key.c_str(), nullptr, nullptr, &status);
    if (status != 0 || !resp || resp->type != RESP_BULK_STRING) {
      redisxDestroyRESP(resp);
      return "";
    }
    std::string result(resp->value ? (char *)resp->value : "");
    redisxDestroyRESP(resp);
    return result;
#else
    (void)key;
    return "";
#endif
  }

  int get_int(const std::string &key, int default_value = 0) {
    auto val = get(key);
    if (val.empty())
      return default_value;
    try {
      return std::stoi(val);
    } catch (...) {
      return default_value;
    }
  }

  nlohmann::json get_json(const std::string &key) {
    auto val = get(key);
    if (val.empty())
      return nullptr;
    try {
      return nlohmann::json::parse(val);
    } catch (...) {
      return nullptr;
    }
  }

  bool del(const std::string &key) {
#if REDISX_AVAILABLE
    if (!connected_)
      return false;
    std::lock_guard<std::mutex> lock(mu_);
    int status = 0;
    auto *resp =
        redisxRequest(redis_, "DEL", key.c_str(), nullptr, nullptr, &status);
    redisxDestroyRESP(resp);
    return status == 0;
#else
    (void)key;
    return false;
#endif
  }

  bool del_pattern(const std::string &pattern) {
#if REDISX_AVAILABLE
    if (!connected_)
      return false;
    std::lock_guard<std::mutex> lock(mu_);
    int n = 0;
    char **keys = redisxScanKeys(redis_, pattern.c_str(), &n);
    if (!keys)
      return true;
    for (int i = 0; i < n; i++) {
      int status = 0;
      auto *resp =
          redisxRequest(redis_, "DEL", keys[i], nullptr, nullptr, &status);
      redisxDestroyRESP(resp);
    }
    redisxDestroyKeys(keys, n);
    return true;
#else
    (void)pattern;
    return false;
#endif
  }

  bool exists(const std::string &key) {
#if REDISX_AVAILABLE
    if (!connected_)
      return false;
    std::lock_guard<std::mutex> lock(mu_);
    int status = 0;
    auto *resp =
        redisxRequest(redis_, "EXISTS", key.c_str(), nullptr, nullptr, &status);
    bool result =
        (status == 0 && resp && resp->type == RESP_INT && resp->n > 0);
    redisxDestroyRESP(resp);
    return result;
#else
    (void)key;
    return false;
#endif
  }

  std::string post_key(const std::string &id) { return "post:" + id; }
  std::string user_key(const std::string &id) { return "user:" + id; }
  std::string feed_key(int page, int limit) {
    return "feed:" + std::to_string(page) + ":" + std::to_string(limit);
  }
  std::string user_posts_key(const std::string &user_id, int page, int limit) {
    return "uposts:" + user_id + ":" + std::to_string(page) + ":" +
           std::to_string(limit);
  }
  std::string like_key(const std::string &user_id, const std::string &post_id) {
    return "like:" + user_id + ":" + post_id;
  }
  std::string post_count_key(const std::string &user_id) {
    return "pcount:" + user_id;
  }

  void invalidate_post(const std::string &id) { del(post_key(id)); }

  void invalidate_user(const std::string &id) { del(user_key(id)); }

  void invalidate_feed() { del_pattern("feed:*"); }

  void invalidate_user_posts(const std::string &user_id) {
    del_pattern("uposts:" + user_id + ":*");
  }

  void invalidate_like(const std::string &user_id, const std::string &post_id) {
    del(like_key(user_id, post_id));
  }

  void invalidate_post_count(const std::string &user_id) {
    del(post_count_key(user_id));
  }

private:
  CacheManager() = default;
  ~CacheManager() {
#if REDISX_AVAILABLE
    if (redis_) {
      redisxDisconnect(redis_);
      redisxDestroy(redis_);
    }
#endif
  }

#if REDISX_AVAILABLE
  Redis *redis_ = nullptr;
#endif
  std::atomic<bool> connected_{false};
  std::mutex mu_;
};

} // namespace Crown
