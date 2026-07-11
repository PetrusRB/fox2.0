#pragma once

#include "../gen/social.grpc.pb.h"
#include <cstdint>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <unordered_map>

#define MAX_CONTENT_CHARS 5000
#define MAX_TITLE_CHARS 256
#define MAX_IMAGE_CHARS 2048

namespace Crown {

std::string SanitizeString(const std::string &input);
std::string SanitizeUrl(const std::string &input);

class CrownServer : public social::PostService::Service,
                    public social::AuthService::Service,
                    public social::UserService::Service {
public:
  bool init(uint16_t port);
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

  grpc::Status GetProfile(grpc::ServerContext *context,
                          const social::GetProfileRequest *request,
                          social::User *response) override;

private:
  std::unique_ptr<grpc::Server> server_;
  std::vector<social::Post> posts_;
  std::vector<social::User> users_;
  std::unordered_map<std::string, size_t> user_index_;
  uint32_t next_id_ = 1;

  social::User *findOrCreateUser(const social::User &source);
};

} // namespace Crown
