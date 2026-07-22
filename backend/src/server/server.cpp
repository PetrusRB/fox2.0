#include "server.h"

#include "../cache/cache.h"
#include "../crypto/cryptbaby.h"
#include "../libs/json.hpp"
#include "../middlewares/authMiddle.h"
#include "../middlewares/ratelimit.h"
#include "../utils/logger.h"
#include "../utils/sanitize.h"
#include "http_server.h"
#include <absl/strings/str_format.h>
#include <absl/time/time.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <sodium.h>
#include <thread>

using json = nlohmann::json;
using Cache = Crown::CacheManager;

namespace {
template <typename T>
T json_or(const json &j, const std::string &key, T def = T{}) {
  auto it = j.find(key);
  return (it != j.end() && !it->is_null()) ? it->get<T>() : def;
}

const std::string POST_SELECT =
    "*,author:users!author_id(id,username,handle,display_name,avatar,bio,"
    "followers_count,following_count,posts_count,created_at)";

std::string sanitize_handle(const std::string &name) {
  std::string h = name;
  std::transform(h.begin(), h.end(), h.begin(), ::tolower);
  std::replace(h.begin(), h.end(), ' ', '_');
  return h;
}

void UpsertUser(Supabase::Client &db, const Crown::AuthenticatedUser &u) {
  if (u.id.empty())
    return;

  std::string check =
      db.from("users").select("id").eq("id", u.id).limit(1).execute();
  auto arr = json::parse(check, nullptr, false);

  if (arr.is_array() && !arr.empty()) {
    json row;
    if (!u.name.empty()) {
      row["username"] = u.name;
      row["display_name"] = u.name;
      row["handle"] = sanitize_handle(u.name);
    }
    if (!u.picture.empty()) {
      row["avatar"] = u.picture;
    }
    if (!row.empty()) {
      db.from("users").eq("id", u.id).update_execute(row.dump());
    }
  } else {
    std::string handle = sanitize_handle(u.name);

    std::string exists =
        db.from("users").select("id").eq("handle", handle).limit(1).execute();
    auto exists_arr = json::parse(exists, nullptr, false);
    if (exists_arr.is_array() && !exists_arr.empty()) {
      handle += "_" + Crown::babycrypt.GenerateRandomHex(4);
    }

    json row;
    row["id"] = u.id;
    row["username"] = u.name;
    row["handle"] = handle;
    row["display_name"] = u.name;
    row["avatar"] = u.picture;
    db.insert("users", row.dump(), false);
  }
}

bool ParsePost(const json &row, social::Post *post) {
  if (!row.contains("id") || row["id"].is_null())
    return false;

  post->set_id(row["id"].get<std::string>());
  post->set_title(json_or<std::string>(row, "title"));
  post->set_content(json_or<std::string>(row, "content"));
  post->set_image_url(json_or<std::string>(row, "image_url"));
  post->set_likes_count(json_or<uint32_t>(row, "likes_count"));
  post->set_comments_count(json_or<uint32_t>(row, "comments_count"));
  post->set_created_at(json_or<std::string>(row, "created_at"));
  post->set_is_liked_by_me(json_or<bool>(row, "is_liked_by_me"));

  if (row.contains("author") && !row["author"].is_null()) {
    const json &a = row["author"].is_array() && !row["author"].empty()
                        ? row["author"][0]
                        : row["author"];
    auto *author = post->mutable_author();
    author->set_id(json_or<std::string>(a, "id"));
    author->set_username(json_or<std::string>(a, "username"));
    author->set_handle(json_or<std::string>(a, "handle"));
    author->set_display_name(json_or<std::string>(a, "display_name"));
    author->set_avatar(json_or<std::string>(a, "avatar"));
    author->set_bio(json_or<std::string>(a, "bio"));
    author->set_followers_count(json_or<uint32_t>(a, "followers_count"));
    author->set_following_count(json_or<uint32_t>(a, "following_count"));
    author->set_posts_count(json_or<uint32_t>(a, "posts_count"));
    author->set_created_at(json_or<std::string>(a, "created_at"));
  }

  return true;
}

bool ParseUser(const json &row, social::User *user) {
  if (!row.contains("id") || row["id"].is_null())
    return false;

  user->set_id(row["id"].get<std::string>());
  user->set_username(json_or<std::string>(row, "username"));
  user->set_handle(json_or<std::string>(row, "handle"));
  user->set_display_name(json_or<std::string>(row, "display_name"));
  user->set_avatar(json_or<std::string>(row, "avatar"));
  user->set_bio(json_or<std::string>(row, "bio"));
  user->set_followers_count(json_or<uint32_t>(row, "followers_count"));
  user->set_following_count(json_or<uint32_t>(row, "following_count"));
  user->set_posts_count(json_or<uint32_t>(row, "posts_count"));
  user->set_created_at(json_or<std::string>(row, "created_at"));
  return true;
}

int CountUserPosts(Supabase::Client &db, Crown::CacheManager &cache,
                   const std::string &user_id) {
  std::string cache_key = cache.post_count_key(user_id);
  auto cached = cache.get_json(cache_key);
  if (cached.is_number()) {
    return cache.get_int(cache_key, 0);
  }

  long long total = db.from("posts").eq("author_id", user_id).count();
  int count = total >= 0 ? static_cast<int>(total) : 0;

  cache.set_json(cache_key, count, Crown::CACHE_TTL_POST_COUNT);
  return count;
}

} // namespace

