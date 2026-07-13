import {
  createPost,
  listFeed,
  deletePost,
  toggleLikePost,
} from "@/composables/useSocialClient";
import { MAX_PAGE_FEED, MIN_PAGE_FEED } from "@/config/config";
import type { Post } from "@/proto/social";
import { defineStore, acceptHMRUpdate } from "pinia";

interface PostState {
  posts: Post[];
  selectedPost: Post | null;
  isLoading: boolean;
  isCreating: boolean;
  isDeleting: boolean;
  error: string | null;
  page: number;
  hasMore: boolean;
}

export const usePostStore = defineStore("post", {
  state: (): PostState => ({
    posts: [],
    selectedPost: null,
    isLoading: false,
    isCreating: false,
    isDeleting: false,
    error: null,
    page: 1,
    hasMore: true,
  }),

  getters: {
    postById: (state) => (id: string) => state.posts.find((p) => p.id === id),

    sortedPosts: (state) =>
      [...state.posts].sort(
        (a, b) =>
          new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime(),
      ),
  },

  actions: {
    async fetchPosts() {
      if (this.isLoading) return;
      this.isLoading = true;
      this.error = null;
      try {
        const posts = await listFeed(MIN_PAGE_FEED, MAX_PAGE_FEED);
        this.posts = posts;
        this.page = 1;
        this.hasMore = posts.length === 20;
      } catch (err) {
        this.error = `Falha ao carregar feed: ${err}`;
      } finally {
        this.isLoading = false;
      }
    },

    async loadMore() {
      if (this.isLoading || !this.hasMore) return;
      this.isLoading = true;
      try {
        const nextPage = this.page + 1;
        const posts = await listFeed(nextPage, 20);
        this.posts.push(...posts);
        this.page = nextPage;
        this.hasMore = posts.length === 20;
      } catch (err) {
        this.error = `Falha ao carregar mais posts: ${err}`;
      } finally {
        this.isLoading = false;
      }
    },

    async criarPost(post: Post) {
      this.isCreating = true;
      this.error = null;
      try {
        const created = await createPost(
          post.title,
          post.content,
          post.imageUrl,
        );
        this.posts.unshift(created);
        return created;
      } catch (err) {
        this.error = `Falha ao criar post: ${err}`;
        throw err;
      } finally {
        this.isCreating = false;
      }
    },

    async removePost(id: string) {
      const index = this.posts.findIndex((p) => p.id === id);
      if (index === -1) return;

      const removed = this.posts[index]!;
      this.posts.splice(index, 1);
      this.isDeleting = true;

      try {
        await deletePost(id);
      } catch (err) {
        this.posts.splice(index, 0, removed);
        this.error = `Falha ao excluir post: ${err}`;
      } finally {
        this.isDeleting = false;
      }
    },

    async toggleLike(id: string) {
      const post = this.posts.find((p) => p.id === id);
      if (!post) return;

      const prevLiked = post.isLikedByMe;
      const prevCount = post.likesCount;

      post.isLikedByMe = !prevLiked;
      post.likesCount = prevLiked ? prevCount - 1 : prevCount + 1;

      try {
        await toggleLikePost(id);
      } catch {
        post.isLikedByMe = prevLiked;
        post.likesCount = prevCount;
      }
    },

    setSelectedPost(post: Post | null | undefined) {
      this.selectedPost = post ?? null;
    },

    clearError() {
      this.error = null;
    },
  },
});

if (import.meta.hot) {
  import.meta.hot.accept(acceptHMRUpdate(usePostStore, import.meta.hot));
}
