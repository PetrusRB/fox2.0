#include "supabase.h"
#include "../libs/http.h"
#include "../libs/json.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "Supabase"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#include <iostream>
#define LOGD(...)                                                              \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
    printf("\n");                                                              \
  } while (0)
#define LOGE(...)                                                              \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#endif

using json = nlohmann::json;

namespace Supabase {

// Helper function to get current time in milliseconds
unsigned long current_time_ms() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

// Helper function to extract hostname without protocol
std::string extract_hostname(const std::string &url) {
  size_t start = url.find("://");
  if (start != std::string::npos) {
    start += 3;
    size_t end = url.find("/", start);
    if (end != std::string::npos) {
      return url.substr(start, end - start);
    }
    return url.substr(start);
  }
  return url;
}

Client::Client() : use_auth(false), auth_timeout(0), login_time(0) {}

Client::~Client() {}

void Client::begin(const std::string &hostname_a, const std::string &key_a) {
  hostname = hostname_a;
  key = key_a;
}

int Client::login_process() {
  try {
// Determine if we should use HTTPS
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    // Use HTTPS when OpenSSL is available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
    LOGD("Using HTTPS with SSL verification for login");
#else
    // Fall back to HTTP when OpenSSL is not available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
    LOGD("Using HTTP (no SSL support) for login");
#endif

    // Set headers
    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    // Prepare login data
    json login_data;
    login_data[login_method] = phone_or_email;
    login_data["password"] = password;

    std::string body = login_data.dump();

    LOGD("Beginning to login...");

    auto res = cli.Post("/auth/v1/token?grant_type=password", headers, body,
                        "application/json");

    if (res && res->status >= 200 && res->status < 300) {
      try {
        json response = json::parse(res->body);

        if (response.contains("access_token") &&
            !response["access_token"].is_null() &&
            response["access_token"].is_string() &&
            !response["access_token"].get<std::string>().empty()) {

          user_token = response["access_token"].get<std::string>();
          auth_timeout = response["expires_in"].get<int>() * 1000;
          login_time = current_time_ms();

          LOGD("Login Success");
          return res->status;
        } else {
          LOGE("Login Failed: Invalid access token in response");
          return -1;
        }
      } catch (const json::exception &e) {
        LOGE("Login Failed: JSON parse error - %s", e.what());
        return -1;
      }
    } else {
      int status = res ? res->status : -100;
      LOGE("Login Failed: HTTP %d", status);
      return status;
    }
  } catch (const std::exception &e) {
    LOGE("Login Failed: Exception - %s", e.what());
    return -100;
  }
}

QueryBuilder Client::from(const std::string &table) {
  return QueryBuilder(this, table + "?");
}

int Client::insert(const std::string &table, const std::string &json_data,
                   bool upsert) {
  try {
// Determine if we should use HTTPS
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    // Use HTTPS when OpenSSL is available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);

    // Temporarily disable SSL verification for testing
    cli.enable_server_certificate_verification(false);
    LOGD("Using HTTPS with SSL verification DISABLED for testing");

    // Set connection timeout
    cli.set_connection_timeout(10, 0); // 10 seconds
    cli.set_read_timeout(30, 0);       // 30 seconds
    cli.set_write_timeout(30, 0);      // 30 seconds

#else
    // Fall back to HTTP when OpenSSL is not available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
    LOGD("Using HTTP (no SSL support) for insert");
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    std::string prefer_header = "return=minimal";
    if (upsert) {
      prefer_header += ",resolution=merge-duplicates";
    }
    headers.emplace("Prefer", prefer_header);

    if (use_auth && !user_token.empty()) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        LOGD("Token expired, refreshing...");
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
      LOGD("Using authentication with user token");
    } else {
      LOGD("Using anonymous access with anon key");
      // For anonymous access, use the anon API key in Authorization header
      headers.emplace("Authorization", "Bearer " + key);
    }

    LOGD("Making POST request to: /rest/v1/%s", table.c_str());
    LOGD("Request body: %s", json_data.c_str());

    auto res =
        cli.Post("/rest/v1/" + table, headers, json_data, "application/json");

    if (res) {
      LOGD("Response status: %d", res->status);
      if (res->status >= 400) {
        LOGE("Error response body: %s", res->body.c_str());
      }
      return res->status;
    } else {
      LOGE("No response received - connection failed");
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
      LOGE("Possible SSL/network connectivity issue");
#endif
      return -100;
    }

  } catch (const std::exception &e) {
    LOGE("Insert failed: %s", e.what());
    return -100;
  }
}

