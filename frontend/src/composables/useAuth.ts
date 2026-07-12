import { ref, computed } from "vue";
import {
  loginWithGoogle,
  getGoogleOAuthUrl,
  clearAccessToken,
  hasAccessToken,
  refreshAccessToken,
  setCookie,
  getCookie,
  deleteCookie,
  getProfile,
} from "./useSocialClient";
import { parseGrpcError, type AppError } from "./errors";
import { RpcError } from "@protobuf-ts/runtime-rpc";
import type { User as GrpcUser } from "@/proto/social";
import { decodeUnicodeEscapes } from "./sanitize";

export interface User {
  id: string;
  name: string;
  handle: string;
  bio: string;
  followers: number;
  following: number;
  email: string;
  avatar?: string;
}

const user = ref<User | null>(null);
const isLoading = ref(false);
const error = ref<AppError | null>(null);

function mapGrpcUser(grpcUser: GrpcUser): User {
  return {
    id: grpcUser.id,
    name: grpcUser.displayName || grpcUser.username,
    bio: grpcUser.bio || "Sem bio",
    handle: grpcUser.handle || grpcUser.username,
    followers: grpcUser.followersCount || 0,
    following: grpcUser.followingCount || 0,
    email: grpcUser.id,
    avatar: decodeUnicodeEscapes(grpcUser.avatar || "/placeholderpfp.png"),
  };
}

function saveUser(u: User) {
  localStorage.setItem("auth_user", JSON.stringify(u));
  setCookie("auth_user", JSON.stringify(u), 30);
  setCookie("auth_user_id", u.id, 30);
}

function loadUser(): User | null {
  const raw = localStorage.getItem("auth_user") || getCookie("auth_user");
  if (!raw) return null;
  try {
    return JSON.parse(raw);
  } catch {
    return null;
  }
}

function clearUser() {
  localStorage.removeItem("auth_user");
  deleteCookie("auth_user");
  deleteCookie("auth_user_id");
}

export function useAuth() {
  const isAuthenticated = computed(() => !!user.value);

  async function restoreSession(): Promise<boolean> {
    if (!hasAccessToken()) return false;

    const saved = loadUser();
    const userId = saved?.id || getCookie("auth_user_id");

    if (saved) {
      user.value = saved;
    }

    if (!userId) return false;

    try {
      const fresh = await getProfile(userId);
      if (fresh.displayName || fresh.username) {
        const updated = mapGrpcUser(fresh);
        user.value = updated;
        saveUser(updated);
      }
    } catch (e) {
      if (e instanceof RpcError && e.code === "UNAUTHENTICATED") {
        console.log(
          "Piece of shitos, is trying to do refresh token. But is not working, look a that :D",
        );
        const ok = await refreshAccessToken();
        if (ok) {
          console.log(
            "Misericordia, funcionou o bem dito refresh token. YEAAAAAAAH!!!!!!!",
          );
          try {
            const fresh = await getProfile(userId);
            if (fresh.displayName || fresh.username) {
              const updated = mapGrpcUser(fresh);
              user.value = updated;
              saveUser(updated);
            }
          } catch {
            if (!user.value) return false;
          }
        } else {
          user.value = null;
          return false;
        }
      } else {
        if (!user.value) return false;
      }
    }

    return true;
  }

  function initiateGoogleLogin() {
    error.value = null;
    window.location.href = getGoogleOAuthUrl();
  }

  async function handleOAuthCallback(code: string): Promise<boolean> {
    isLoading.value = true;
    error.value = null;

    try {
      const redirectUri = window.location.origin;
      const result = await loginWithGoogle(code, redirectUri);

      if (result.user) {
        user.value = mapGrpcUser(result.user);
        saveUser(user.value);
      }
      return true;
    } catch (e) {
      error.value = parseGrpcError(e);
      return false;
    } finally {
      isLoading.value = false;
    }
  }

  function clearError() {
    error.value = null;
  }

  function logout() {
    user.value = null;
    error.value = null;
    clearAccessToken();
    clearUser();
  }

  return {
    user,
    isLoading,
    error,
    isAuthenticated,
    restoreSession,
    initiateGoogleLogin,
    handleOAuthCallback,
    clearError,
    logout,
  };
}
