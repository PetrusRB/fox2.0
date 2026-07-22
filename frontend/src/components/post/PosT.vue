<script setup lang="ts">
import { computed } from "vue";
import { Post } from "@/proto/social";
import DropDown, { type DropDownItem } from "../ui/DropDown.vue";
import ViDeoPlayer from "../ui/VideoPlayer.vue";
import { usePostStore } from "@/stores/post-store";
import { useAuth } from "@/composables/useAuth";
import { isVideo } from "@/utils/testMidia";

const props = defineProps<{
  post: Post;
}>();

const postStore = usePostStore();
const { user } = useAuth();

const isAuthor = computed(
  () => !!user.value && user.value.id === props.post.author?.id,
);

const menuItems = computed<DropDownItem[]>(() => {
  const items: DropDownItem[] = [
    { key: "copyLink", label: "Copiar link", icon: "link" },
  ];

  if (isAuthor.value) {
    items.push({ key: "divider", label: "", divider: true });
    items.push({
      key: "delete",
      label: "Excluir post",
      icon: "delete",
      danger: true,
    });
  }

  return items;
});

function handleMenuSelect(item: DropDownItem) {
  switch (item.key) {
    case "delete":
      postStore.removePost(props.post.id);
      break;
    case "copyLink":
      navigator.clipboard.writeText(
        `${window.location.origin}/post/${props.post.id}`,
      );
      break;
  }
}

const formattedDate = computed(() => {
  const date = new Date(props.post.createdAt);
  const now = new Date();
  const diffMs = now.getTime() - date.getTime();
  const diffMins = Math.floor(diffMs / 60000);
  const diffHours = Math.floor(diffMs / 3600000);
  const diffDays = Math.floor(diffMs / 86400000);

  if (diffMins < 1) return "Agora";
  if (diffMins < 60) return `${diffMins}min`;
  if (diffHours < 24) return `${diffHours}h`;
  if (diffDays < 7) return `${diffDays}d`;
  return date.toLocaleDateString("pt-BR", { day: "2-digit", month: "short" });
});

const initials = computed(() => {
  const name = props.post.author?.displayName || "U";
  return name
    .split(" ")
    .map((n) => n[0])
    .join("")
    .toUpperCase()
    .slice(0, 2);
});
</script>

<template>
  <article class="post-card">
    <div class="post-card__header">
      <div class="post-card__avatar">
        <img
          v-if="post.author?.avatar"
          :src="post.author.avatar"
          :alt="post.author.displayName"
        />
        <span v-else>{{ initials }}</span>
      </div>
      <div class="post-card__meta">
        <span class="post-card__author">{{
          post.author?.displayName || "Usuario"
        }}</span>
        <span class="post-card__handle"
          >@{{ post.author?.username || "user" }}</span
        >
      </div>
      <span class="post-card__time">{{ formattedDate }}</span>
      <DropDown
        :items="menuItems"
        placement="right"
        @select="handleMenuSelect"
      />
    </div>

    <div class="post-card__body">
      <h3 class="post-card__title">{{ post.title }}</h3>
      <p class="post-card__content">{{ post.content }}</p>
      <ViDeoPlayer
        v-if="post.imageUrl && isVideo(post.imageUrl)"
        :src="post.imageUrl"
        class="post-card__media"
      />
      <img
        v-else-if="post.imageUrl"
        :src="post.imageUrl"
        alt="Post image"
        loading="lazy"
        class="post-card__image"
      />
    </div>

    <div class="post-card__footer">
      <button
        class="post-card__action"
        :class="{ 'post-card__action--liked': post.isLikedByMe }"
        @click="postStore.toggleLike(post.id)"
      >
        <q-icon
          :name="post.isLikedByMe ? 'favorite' : 'favorite_border'"
          size="18px"
        />
        <span>{{ post.likesCount }}</span>
      </button>
      <button class="post-card__action">
        <q-icon name="chat_bubble_outline" size="18px" />
        <span>{{ post.commentsCount }}</span>
      </button>
      <button class="post-card__action">
        <q-icon name="share" size="18px" />
      </button>
      <button class="post-card__action post-card__action--right">
        <q-icon name="bookmark_border" size="18px" />
      </button>
    </div>
  </article>
</template>

<style lang="scss" scoped>
.post-card {
  background: var(--bg-surface);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 20px;
  transition: all 0.2s ease;

  &:hover {
    border-color: var(--zinc-600);
    box-shadow: 0 4px 24px rgba(0, 0, 0, 0.3);
  }

  &__header {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 16px;
  }

  &__avatar {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 40px;
    height: 40px;
    background: linear-gradient(135deg, var(--accent), var(--avocado-500));
    border-radius: 50%;
    color: var(--zinc-950);
    flex-shrink: 0;
    overflow: hidden;
    font-size: 14px;
    font-weight: 600;
    border: 1px solid transparent;

    transition:
      border-color 0.25s ease,
      transform 0.25s ease;

    cursor: pointer;
    &:hover {
      border-color: var(--secondary);
      transform: scale(1.06);
    }
    img {
      width: 100%;
      height: 100%;
      object-fit: cover;
    }
  }

  &__meta {
    display: flex;
    flex-direction: column;
    flex: 1;
    min-width: 0;
  }

  &__author {
    font-size: 14px;
    font-weight: 600;
    color: var(--text-primary);
  }

  &__handle {
    font-size: 12px;
    color: var(--text-muted);
  }

  &__time {
    font-size: 12px;
    color: var(--text-muted);
  }

  &__body {
    margin-bottom: 16px;
  }

  &__title {
    font-size: 17px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 8px;
    line-height: 1.4;
  }

  &__content {
    font-size: 15px;
    color: var(--text-primary);
    line-height: 1.6;
    white-space: pre-wrap;
    word-break: break-word;
  }

  &__image {
    width: 100%;
    border-radius: var(--radius-md);
    margin-top: 12px;
    max-height: 400px;
    object-fit: cover;
  }

  &__media {
    margin-top: 12px;
  }

  &__footer {
    display: flex;
    align-items: center;
    gap: 4px;
    padding-top: 16px;
    border-top: 1px solid var(--border-color);
  }

  &__action {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 12px;
    background: transparent;
    border: none;
    border-radius: var(--radius-sm);
    color: var(--text-muted);
    font-size: 13px;
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      background: var(--accent-bg);
      color: var(--accent);
    }

    &--liked {
      color: #ef4444;

      &:hover {
        background: rgba(239, 68, 68, 0.1);
        color: #ef4444;
      }
    }

    span {
      font-weight: 500;
    }

    &--right {
      margin-left: auto;
    }
  }
}
</style>