std::string Client::insert_return(const std::string &table,
                                  const std::string &json_data, bool upsert) {
  try {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(false);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(30, 0);
    cli.set_write_timeout(30, 0);
#else
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    if (upsert) {
      headers.emplace("Prefer",
                      "return=representation,resolution=merge-duplicates");
    } else {
      headers.emplace("Prefer", "return=representation");
    }

    if (use_auth && !user_token.empty()) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        LOGD("Token expired, refreshing...");
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
      LOGD("insert_return: using user token auth");
    } else {
      headers.emplace("Authorization", "Bearer " + key);
      LOGD("insert_return: using anon key auth");
    }

    LOGD("insert_return: POST /rest/v1/%s", table.c_str());
    LOGD("insert_return: body: %s", json_data.c_str());

    auto res =
        cli.Post("/rest/v1/" + table, headers, json_data, "application/json");

    if (!res) {
      LOGE("insert_return: no response from server");
      return "";
    }

    LOGD("insert_return: status=%d", res->status);

    if (res->status >= 400) {
      LOGE("insert_return: error %d: %s", res->status, res->body.c_str());
      return "";
    }

    if (res->status >= 200 && res->status < 300) {
      auto parsed = json::parse(res->body, nullptr, false);
      if (parsed.is_array() && !parsed.empty()) {
        LOGD("insert_return: success, got %zu items", parsed.size());
        return parsed[0].dump();
      }
      LOGE("insert_return: response not array or empty. body=%.200s",
           res->body.c_str());
      return "";
    }

    return "";
  } catch (const std::exception &e) {
    LOGE("insert_return: exception: %s", e.what());
    return "";
  }
}

int Client::upload(const std::string &bucket, const std::string &filename,
                   const std::string &mime_type, const unsigned char *buffer,
                   unsigned int size) {
  try {
    std::string host = extract_hostname(hostname);
    httplib::Client cli(host);

// Enable SSL verification for production
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    cli.enable_server_certificate_verification(true);
#endif

    httplib::Headers headers = {{"apikey", key}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    }

    // Simple file upload as body
    std::string file_content(reinterpret_cast<const char *>(buffer), size);
    auto res = cli.Post("/storage/v1/object/" + bucket + "/" + filename,
                        headers, file_content, mime_type);
    return res ? res->status : -100;

  } catch (const std::exception &e) {
    LOGE("Upload failed: %s", e.what());
    return -100;
  }
}

std::string Client::do_select(const std::string &url) {
  try {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
#else
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    }

    auto res = cli.Get("/rest/v1/" + url, headers);

    if (!res) {
      LOGE("do_select: no response from server");
      return "";
    }

    LOGD("do_select: status=%d", res->status);

    if (res->status >= 400) {
      LOGE("do_select: error %d: %s", res->status, res->body.c_str());
      return "";
    }

    if (res->status >= 200 && res->status < 300) {
      return res->body;
    }

    return "";

  } catch (const std::exception &e) {
    LOGE("Select failed: %s", e.what());
    return "";
  }
}

