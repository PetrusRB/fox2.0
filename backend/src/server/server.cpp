#include "server.h"

#include "../cache/cache.h"
#include "../crypto/cryptbaby.h"
#include "../libs/json.hpp"
#include "../middlewares/authMiddle.h"
#include "../middlewares/ratelimit.h"
#include "../utils/sanitize.h"
#include <absl/strings/str_format.h>
#include <absl/time/time.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <sodium.h>

using json = nlohmann::json;
using Cache = Crown::CacheManager;

namespace {
Crown::CryptBaby babycrypt;
template <typename T>
T json_or(const json &j, const std::string &key, T def = T{}) {
  auto it = j.find(key);
  return (it != j.end() && !it->is_null()) ? it->get<T>() : def;
}

const std::string POST_SELECT =
    "*,author:users!author_id(id,username,handle,display_name,avatar,bio,"
    "followers_count,following_count,posts_count,created_at)";

void UpsertUser(Supabase::Client &db, const Crown::AuthenticatedUser &u) {
  std::string handle = u.name;
  std::transform(handle.begin(), handle.end(), handle.begin(), ::tolower);
  std::replace(handle.begin(), handle.end(), ' ', '_');

  json row;
  row["username"] = u.name;
  row["handle"] = handle;
  row["display_name"] = u.name;
  row["avatar"] = u.picture;

  std::string check =
      db.from("users").select("id").eq("id", u.id).limit(1).execute();
  auto arr = json::parse(check, nullptr, false);

  if (arr.is_array() && !arr.empty()) {
    db.from("users").eq("id", u.id).update_execute(row.dump());
  } else {
    row["id"] = u.id;
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

int CountUserPosts(Supabase::Client &db, const std::string &user_id) {
  std::string result = db.from("posts")
                           .select("id")
                           .eq("author_id", user_id)
                           .limit(10000)
                           .execute();
  auto arr = json::parse(result, nullptr, false);
  return arr.is_array() ? static_cast<int>(arr.size()) : 0;
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
      std::cerr << "insert_return failed, falling back to insert + select\n";
      int status = app_->db().insert("posts", row.dump(), false);
      if (status < 200 || status >= 300) {
        return grpc::Status(grpc::StatusCode::INTERNAL,
                            "Falha ao criar post no banco de dados");
      }

      std::string result = app_->db()
                               .from("posts")
                               .select("id")
                               .eq("author_id", user->id)
                               .order("created_at", "desc", false)
                               .limit(1)
                               .execute();

      auto arr = json::parse(result, nullptr, false);
      if (arr.is_array() && !arr.empty()) {
        post_id = json_or<std::string>(arr[0], "id");
      }
    }

    if (post_id.empty()) {
      return grpc::Status(grpc::StatusCode::INTERNAL,
                          "Nao foi possivel obter o ID do post criado");
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

    std::cout << "Post criado: [" << post_id << "] por " << user->name << "\n";
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
                             .select("id,author_id")
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

    int del_status =
        app_->db().from("posts").eq("id", request->post_id()).delete_execute();

    if (del_status < 200 || del_status >= 300) {
      return grpc::Status(grpc::StatusCode::INTERNAL, "Falha ao remover post");
    }

    Cache::instance().invalidate_post(request->post_id());
    Cache::instance().invalidate_feed();
    Cache::instance().invalidate_user_posts(author_id);

    std::cout << "Post removido: [" << request->post_id() << "]\n";
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

grpc::Status CrownServer::GetProfile(grpc::ServerContext *context,
                                     const social::GetProfileRequest *request,
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
      response->set_id(json_or<std::string>(cached, "id"));
      response->set_username(json_or<std::string>(cached, "username"));
      response->set_handle(json_or<std::string>(cached, "handle"));
      response->set_display_name(json_or<std::string>(cached, "display_name"));
      response->set_avatar(json_or<std::string>(cached, "avatar"));
      response->set_bio(json_or<std::string>(cached, "bio"));
      response->set_followers_count(
          json_or<uint32_t>(cached, "followers_count"));
      response->set_following_count(
          json_or<uint32_t>(cached, "following_count"));
      response->set_posts_count(json_or<uint32_t>(cached, "posts_count"));
      response->set_created_at(json_or<std::string>(cached, "created_at"));
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

    const auto &u = arr[0];
    response->set_id(json_or<std::string>(u, "id"));
    response->set_username(json_or<std::string>(u, "username"));
    response->set_handle(json_or<std::string>(u, "handle"));
    response->set_display_name(json_or<std::string>(u, "display_name"));
    response->set_avatar(json_or<std::string>(u, "avatar"));
    response->set_bio(json_or<std::string>(u, "bio"));
    response->set_followers_count(json_or<uint32_t>(u, "followers_count"));
    response->set_following_count(json_or<uint32_t>(u, "following_count"));
    response->set_posts_count(json_or<uint32_t>(u, "posts_count"));
    response->set_created_at(json_or<std::string>(u, "created_at"));

    cache.set_json(cache_key, u, Crown::CACHE_TTL_USER);

    return grpc::Status::OK;
  } catch (const std::exception &e) {
    std::cerr << "GetProfile exception: " << e.what() << "\n";
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
    proto_user.set_posts_count(CountUserPosts(app_->db(), user.id));

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
    std::cout << "Cache Redis conectado\n";
  } else {
    std::cout << "Cache Redis desabilitado (sem URL configurada)\n";
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

  std::cout << "Servidor iniciado em " << server_addr << "\n";
  server_->Wait();
  return true;
}

void CrownServer::shutdown() {
  if (server_) {
    server_->Shutdown();
  }
}

} // namespace Crown
