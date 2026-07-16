#include "server.h"

#include "../middlewares/authMiddle.h"
#include "../middlewares/ratelimit.h"
#include "../utils/sanitize.h"
#include <absl/strings/str_format.h>
#include <absl/time/time.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <format>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>

namespace Crown {

grpc::Status CrownServer::CreatePost(grpc::ServerContext *context,
                                     const social::CreatePostRequest *request,
                                     social::Post *response) {

  auto user = AuthMiddleware::GetUser(context);
  if (!user) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Not authenticated");
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
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Image URL exceeds maximum length of 2048 characters");
  }

  social::Post post;
  post.set_id(std::to_string(next_id_++));
  post.set_title(clean_title);
  post.set_content(clean_content);
  if (!clean_image.empty()) {
    post.set_image_url(clean_image);
  }
  auto now = std::chrono::system_clock::now();
  std::string time = absl::FormatTime(
      "%Y-%m-%dT%H:%M:%SZ", absl::FromChrono(now), absl::UTCTimeZone());

  post.set_likes_count(0);
  post.set_created_at(time);
  post.set_comments_count(0);
  post.set_is_liked_by_me(false);

  social::User source;
  source.set_id(user->id);
  source.set_username(user->name);
  source.set_display_name(user->name);
  source.set_avatar(user->picture);

  social::User *stored = findOrCreateUser(source);
  stored->set_posts_count(stored->posts_count() + 1);

  social::User *author = post.mutable_author();
  *author = *stored;

  posts_.push_back(post);
  post_index_[post.id()] = posts_.size() - 1;

  *response = post;

  std::cout << "Post criado: [" << post.id() << "] por " << user->name << "\n";
  return grpc::Status::OK;
}

grpc::Status CrownServer::GetPost(grpc::ServerContext *context,
                                  const social::GetPostRequest *request,
                                  social::Post *response) {

  auto it = post_index_.find(request->post_id());
  if (it == post_index_.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
  }

  *response = posts_[it->second];

  auto user = AuthMiddleware::GetUser(context);
  if (user) {
    auto userLikes = likes_.find(user->id);
    if (userLikes != likes_.end()) {
      response->set_is_liked_by_me(userLikes->second.count(request->post_id()) >
                                   0);
    }
  }

  return grpc::Status::OK;
}

grpc::Status CrownServer::DeletePost(grpc::ServerContext *context,
                                     const social::DeletePostRequest *request,
                                     social::Empty *response) {

  auto user = AuthMiddleware::GetUser(context);
  if (!user) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Not authenticated");
  }

  if (request->post_id().empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Post ID is required");
  }

  auto it = post_index_.find(request->post_id());
  if (it == post_index_.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
  }

  auto &post = posts_[it->second];
  if (post.author().id() != user->id) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "Not authorized to delete this post");
  }

  std::cout << "Post removido: [" << post.id() << "]\n";
  posts_.erase(posts_.begin() + it->second);
  post_index_.erase(it);

  // Rebuild index after erase
  post_index_.clear();
  for (size_t i = 0; i < posts_.size(); ++i) {
    post_index_[posts_[i].id()] = i;
  }

  return grpc::Status::OK;
}

grpc::Status CrownServer::ListFeed(grpc::ServerContext *context,
                                   const social::ListFeedRequest *request,
                                   social::PostList *response) {

  auto user = AuthMiddleware::GetUser(context);
  if (!user) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Not authenticated");
  }
  uint32_t page = request->page() > 0 ? request->page() : 1;
  uint32_t limit = request->limit() > 0 ? request->limit() : 20;

  if (posts_.empty()) {
    return grpc::Status::OK;
  }

  int total = static_cast<int>(posts_.size());
  int end = total - static_cast<int>((page - 1) * limit);
  int begin = std::max(0, end - static_cast<int>(limit));

  std::string userId = user ? user->id : "";

  for (int i = end - 1; i >= begin; --i) {
    auto *post = response->add_posts();
    *post = posts_[i];
    if (!userId.empty()) {
      auto it = likes_.find(userId);
      if (it != likes_.end()) {
        post->set_is_liked_by_me(it->second.count(posts_[i].id()) > 0);
      }
    }
  }

  return grpc::Status::OK;
}