int Client::do_update(const std::string &url, const std::string &json_data) {
  try {
// Determine if we should use HTTPS
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    // Use HTTPS when OpenSSL is available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
    LOGD("Using HTTPS with SSL verification for update");
#else
    // Fall back to HTTP when OpenSSL is not available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
    LOGD("Using HTTP (no SSL support) for update");
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    }

    auto res =
        cli.Patch("/rest/v1/" + url, headers, json_data, "application/json");

    return res ? res->status : -100;

  } catch (const std::exception &e) {
    LOGE("Update failed: %s", e.what());
    return -100;
  }
}
long long Client::do_count(const std::string &url) {
  try {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
#else
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
#endif
    httplib::Headers headers = {{"apikey", key}, {"Prefer", "count=exact"}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    } else {
      headers.emplace("Authorization", "Bearer " + key);
    }

    auto res = cli.Head("/rest/v1/" + url, headers);

    if (!res) {
      LOGE("do_count: sem resposta vindo do servidor");
      return -1;
    }

    if (res->status >= 400) {
      LOGE("do_count: error %d", res->status);
      return -1;
    }

    auto it = res->headers.find("Content-Range");
    if (it == res->headers.end()) {
      LOGE("do_count: A header Content-Range esta faltando");
      return -1;
    }

    const std::string &cr = it->second;
    size_t slash = cr.find("/");
    if (slash == std::string::npos || slash + 1 >= cr.size()) {
      return -1;
    }

    std::string total_str = cr.substr(slash + 1);
    if (total_str == "*") {
      return -1;
    }

    try {
      return std::stoll(total_str);
    } catch (...) {
      return -1;
    }

  } catch (const std::exception &e) {
    LOGE("Falhou ao tentar CONTAR: %s", e.what());
    return -1;
  }
}
int Client::do_delete(const std::string &url) {
  try {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
#else
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"},
                                {"Prefer", "return=minimal"}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    }

    auto res = cli.Delete("/rest/v1/" + url, headers);

    return res ? res->status : -100;

  } catch (const std::exception &e) {
    LOGE("Delete failed: %s", e.what());
    return -100;
  }
}

std::string Client::rpc(const std::string &function_name,
                        const std::string &json_params) {
  try {
// Determine if we should use HTTPS
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    // Use HTTPS when OpenSSL is available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "https://" + hostname;
    httplib::Client cli(scheme);
    cli.enable_server_certificate_verification(true);
    LOGD("Using HTTPS with SSL verification for RPC");
#else
    // Fall back to HTTP when OpenSSL is not available
    std::string scheme = hostname.find("://") != std::string::npos
                             ? hostname
                             : "http://" + hostname;
    httplib::Client cli(scheme);
    LOGD("Using HTTP (no SSL support) for RPC");
#endif

    httplib::Headers headers = {{"apikey", key},
                                {"Content-Type", "application/json"}};

    if (use_auth) {
      unsigned long t_now = current_time_ms();
      if (t_now - login_time >= auth_timeout) {
        login_process();
      }
      headers.emplace("Authorization", "Bearer " + user_token);
    }

    auto res = cli.Post("/rest/v1/rpc/" + function_name, headers, json_params,
                        "application/json");

    if (res && res->status >= 200 && res->status < 300) {
      return res->body;
    }

    return std::to_string(res ? res->status : -100);

  } catch (const std::exception &e) {
    LOGE("RPC failed: %s", e.what());
    return "-100";
  }
}

// QueryBuilder implementation
QueryBuilder::QueryBuilder(Client *client, std::string query)
    : client(client), url_query(std::move(query)) {}

void QueryBuilder::check_last_string() {
  if (!url_query.empty() && url_query.back() != '?') {
    url_query += "&";
  }
}

QueryBuilder &QueryBuilder::eq(const std::string &column,
                               const std::string &value) {
  check_last_string();
  url_query += column + "=eq." + value;
  return *this;
}

QueryBuilder &QueryBuilder::gt(const std::string &column,
                               const std::string &value) {
  check_last_string();
  url_query += column + "=gt." + value;
  return *this;
}

QueryBuilder &QueryBuilder::gte(const std::string &column,
                                const std::string &value) {
  check_last_string();
  url_query += column + "=gte." + value;
  return *this;
}

