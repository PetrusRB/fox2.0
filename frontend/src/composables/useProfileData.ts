import { ref, watch, type Ref } from "vue";
import { getUserByHandle, listUserPosts } from "@/composables/useSocialClient";
import type { User, Post } from "@/proto/social";

export function useProfileData(handle: Ref<string>) {
  const profile = ref<User | null>(null);
  const posts = ref<Post[]>([]);
  const isLoading = ref(true);
  const isLoadingPosts = ref(true);
  const error = ref<string | null>(null);

  async function load() {
    if (!handle.value) return;

    isLoading.value = true;
    error.value = null;
    profile.value = null;
    posts.value = [];

    profile.value = await getUserByHandle(handle.value).catch(() => null);
    isLoading.value = false;

    if (!profile.value) {
      error.value = "Usuário não encontrado";
      isLoadingPosts.value = false;
      return;
    }

    isLoadingPosts.value = true;
    posts.value = await listUserPosts(profile.value.id).catch(() => []);
    isLoadingPosts.value = false;
  }

  watch(handle, load, { immediate: true });

  return { profile, posts, isLoading, isLoadingPosts, error };
}
