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

// --------------------- Helpers para decodificação ---------------------
function decodeUrl(url: string): string {
  if (!url) return url;
  return url.replace(/(\\|\/)u([0-9a-fA-F]{4})/g, (_, _prefix, hex) =>
    String.fromCharCode(parseInt(hex, 16)),
  );
}
const utf8Decoder = new TextDecoder("utf-8");
function utf8(textoIncorreto: string): string {
  try {
    const len = textoIncorreto.length;
    const bytes = new Uint8Array(len);

    for (let i = 0; i < len; i++) {
      bytes[i] = textoIncorreto.charCodeAt(i);
    }

    return utf8Decoder.decode(bytes);
  } catch (e) {
    return textoIncorreto;
  }
}
function decodePost(post: Post): Post {
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
  return {
    ...decoded,
    title: utf8(decoded.title),
    content: utf8(decoded.content),
  };
}

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

const AUTH_BYPASS = new Set(["login", "refreshAccessToken"]);

function createProtectedClient<T extends object>(client: T): T {
  return new Proxy(client, {
    get(target, prop, receiver) {
      const value = Reflect.get(target, prop, receiver);
      if (
        typeof prop !== "string" ||
        typeof value !== "function" ||
        AUTH_BYPASS.has(prop)
      ) {
        return value;
      }
      return (...args: unknown[]) =>
        handleUnauthenticated(() => value.apply(target, args));
    },
  });
}

function getPostClient(): PostServiceClient {
  if (!postClient) {
    postClient = createProtectedClient(new PostServiceClient(getTransport()));
  }
  return postClient;
}

function getInteractionClient(): InteractionServiceClient {
  if (!interactionClient) {
    interactionClient = createProtectedClient(
      new InteractionServiceClient(getTransport()),
    );
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
    userClient = createProtectedClient(new UserServiceClient(getTransport()));
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

  if (response.idToken) {
    setCookie("id_token", response.idToken, 1);
    if (response.refreshToken) {
      setCookie("refresh_token", response.refreshToken, 30);
    }
  }

  return response;
}

export function hasAccessToken(): boolean {
  return !!getCookie("id_token");
}

export function clearAccessToken(): void {
  deleteCookie("id_token");
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

    if (response.idToken) {
      setCookie("id_token", response.idToken, 1);
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

  const client = getPostClient();
  const { response } = await client.createPost({
    title: clean.title,
    content: clean.content,
    imageUrl: clean.imageUrl,
  });
  return response;
}

export async function getPost(postId: string): Promise<Post> {
  const client = getPostClient();
  const { response } = await client.getPost({ postId });
  return decodePost(response);
}

export async function deletePost(postId: string): Promise<void> {
  const client = getPostClient();
  await client.deletePost({ postId });
}

export async function listFeed(page = 1, limit = 20): Promise<Post[]> {
  const client = getPostClient();
  const { response } = await client.listFeed({ page, limit });
  return response.posts.map(decodePost);
}

export async function listUserPosts(
  userId: string,
  page = 1,
  limit = 20,
): Promise<Post[]> {
  const client = getPostClient();
  const { response } = await client.listUserPosts({ userId, page, limit });
  return response.posts.map(decodePost);
}

// --------------------- Interações ---------------------
export async function toggleLikePost(
  postId: string,
): Promise<ToggleLikeResult> {
  const client = getInteractionClient();
  const { response } = await client.toggleLike({ postId });
  return response;
}

export async function addComment(
  postId: string,
  content: string,
): Promise<CommentItem> {
  const { sanitizeString } = await import("./sanitize");
  const cleanContent = sanitizeString(content, MAX_CONTENT_CHARS);

  const client = getInteractionClient();
  const { response } = await client.addComment({
    postId,
    content: cleanContent,
  });
  return response;
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