QueryBuilder &QueryBuilder::lt(const std::string &column,
                               const std::string &value) {
  check_last_string();
  url_query += column + "=lt." + value;
  return *this;
}

QueryBuilder &QueryBuilder::lte(const std::string &column,
                                const std::string &value) {
  check_last_string();
  url_query += column + "=lte." + value;
  return *this;
}

QueryBuilder &QueryBuilder::neq(const std::string &column,
                                const std::string &value) {
  check_last_string();
  url_query += column + "=neq." + value;
  return *this;
}

QueryBuilder &QueryBuilder::in(const std::string &column,
                               const std::string &values) {
  check_last_string();
  url_query += column + "=in.(" + values + ")";
  return *this;
}

QueryBuilder &QueryBuilder::is(const std::string &column,
                               const std::string &value) {
  check_last_string();
  url_query += column + "=is." + value;
  return *this;
}

QueryBuilder &QueryBuilder::cs(const std::string &column,
                               const std::string &values) {
  check_last_string();
  url_query += column + "=cs.{" + values + "}";
  return *this;
}

QueryBuilder &QueryBuilder::cd(const std::string &column,
                               const std::string &values) {
  check_last_string();
  url_query += column + "=cd.{" + values + "}";
  return *this;
}

QueryBuilder &QueryBuilder::ov(const std::string &column,
                               const std::string &values) {
  check_last_string();
  url_query += column + "=ov.{" + values + "}";
  return *this;
}

QueryBuilder &QueryBuilder::sl(const std::string &column,
                               const std::string &range) {
  check_last_string();
  url_query += column + "=sl.(" + range + ")";
  return *this;
}

QueryBuilder &QueryBuilder::sr(const std::string &column,
                               const std::string &range) {
  check_last_string();
  url_query += column + "=sr.(" + range + ")";
  return *this;
}

QueryBuilder &QueryBuilder::nxr(const std::string &column,
                                const std::string &range) {
  check_last_string();
  url_query += column + "=nxr.(" + range + ")";
  return *this;
}

QueryBuilder &QueryBuilder::nxl(const std::string &column,
                                const std::string &range) {
  check_last_string();
  url_query += column + "=nxl.(" + range + ")";
  return *this;
}

QueryBuilder &QueryBuilder::adj(const std::string &column,
                                const std::string &range) {
  check_last_string();
  url_query += column + "=adj.(" + range + ")";
  return *this;
}

QueryBuilder &QueryBuilder::order(const std::string &column,
                                  const std::string &direction,
                                  bool nulls_first) {
  check_last_string();
  std::string nulls = nulls_first ? "nullsfirst" : "nullslast";
  url_query += "order=" + column + "." + direction + "." + nulls;
  return *this;
}

QueryBuilder &QueryBuilder::limit(unsigned int count) {
  check_last_string();
  url_query += "limit=" + std::to_string(count);
  return *this;
}

QueryBuilder &QueryBuilder::offset(int count) {
  check_last_string();
  url_query += "offset=" + std::to_string(count);
  return *this;
}

QueryBuilder &QueryBuilder::select(const std::string &columns) {
  check_last_string();
  url_query += "select=" + columns;
  return *this;
}

std::string QueryBuilder::execute() { return client->do_select(url_query); }

int QueryBuilder::update_execute(const std::string &json_data) {
  return client->do_update(url_query, json_data);
}

int QueryBuilder::delete_execute() { return client->do_delete(url_query); }

long long QueryBuilder::count() { return client->do_count(url_query); }

} // namespace Supabase

