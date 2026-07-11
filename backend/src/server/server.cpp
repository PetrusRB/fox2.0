#include "server.h"

#include "../middlewares/authMiddle.h"
#include <absl/strings/str_format.h>
#include <algorithm>
#include <cctype>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <iostream>

namespace Crown {

std::string SanitizeString(const std::string &input) {
  std::string result;
  result.reserve(input.size());
  bool in_tag = false;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];

    if (c == '<') {
      in_tag = true;
      continue;
    }
    if (c == '>') {
      in_tag = false;
      continue;
    }
    if (in_tag) {
      continue;
    }

    // Bloqueia eventos começando com 'on'
    if (c == 'o' || c == 'O') {
      std::string lower_input = input.substr(i);
      std::transform(lower_input.begin(),
                     lower_input.begin() +
                         std::min(lower_input.size(), (size_t)10),
                     lower_input.begin(), ::tolower);
      if (lower_input.substr(0, 3) == "on" && i + 3 < input.size()) {
        char next = std::tolower(input[i + 2]);
        if (next == '=' || next == ' ' || next == '(') {
          continue;
        }
      }
    }

    result += c;
  }

  return result;
}

std::string SanitizeUrl(const std::string &input) {
  std::string trimmed = input;
  // Remove espaços chatonildos
  auto start = trimmed.find_first_not_of(" \t\n\r");
  if (start == std::string::npos)
    return "";
  auto end = trimmed.find_last_not_of(" \t\n\r");
  trimmed = trimmed.substr(start, end - start + 1);

  // Bloqueia o javascript fi de rapariga + dados
  std::string lower = trimmed;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower.rfind("javascript:", 0) == 0)
    return "";
  if (lower.rfind("data:", 0) == 0)
    return "";

  return trimmed;
}

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
  post.set_likes_count(0);
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

  *response = post;

  std::cout << "Post criado: [" << post.id() << "] por " << user->name << "\n";
  return grpc::Status::OK;
}

grpc::Status CrownServer::GetPost(grpc::ServerContext *context,
                                  const social::GetPostRequest *request,
                                  social::Post *response) {

  for (const auto &post : posts_) {
    if (post.id() == request->post_id()) {
      *response = post;
      return grpc::Status::OK;
    }
  }

  return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
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

  for (auto it = posts_.begin(); it != posts_.end(); ++it) {
    if (it->id() == request->post_id()) {
      if (it->author().id() != user->id) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                            "Not authorized to delete this post");
      }
      std::cout << "Post removido: [" << it->id() << "]\n";
      posts_.erase(it);
      return grpc::Status::OK;
    }
  }

  return grpc::Status(grpc::StatusCode::NOT_FOUND, "Post nao encontrado");
}

grpc::Status CrownServer::ListFeed(grpc::ServerContext *context,
                                   const social::ListFeedRequest *request,
                                   social::PostList *response) {

  uint32_t page = request->page() > 0 ? request->page() : 1;
  uint32_t limit = request->limit() > 0 ? request->limit() : 20;
  uint32_t start = (page - 1) * limit;

  int count = 0;
  for (int i = posts_.size() - 1; i >= 0 && count < static_cast<int>(limit);
       --i) {
    if (static_cast<uint32_t>(count) >= start) {
      *response->add_posts() = posts_[i];
      count++;
    }
  }

  return grpc::Status::OK;
}

grpc::Status
CrownServer::ListUserPosts(grpc::ServerContext *context,
                           const social::ListUserPostsRequest *request,
                           social::PostList *response) {

  uint32_t page = request->page() > 0 ? request->page() : 1;
  uint32_t limit = request->limit() > 0 ? request->limit() : 20;
  uint32_t start = (page - 1) * limit;

  int count = 0;
  for (int i = posts_.size() - 1; i >= 0; --i) {
    if (posts_[i].author().id() == request->user_id()) {
      if (static_cast<uint32_t>(count) >= start &&
          count < static_cast<int>(limit)) {
        *response->add_posts() = posts_[i];
      }
      count++;
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

  auto [user_status, user] = AuthMiddleware::ValidateGoogleToken(tokens.first);
  if (!user_status.ok()) {
    return user_status;
  }

  response->set_access_token(tokens.first);
  response->set_refresh_token(tokens.second);
  response->set_expires_in(3600);

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

bool CrownServer::init(uint16_t port) {
  std::string server_addr = absl::StrFormat("0.0.0.0:%d", port);

  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(static_cast<social::PostService::Service *>(this));
  builder.RegisterService(static_cast<social::AuthService::Service *>(this));
  builder.RegisterService(static_cast<social::UserService::Service *>(this));

  std::vector<
      std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
      creators;
  creators.push_back(std::make_unique<AuthInterceptorFactory>());
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
