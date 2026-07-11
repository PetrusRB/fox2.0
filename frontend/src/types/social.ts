export interface User {
  id: string;
  username: string;
  displayName: string;
  avatarUrl: string;
  bio: string;
  followersCount: number;
  followingCount: number;
  postsCount: number;
  createdAt: string;
}

export interface Post {
  id: string;
  author?: User;
  content: string;
  imageUrl: string;
  likesCount: number;
  commentsCount: number;
  isLikedByMe: boolean;
  createdAt: string;
}

export interface CommentItem {
  id: string;
  author: User;
  postId: string;
  content: string;
  createdAt: string;
}

export interface Notification {
  id: string;
  type: "like" | "comment" | "follow";
  fromUser: User;
  postId: string;
  message: string;
  isRead: boolean;
  createdAt: string;
}
