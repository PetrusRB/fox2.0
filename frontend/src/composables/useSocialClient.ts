import { GrpcWebFetchTransport } from "@protobuf-ts/grpcweb-transport";
import { RpcError } from "@protobuf-ts/runtime-rpc";
import {
  PostServiceClient,
  InteractionServiceClient,
  AuthServiceClient,
  UserServiceClient,
} from "@/proto/social.client";
import type { Post, CommentItem, ToggleLikeResult, User } from "@/proto/social";
import { MAX_CONTENT_CHARS } from "@/config/config";

const HOST = import.meta.env.VITE_GRPC_HOST || "http://localhost:8080";

let transport: GrpcWebFetchTransport | null = null;
let postClient: PostServiceClient | null = null;
let interactionClient: InteractionServiceClient | null = null;
let authClient: AuthServiceClient | null = null;
let userClient: UserServiceClient | null = null;

// --------------------- Helpers pros cookies ---------------------
export function setCookie(name: string, value: string, days: number) {
  const expires = new Date(Date.now() + days * 864e5).toUTCString();
  document.cookie = `${name}=${encodeURIComponent(value)}; expires=${expires}; path=/; SameSite=Strict`;
}

export function getCookie(name: string): string | null {
  const match = document.cookie.match(new RegExp(`(?:^|; )${name}=([^;]*)`));
  return match?.[1] ? decodeURIComponent(match[1]) : null;
}

export function deleteCookie(name: string) {
  document.cookie = `${name}=; expires=Thu, 01 Jan 1970 00:00:00 GMT; path=/`;
}

// --------------------- Transport e clients ---------------------
function getTransport(): GrpcWebFetchTransport {
  if (!transport) {
    transport = new GrpcWebFetchTransport({
      baseUrl: HOST,
      fetchInit: { credentials: "include" },
    });
  }
  return transport;
}

function getPostClient(): PostServiceClient {
  if (!postClient) {
    postClient = new PostServiceClient(getTransport());
  }
  return postClient;
}

function getInteractionClient(): InteractionServiceClient {
  if (!interactionClient) {
    interactionClient = new InteractionServiceClient(getTransport());
  }
  return interactionClient;
}

function getAuthClient(): AuthServiceClient {
  if (!authClient) {
    authClient = new AuthServiceClient(getTransport());
  }
  return authClient;
}

function getUserClient(): UserServiceClient {
  if (!userClient) {
    userClient = new UserServiceClient(getTransport());
  }
  return userClient;
}

// --------------------- Auth ---------------------
export async function loginWithGoogle(
  authorizationCode: string,
  redirectUri: string,
) {
  const client = getAuthClient();
  const { response } = await client.login({
    authorizationCode,
    redirectUri,
  });

  if (response.accessToken) {
    setCookie("access_token", response.accessToken, 1);
    if (response.refreshToken) {
      setCookie("refresh_token", response.refreshToken, 30);
    }
  }

  return response;
}

export function hasAccessToken(): boolean {
  return !!getCookie("access_token");
}

export function clearAccessToken(): void {
  deleteCookie("access_token");
  deleteCookie("refresh_token");
}

export function getGoogleOAuthUrl(): string {
  const clientId = import.meta.env.VITE_GOOGLE_CLIENT_ID || "";
  const redirectUri = window.location.origin;
  const params = new URLSearchParams({
    client_id: clientId,
    redirect_uri: redirectUri,
    response_type: "code",
    scope: "openid email profile",
    access_type: "offline",
    prompt: "consent",
  });
  return `https://accounts.google.com/o/oauth2/v2/auth?${params.toString()}`;
}

// --------------------- Token Refresh ---------------------
let refreshPromise: Promise<boolean> | null = null;