namespace Crown {

grpc::Status CrownServer::CreatePost(grpc::ServerContext *context,
                                     const social::CreatePostRequest *request,
                                     social::Post *response) {
  try {
    auto user = AuthMiddleware::GetUser(context);
    if (!user) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "Not authenticated");
    }

    if (request->title().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Title cannot be empty");
    }
    if (request->title().size() > MAX_TITLE_CHARS) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Title exceeds maximum length of 256 characters");
    }
    if (request->content().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Content cannot be empty");
    }
    if (request->content().size() > MAX_CONTENT_CHARS) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Content exceeds maximum length of 5000 characters");
    }

    std::string clean_title = SanitizeString(request->title());
    std::string clean_content = SanitizeString(request->content());
    std::string clean_image = SanitizeUrl(request->image_url());

    if (!clean_image.empty() && clean_image.size() > MAX_IMAGE_CHARS) {
      return grpc::Status(
          grpc::StatusCode::INVALID_ARGUMENT,
          "Image URL exceeds maximum length of 2048 characters");
    }

    UpsertUser(app_->db(), *user);

    json row;
    std::string idSec = babycrypt.generateSecId();
    row["id"] = idSec;
    row["author_id"] = user->id;
    row["title"] = clean_title;
    row["content"] = clean_content;
    row["image_url"] = clean_image.empty() ? "" : clean_image;

    std::string created = app_->db().insert_return("posts", row.dump(), false);
    std::string post_id;

    if (!created.empty()) {
      auto created_json = json::parse(created, nullptr, false);
      if (created_json.is_object()) {
        post_id = json_or<std::string>(created_json, "id");
      }
    }

    if (post_id.empty()) {
      return grpc::Status(grpc::StatusCode::INTERNAL,
                          "Nao foi possivel obter o ID do post criado, então "
                          "não foi postado o post. Mas não foi falha sua, e "
                          "sim dos nossos servidores :( ");
    }

    std::string result = app_->db()
                             .from("posts")
                             .select(POST_SELECT)
                             .eq("id", post_id)
                             .limit(1)
                             .execute();

    auto posts_arr = json::parse(result, nullptr, false);
    if (posts_arr.is_array() && !posts_arr.empty()) {
      ParsePost(posts_arr[0], response);
    }

    Cache::instance().invalidate_feed();
    Cache::instance().invalidate_post_count(user->id);

    loge(absl::StrFormat("Post criado: [%s] por %s", post_id, user->name));
    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "CreatePost exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::GetPost(grpc::ServerContext *context,
                                  const social::GetPostRequest *request,
                                  social::Post *response) {
  try {
    auto &cache = Cache::instance();
    std::string cache_key = cache.post_key(request->post_id());

    auto cached = cache.get_json(cache_key);
    if (cached.is_object()) {
      ParsePost(cached, response);
      return grpc::Status::OK;
    }

    auto user = AuthMiddleware::GetUser(context);
    std::string user_id = user ? user->id : "";

    json params;
    params["p_user_id"] = user_id;
    params["p_post_id"] = request->post_id();

    std::string result = app_->db().rpc("get_post", params.dump());

    auto parsed = json::parse(result, nullptr, false);
    if (!parsed.is_object() || !parsed.contains("id") ||
        parsed["id"].is_null()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
    }

    ParsePost(parsed, response);
    cache.set_json(cache_key, parsed, Crown::CACHE_TTL_POST);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "GetPost exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::DeletePost(grpc::ServerContext *context,
                                     const social::DeletePostRequest *request,
                                     social::Empty *response) {
  try {
    auto user = AuthMiddleware::GetUser(context);
    if (!user) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "Not authenticated");
    }

    if (request->post_id().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Post ID is required");
    }

    std::string result = app_->db()
                             .from("posts")
                             .select("id,author_id,image_url")
                             .eq("id", request->post_id())
                             .limit(1)
                             .execute();

    auto arr = json::parse(result, nullptr, false);
    if (!arr.is_array() || arr.empty()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
    }

    std::string author_id = json_or<std::string>(arr[0], "author_id");
    if (author_id != user->id) {
      return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                          "Not authorized to delete this post");
    }

    std::string image_url = json_or<std::string>(arr[0], "image_url");

    int del_status =
        app_->db().from("posts").eq("id", request->post_id()).delete_execute();

    if (del_status < 200 || del_status >= 300) {
      return grpc::Status(grpc::StatusCode::INTERNAL, "Falha ao remover post");
    }

    if (!image_url.empty()) {
      std::string hash_result = app_->db()
                                    .from("midia_hashes")
                                    .select("file_id")
                                    .eq("url", image_url)
                                    .limit(1)
                                    .execute();

      auto hash_arr = json::parse(hash_result, nullptr, false);
      if (hash_arr.is_array() && !hash_arr.empty()) {
        std::string file_id = json_or<std::string>(hash_arr[0], "file_id");
        app_->db().from("midia_hashes").eq("url", image_url).delete_execute();

        if (!file_id.empty()) {
          S3DeleteAsync(app_->config().cdnBucket, file_id);
        }
      }
    }

    Cache::instance().invalidate_post(request->post_id());
    Cache::instance().invalidate_feed();
    Cache::instance().invalidate_user_posts(author_id);

    loge(absl::StrFormat("Post removido: [%s]", request->post_id()));

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "DeletePost exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::ListFeed(grpc::ServerContext *context,
                                   const social::ListFeedRequest *request,
                                   social::PostList *response) {
  try {
    auto user = AuthMiddleware::GetUser(context);
    if (!user) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "Not authenticated");
    }

    uint32_t page = request->page() > 0 ? request->page() : 1;
    uint32_t limit = request->limit() > 0 ? request->limit() : 20;
    int offset = static_cast<int>((page - 1) * limit);

    auto &cache = Cache::instance();
    std::string cache_key = cache.feed_key(page, limit);

    auto cached = cache.get_json(cache_key);
    if (cached.is_array()) {
      for (const auto &row : cached) {
        auto *post = response->add_posts();
        ParsePost(row, post);
      }
      return grpc::Status::OK;
    }

    json params;
    params["p_user_id"] = user->id;
    params["p_limit"] = static_cast<int>(limit);
    params["p_offset"] = offset;

    std::string result = app_->db().rpc("get_feed", params.dump());

    auto arr = json::parse(result, nullptr, false);
    if (!arr.is_array()) {
      std::cerr << "ListFeed: rpc get_feed failed, raw="
                << result.substr(0, 500) << "\n";
      return grpc::Status::OK;
    }

    for (const auto &row : arr) {
      auto *post = response->add_posts();
      ParsePost(row, post);
    }

    cache.set_json(cache_key, arr, Crown::CACHE_TTL_FEED);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "ListFeed exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status