grpc::Status
CrownServer::ListUserPosts(grpc::ServerContext *context,
                           const social::ListUserPostsRequest *request,
                           social::PostList *response) {

  auto user = AuthMiddleware::GetUser(context);
  if (!user) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Not authenticated");
  }
  uint32_t page = request->page() > 0 ? request->page() : 1;
  uint32_t limit = request->limit() > 0 ? request->limit() : 20;

  int total = static_cast<int>(posts_.size());
  int end = total - static_cast<int>((page - 1) * limit);
  int begin = std::max(0, end - static_cast<int>(limit));

  std::string userId = user ? user->id : "";

  for (int i = end - 1; i >= begin; --i) {
    if (posts_[i].author().id() != request->user_id()) {
      continue;
    }
    auto *post = response->add_posts();
    *post = posts_[i];
    if (!userId.empty()) {
      auto it = likes_.find(userId);
      if (it != likes_.end()) {
        post->set_is_liked_by_me(it->second.count(posts_[i].id()) > 0);
      }
    }
  }

  return grpc::Status::OK;
}

social::User *CrownServer::findOrCreateUser(const social::User &source) {
  auto it = user_index_.find(source.id());
  if (it != user_index_.end()) {
    auto &u = users_[it->second];
    if (!source.username().empty())
      u.set_username(source.username());
    if (!source.handle().empty())
      u.set_handle(source.handle());
    if (!source.display_name().empty())
      u.set_display_name(source.display_name());
    if (!source.avatar().empty())
      u.set_avatar(source.avatar());
    if (!source.bio().empty())
      u.set_bio(source.bio());
    if (source.followers_count() > 0)
      u.set_followers_count(source.followers_count());
    if (source.following_count() > 0)
      u.set_following_count(source.following_count());
    if (source.posts_count() > 0)
      u.set_posts_count(source.posts_count());
    if (!source.created_at().empty())
      u.set_created_at(source.created_at());
    if (source.is_following_me())
      u.set_is_following_me(source.is_following_me());
    if (source.is_followed_by_me())
      u.set_is_followed_by_me(source.is_followed_by_me());
    return &u;
  }

  auto &u = users_.emplace_back();
  user_index_[source.id()] = users_.size() - 1;
  u = source;

  if (u.handle().empty() && !u.username().empty()) {
    std::string handle = u.username();
    std::transform(handle.begin(), handle.end(), handle.begin(), ::tolower);
    std::replace(handle.begin(), handle.end(), ' ', '_');
    u.set_handle(handle);
  }

  return &u;
}

grpc::Status CrownServer::GetProfile(grpc::ServerContext *context,
                                     const social::GetProfileRequest *request,
                                     social::User *response) {
  if (request->user_id().empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "User ID is required");
  }

  auto it = user_index_.find(request->user_id());
  if (it != user_index_.end()) {
    *response = users_[it->second];
    return grpc::Status::OK;
  }

  return grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found");
}

grpc::Status CrownServer::ToggleLike(grpc::ServerContext *context,
                                     const social::ToggleLikeRequest *request,
                                     social::ToggleLikeResult *response) {

  auto user = AuthMiddleware::GetUser(context);
  if (!user) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Not authenticated");
  }

  if (request->post_id().empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "Post ID is required");
  }

  auto it = post_index_.find(request->post_id());
  if (it == post_index_.end()) {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post not found");
  }

  auto &post = posts_[it->second];
  auto &user_likes = likes_[user->id];

  if (user_likes.count(post.id())) {
    user_likes.erase(post.id());
    post.set_likes_count(post.likes_count() - 1);
    response->set_is_liked(false);
  } else {
    user_likes.insert(post.id());
    post.set_likes_count(post.likes_count() + 1);
    response->set_is_liked(true);
  }

  response->set_updated_likes_count(post.likes_count());
  return grpc::Status::OK;
}

grpc::Status CrownServer::Login(grpc::ServerContext *context,
                                const social::LoginRequest *request,
                                social::LoginResult *response) {

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
  response->set_expires_in(REFRESH_EXPIRES);

  social::User source;
  source.set_id(user.id);
  source.set_username(user.name);
  source.set_display_name(user.name);
  source.set_avatar(user.picture);

  social::User *stored = findOrCreateUser(source);

  int post_count = 0;
  for (const auto &p : posts_) {
    if (p.author().id() == user.id)
      post_count++;
  }
  stored->set_posts_count(post_count);

  *response->mutable_user() = *stored;

  return grpc::Status::OK;
}

grpc::Status CrownServer::RefreshAccessToken(
    grpc::ServerContext *context,
    const social::RefreshAccessTokenRequest *request,
    social::LoginResult *response) {

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
  response->set_expires_in(REFRESH_EXPIRES);

  return grpc::Status::OK;
}

bool CrownServer::init(uint16_t port, AppContext &app) {
  app_ = &app;
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
  creators.push_back(std::make_unique<Crown::RatelimitInterceptFactory>(routes));

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