export async function refreshAccessToken(): Promise<boolean> {
  const refreshToken = getCookie("refresh_token");
  if (!refreshToken) return false;

  try {
    const client = getAuthClient();
    const { response } = await client.refreshAccessToken({
      refreshToken,
    });

    if (response.accessToken) {
      setCookie("access_token", response.accessToken, 1);
      return true;
    }
    return false;
  } catch {
    clearAccessToken();
    return false;
  }
}

async function handleUnauthenticated<T>(fn: () => Promise<T>): Promise<T> {
  try {
    return await fn();
  } catch (error) {
    if (!(error instanceof RpcError) || error.code !== "UNAUTHENTICATED") {
      throw error;
    }

    if (!refreshPromise) {
      refreshPromise = refreshAccessToken().finally(() => {
        refreshPromise = null;
      });
    }

    const ok = await refreshPromise;
    if (!ok) {
      throw error;
    }

    return fn();
  }
}

// --------------------- Posts ---------------------
export async function createPost(
  title: string,
  content: string,
  imageUrl?: string,
): Promise<Post> {
  const { sanitizePost } = await import("./sanitize");
  const { validateCreatePost } = await import("./validators");

  const clean = sanitizePost(title, content, imageUrl);
  const validation = validateCreatePost(clean);
  if (!validation.success) {
    throw new Error(validation.error.message);
  }

  return handleUnauthenticated(async () => {
    const client = getPostClient();
    const { response } = await client.createPost({
      title: clean.title,
      content: clean.content,
      imageUrl: clean.imageUrl,
    });
    return response;
  });
}

function decodeUrl(url: string): string {
  if (!url) return url;
  return url.replace(/(\\|\/)u([0-9a-fA-F]{4})/g, (_, _prefix, hex) =>
    String.fromCharCode(parseInt(hex, 16)),
  );
}

function decodePostUrls(post: Post): Post {
  let decoded = post;
  if (post.imageUrl) {
    decoded = { ...decoded, imageUrl: decodeUrl(post.imageUrl) };
  }
  if (decoded.author?.avatar) {
    decoded = {
      ...decoded,
      author: { ...decoded.author, avatar: decodeUrl(decoded.author.avatar) },
    };
  }
  return decoded;
}

export async function getPost(postId: string): Promise<Post> {
  const client = getPostClient();
  const { response } = await client.getPost({ postId });
  return decodePostUrls(response);
}

export async function deletePost(postId: string): Promise<void> {
  return handleUnauthenticated(async () => {
    const client = getPostClient();
    await client.deletePost({ postId });
  });
}

export async function listFeed(page = 1, limit = 20): Promise<Post[]> {
  const client = getPostClient();
  const { response } = await client.listFeed({ page, limit });
  return response.posts.map(decodePostUrls);
}

export async function listUserPosts(
  userId: string,
  page = 1,
  limit = 20,
): Promise<Post[]> {
  const client = getPostClient();
  const { response } = await client.listUserPosts({ userId, page, limit });
  return response.posts.map(decodePostUrls);
}

// --------------------- Interações ---------------------
export async function toggleLikePost(
  postId: string,
): Promise<ToggleLikeResult> {
  return handleUnauthenticated(async () => {
    const client = getInteractionClient();
    const { response } = await client.toggleLike({ postId });
    return response;
  });
}

export async function addComment(
  postId: string,
  content: string,
): Promise<CommentItem> {
  const { sanitizeString } = await import("./sanitize");
  const cleanContent = sanitizeString(content, MAX_CONTENT_CHARS);

  return handleUnauthenticated(async () => {
    const client = getInteractionClient();
    const { response } = await client.addComment({
      postId,
      content: cleanContent,
    });
    return response;
  });
}

export async function listComments(
  postId: string,
  page = 1,
  limit = 20,
): Promise<CommentItem[]> {
  const client = getInteractionClient();
  const { response } = await client.listComments({ postId, page, limit });
  return response.comments;
}

// --------------------- Users ---------------------
export async function getProfile(userId: string): Promise<User> {
  const client = getUserClient();
  const { response } = await client.getProfile({ userId });
  return response;
}
