import z from "zod";

export const UserSchema = z.object({
  id: z.string(),
  username: z.string(),
  displayName: z.string(),
  avatarUrl: z.string(),
  bio: z.string(),
  followersCount: z.number(),
  followingCount: z.number(),
  postsCount: z.number(),
  createdAt: z.string()
});

export const PostSchema = z.object({
  id: z.string(),
  author: UserSchema.optional(),
  title: z.string(),
  content: z.string(),
  imageUrl: z.string().optional(),
  likesCount: z.number(),
  commentsCount: z.number(),
  isLikedByMe: z.boolean(),
  createdAt: z.string()
});

export const CommentItemSchema = z.object({
  id: z.string(),
  author: UserSchema,
  postId: z.string(),
  content: z.string(),
  createdAt: z.string()
});

export type User = z.infer<typeof UserSchema>;
export type Post = z.infer<typeof PostSchema>;
export type CommentItem = z.infer<typeof CommentItemSchema>;