// C interface implementation
extern "C" {

int supabase_init(supabase_client_t *client, const char *hostname,
                  const char *key) {
  try {
    client->client = new Supabase::Client();
    static_cast<Supabase::Client *>(client->client)->begin(hostname, key);
    return 0;
  } catch (const std::exception &e) {
    LOGE("supabase_init failed: %s", e.what());
    return -1;
  }
}

void supabase_cleanup(supabase_client_t *client) {
  if (client && client->client) {
    delete static_cast<Supabase::Client *>(client->client);
    client->client = nullptr;
  }
}

int supabase_insert(supabase_client_t *client, const char *table,
                    const char *json, int upsert) {
  if (!client || !client->client)
    return -1;
  return static_cast<Supabase::Client *>(client->client)
      ->insert(table, json, upsert != 0);
}

int supabase_update(supabase_client_t *client, const char *table,
                    const char *json, const char *conditions) {
  if (!client || !client->client)
    return -1;

  auto *cpp_client = static_cast<Supabase::Client *>(client->client);
  auto builder = cpp_client->from(table);

  // Parse simple conditions (this is a basic implementation)
  // For complex conditions, users should use the C++ interface
  if (conditions && strlen(conditions) > 0) {
    // Simple eq condition parsing: "column=value"
    std::string cond(conditions);
    size_t eq_pos = cond.find('=');
    if (eq_pos != std::string::npos) {
      std::string column = cond.substr(0, eq_pos);
      std::string value = cond.substr(eq_pos + 1);
      builder.eq(column, value);
    }
  }

  return builder.update_execute(json);
}

char *supabase_select(supabase_client_t *client, const char *table,
                      const char *columns, const char *conditions) {
  if (!client || !client->client)
    return nullptr;

  auto *cpp_client = static_cast<Supabase::Client *>(client->client);

  // Build query manually for C interface
  std::string query = std::string(table) + "?";
  if (columns) {
    query += "select=" + std::string(columns);
  } else {
    query += "select=*";
  }

  // Parse simple conditions
  if (conditions && strlen(conditions) > 0) {
    query += "&";
    std::string cond(conditions);
    size_t eq_pos = cond.find('=');
    if (eq_pos != std::string::npos) {
      std::string column = cond.substr(0, eq_pos);
      std::string value = cond.substr(eq_pos + 1);
      query += column + "=eq." + value;
    }
  }

  // Execute query manually
  std::string result;
  try {
    std::string host = Supabase::extract_hostname(cpp_client->hostname);

    httplib::Client cli(host);
    httplib::Headers headers = {{"apikey", cpp_client->key},
                                {"Content-Type", "application/json"}};

    if (cpp_client->use_auth && !cpp_client->user_token.empty()) {
      headers.emplace("Authorization", "Bearer " + cpp_client->user_token);
    } else {
      // For anonymous access, use the anon API key in Authorization header
      headers.emplace("Authorization", "Bearer " + cpp_client->key);
    }

    auto res = cli.Get("/rest/v1/" + query, headers);
    if (res && res->status >= 200 && res->status < 300) {
      result = res->body;
    }
  } catch (...) {
    return nullptr;
  }

  if (result.empty())
    return nullptr;

  char *c_result = static_cast<char *>(malloc(result.length() + 1));
  if (c_result) {
    strcpy(c_result, result.c_str());
  }
  return c_result;
}

char *supabase_rpc(supabase_client_t *client, const char *function_name,
                   const char *json_params) {
  if (!client || !client->client)
    return nullptr;

  std::string result = static_cast<Supabase::Client *>(client->client)
                           ->rpc(function_name, json_params ? json_params : "");

  if (result.empty())
    return nullptr;

  char *c_result = static_cast<char *>(malloc(result.length() + 1));
  if (c_result) {
    strcpy(c_result, result.c_str());
  }
  return c_result;
}

int supabase_upload_buffer(supabase_client_t *client, const char *bucket,
                           const char *filename, const char *mime_type,
                           const unsigned char *buffer, unsigned int size) {
  if (!client || !client->client)
    return -1;
  return static_cast<Supabase::Client *>(client->client)
      ->upload(bucket, filename, mime_type, buffer, size);
}

void supabase_free_string(char *str) {
  if (str) {
    free(str);
  }
}

} // extern "C"
