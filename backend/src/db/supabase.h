#ifndef SUPABASE_H
#define SUPABASE_H

#include <functional>
#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for C++ classes
class SupabaseClient;

// C interface for cross-platform compatibility
typedef struct {
  void *client;
} supabase_client_t;

// Initialize Supabase client
int supabase_init(supabase_client_t *client, const char *hostname,
                  const char *key);

// Cleanup
void supabase_cleanup(supabase_client_t *client);

// Database operations
int supabase_insert(supabase_client_t *client, const char *table,
                    const char *json, int upsert);
int supabase_update(supabase_client_t *client, const char *table,
                    const char *json, const char *conditions);
char *supabase_select(supabase_client_t *client, const char *table,
                      const char *columns, const char *conditions);
char *supabase_rpc(supabase_client_t *client, const char *function_name,
                   const char *json_params);

// File operations
int supabase_upload_buffer(supabase_client_t *client, const char *bucket,
                           const char *filename, const char *mime_type,
                           const unsigned char *buffer, unsigned int size);

// Memory management for returned strings
void supabase_free_string(char *str);

#ifdef __cplusplus
}

// C++ interface
namespace Supabase {

class QueryBuilder;

class Client {
public:
  std::string hostname;
  std::string key;
  std::string user_token;
  bool use_auth;

private:
  std::string url_query;
  std::string phone_or_email;
  std::string password;
  std::string login_method;

  unsigned int auth_timeout;
  unsigned long login_time;

  int login_process();

public:
  Client();
  ~Client();

  // Basic setup
  void begin(const std::string &hostname, const std::string &key);
  std::string get_query();
  void url_query_reset();
  void check_last_string();

  // Query builder methods
  QueryBuilder &from(const std::string &table);
  int insert(const std::string &table, const std::string &json,
             bool upsert = false);
  QueryBuilder &select(const std::string &columns);
  QueryBuilder &update(const std::string &table);

  // Upload methods
  int upload(const std::string &bucket, const std::string &filename,
             const std::string &mime_type, const unsigned char *buffer,
             unsigned int size);

  // RPC
  std::string rpc(const std::string &function_name,
                  const std::string &json_params = "");

  // Execute operations
  std::string do_select();
  int do_update(const std::string &json);

  // Get auth token for advanced usage
  std::string get_auth_token() const { return user_token; }
  bool is_authenticated() const { return use_auth && !user_token.empty(); }

  friend class QueryBuilder;
};

class QueryBuilder {
private:
  Client *client;
  std::string &url_query;

public:
  QueryBuilder(Client *client, std::string &query);

  // Comparison operators
  QueryBuilder &eq(const std::string &column, const std::string &value);
  QueryBuilder &gt(const std::string &column, const std::string &value);
  QueryBuilder &gte(const std::string &column, const std::string &value);
  QueryBuilder &lt(const std::string &column, const std::string &value);
  QueryBuilder &lte(const std::string &column, const std::string &value);
  QueryBuilder &neq(const std::string &column, const std::string &value);
  QueryBuilder &in(const std::string &column, const std::string &values);
  QueryBuilder &is(const std::string &column, const std::string &value);
  QueryBuilder &cs(const std::string &column, const std::string &values);
  QueryBuilder &cd(const std::string &column, const std::string &values);
  QueryBuilder &ov(const std::string &column, const std::string &values);
  QueryBuilder &sl(const std::string &column, const std::string &range);
  QueryBuilder &sr(const std::string &column, const std::string &range);
  QueryBuilder &nxr(const std::string &column, const std::string &range);
  QueryBuilder &nxl(const std::string &column, const std::string &range);
  QueryBuilder &adj(const std::string &column, const std::string &range);

  // Ordering and limits
  QueryBuilder &order(const std::string &column,
                      const std::string &direction = "asc",
                      bool nulls_first = true);
  QueryBuilder &limit(unsigned int count);
  QueryBuilder &offset(int count);

  // Execute
  std::string execute();
  int update_execute(const std::string &json);

  friend class Client;
};

} // namespace Supabase

#endif // __cplusplus

#endif // SUPABASE_H
