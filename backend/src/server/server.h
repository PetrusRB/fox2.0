#pragma once

#include "../app.h"
#include "../gen/social.grpc.pb.h"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#define MAX_CONTENT_CHARS 5000
#define MAX_TITLE_CHARS 256
#define MAX_IMAGE_CHARS 2048

namespace Crown {

class CrownServer : public social::PostService::Service,
                    public social::AuthService::Service,
                    public social::UserService::Service,
                    public social::InteractionService::Service {
public:
  bool init(uint16_t port, AppContext &app);
  void shutdown();

  grpc::Status CreatePost(grpc::ServerContext *context,
                          const social::CreatePostRequest *request,
                          social::Post *response) override;

  grpc::Status GetPost(grpc::ServerContext *context,
                       const social::GetPostRequest *request,
                       social::Post *response) override;

  grpc::Status DeletePost(grpc::ServerContext *context,
                          const social::DeletePostRequest *request,
                          social::Empty *response) override;

  grpc::Status ListFeed(grpc::ServerContext *context,
                        const social::ListFeedRequest *request,
                        social::PostList *response) override;

  grpc::Status ListUserPosts(grpc::ServerContext *context,
                             const social::ListUserPostsRequest *request,
                             social::PostList *response) override;

  grpc::Status Login(grpc::ServerContext *context,
                     const social::LoginRequest *request,
                     social::LoginResult *response) override;

  grpc::Status
  RefreshAccessToken(grpc::ServerContext *context,
                     const social::RefreshAccessTokenRequest *request,
                     social::LoginResult *response) override;

  grpc::Status GetProfile(grpc::ServerContext *context,
                          const social::GetProfileRequest *request,
                          social::User *response) override;

  grpc::Status ToggleLike(grpc::ServerContext *context,
                          const social::ToggleLikeRequest *request,
                          social::ToggleLikeResult *response) override;

private:
  AppContext *app_ = nullptr;
  std::unique_ptr<grpc::Server> server_;
  std::vector<social::Post> posts_;
  std::vector<social::User> users_;
  std::unordered_map<std::string, size_t> user_index_;
  std::unordered_map<std::string, size_t> post_index_;
  std::unordered_map<std::string, std::unordered_set<std::string>> likes_;
  uint32_t next_id_ = 1;

  social::User *findOrCreateUser(const social::User &source);
};

} // namespace Crown
