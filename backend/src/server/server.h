#pragma once

#include "../app.h"
#include "../gen/social.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

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

  grpc::Status GetUserByHandle(grpc::ServerContext *context,
                               const social::GetUserByHandleRequest *request,
                               social::User *response) override;

  grpc::Status ToggleLike(grpc::ServerContext *context,
                          const social::ToggleLikeRequest *request,
                          social::ToggleLikeResult *response) override;

private:
  AppContext *app_ = nullptr;
  std::unique_ptr<grpc::Server> server_;
};

} // namespace Crown