CrownServer::ListUserPosts(grpc::ServerContext *context,
                           const social::ListUserPostsRequest *request,
                           social::PostList *response) {
  try {
    auto user = AuthMiddleware::GetUser(context);
    if (!user) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "Not authenticated");
    }

    uint32_t page = request->page() > 0 ? request->page() : 1;
    uint32_t limit = request->limit() > 0 ? request->limit() : 20;
    int offset = static_cast<int>((page - 1) * limit);

    auto &cache = Cache::instance();
    std::string cache_key =
        cache.user_posts_key(request->user_id(), page, limit);

    auto cached = cache.get_json(cache_key);
    if (cached.is_array()) {
      for (const auto &row : cached) {
        auto *post = response->add_posts();
        ParsePost(row, post);
      }
      return grpc::Status::OK;
    }

    json params;
    params["p_user_id"] = user->id;
    params["p_author_id"] = request->user_id();
    params["p_limit"] = static_cast<int>(limit);
    params["p_offset"] = offset;

    std::string result = app_->db().rpc("get_user_posts", params.dump());

    auto arr = json::parse(result, nullptr, false);
    if (!arr.is_array()) {
      std::cerr << "ListUserPosts: rpc get_user_posts failed, raw="
                << result.substr(0, 500) << "\n";
      return grpc::Status::OK;
    }

    for (const auto &row : arr) {
      auto *post = response->add_posts();
      ParsePost(row, post);
    }

    cache.set_json(cache_key, arr, Crown::CACHE_TTL_FEED);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "ListUserPosts exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::GetUserById(grpc::ServerContext *context,
                                      const social::GetUserByIdRequest *request,
                                      social::User *response) {
  try {
    if (request->user_id().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "User ID is required");
    }

    auto &cache = Cache::instance();
    std::string cache_key = cache.user_key(request->user_id());

    auto cached = cache.get_json(cache_key);
    if (cached.is_object()) {
      ParseUser(cached, response);
      return grpc::Status::OK;
    }

    std::string result = app_->db()
                             .from("users")
                             .select("*")
                             .eq("id", request->user_id())
                             .limit(1)
                             .execute();

    auto arr = json::parse(result, nullptr, false);
    if (!arr.is_array() || arr.empty()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
    }

    ParseUser(arr[0], response);
    cache.set_json(cache_key, arr[0], Crown::CACHE_TTL_USER);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "GetProfile exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status
CrownServer::GetUserByHandle(grpc::ServerContext *context,
                             const social::GetUserByHandleRequest *request,
                             social::User *response) {
  try {
    if (request->handle().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Handle is required");
    }

    std::string handle = request->handle();
    if (!handle.empty() && handle[0] == '@') {
      handle = handle.substr(1);
    }

    auto &cache = Cache::instance();
    std::string cache_key = cache.handle_key(handle);

    auto cached = cache.get_json(cache_key);
    if (cached.is_object()) {
      ParseUser(cached, response);
      return grpc::Status::OK;
    }

    std::string result = app_->db()
                             .from("users")
                             .select("*")
                             .eq("handle", handle)
                             .limit(1)
                             .execute();

    auto arr = json::parse(result, nullptr, false);
    if (!arr.is_array() || arr.empty()) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "User not found for this handle");
    }

    ParseUser(arr[0], response);
    cache.set_json(cache_key, arr[0], Crown::CACHE_TTL_USER);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "GetUserByHandle exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::ToggleLike(grpc::ServerContext *context,
                                     const social::ToggleLikeRequest *request,
                                     social::ToggleLikeResult *response) {
  try {
    auto user = AuthMiddleware::GetUser(context);
    if (!user) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "Not authenticated");
    }

    if (request->post_id().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Post ID is required");
    }

    json params;
    params["p_user_id"] = user->id;
    params["p_post_id"] = request->post_id();

    std::string result = app_->db().rpc("toggle_like", params.dump());

    auto res = json::parse(result, nullptr, false);
    if (!res.is_object()) {
      std::cerr << "ToggleLike: rpc toggle_like failed, raw="
                << result.substr(0, 500) << "\n";
      return grpc::Status(grpc::StatusCode::INTERNAL, "Toggle like failed");
    }

    response->set_is_liked(json_or<bool>(res, "is_liked"));
    response->set_updated_likes_count(json_or<int>(res, "likes_count"));

    Cache::instance().invalidate_post(request->post_id());
    Cache::instance().invalidate_like(user->id, request->post_id());
    Cache::instance().invalidate_feed();

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "ToggleLike exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::Login(grpc::ServerContext *context,
                                const social::LoginRequest *request,
                                social::LoginResult *response) {
  try {
    if (request->authorization_code().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Authorization code is required");
    }
    if (request->redirect_uri().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Redirect URI is required");
    }

    auto [status, tokens] = AuthMiddleware::ExchangeCodeForToken(
        request->authorization_code(), request->redirect_uri());

    if (!status.ok()) {
      return status;
    }

    auto [user_status, user] = AuthMiddleware::ValidateIdToken(tokens.first);
    if (!user_status.ok()) {
      return user_status;
    }

    response->set_id_token(tokens.first);
    response->set_refresh_token(tokens.second);
    response->set_expires_in(ID_ACCESS_TOKEN_EXPIRES_SECONDS);

    UpsertUser(app_->db(), user);

    social::User proto_user;
    proto_user.set_id(user.id);
    proto_user.set_username(user.name);
    proto_user.set_display_name(user.name);
    proto_user.set_avatar(user.picture);
    proto_user.set_posts_count(
        CountUserPosts(app_->db(), Cache::instance(), user.id));

    *response->mutable_user() = proto_user;

    Cache::instance().invalidate_user(user.id);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "Login exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

grpc::Status CrownServer::RefreshAccessToken(
    grpc::ServerContext *context,
    const social::RefreshAccessTokenRequest *request,
    social::LoginResult *response) {
  try {
    if (request->refresh_token().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "Refresh token is required");
    }

    auto [status, new_id_token] =
        AuthMiddleware::RefreshAccessToken(request->refresh_token());

    if (!status.ok()) {
      return status;
    }

    response->set_id_token(new_id_token);
    response->set_expires_in(ID_REFRESH_TOKEN_EXPIRES_SECONDS);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "RefreshAccessToken exception: " << e.what() << "\n";
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  }
}

bool CrownServer::init(uint16_t port, AppContext &app) {
  app_ = &app;

  if (sodium_init() < 0) {
    throw std::runtime_error("Failed to initialize libsodium");
  }

  if (!app.config().redisUrl.empty()) {
    Cache::instance().init(app.config().redisUrl);
    loge("Cache Redis conectado!");
  } else {
    loge("Cache Redis desabilitado (sem url configurada)");
  }

  std::string server_addr = absl::StrFormat("0.0.0.0:%d", port);

  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(static_cast<social::PostService::Service *>(this));
  builder.RegisterService(static_cast<social::AuthService::Service *>(this));
  builder.RegisterService(static_cast<social::UserService::Service *>(this));
  builder.RegisterService(
      static_cast<social::InteractionService::Service *>(this));

  std::vector<
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
      creators;
  creators.push_back(std::make_unique<AuthInterceptorFactory>());

  std::vector<Crown::RouteConfig> routes = {
      {"/social.PostService/CreatePost", 5, std::chrono::seconds(60)},
      {"/social.InteractionService/ToggleLike", 10, std::chrono::seconds(60)},
      {"/social.InteractionService/AddComment", 5, std::chrono::seconds(60)},
      {"/social.SocialService/ToggleFollow", 5, std::chrono::seconds(60)},
  };
  creators.push_back(
      std::make_unique<Crown::RatelimitInterceptFactory>(routes));

  builder.experimental().SetInterceptorCreators(std::move(creators));

  server_ = builder.BuildAndStart();
  if (!server_) {
    return false;
  }

  loge(absl::StrFormat("Servidor iniciado em %s", server_addr));
  server_->Wait();
  return true;
}

void CrownServer::shutdown() {
  if (server_) {
    server_->Shutdown();
  }
}

} // namespace Crown
