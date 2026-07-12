<script setup lang="ts">
import { computed } from "vue";
import { Post } from "@/types/post.type";

const props = defineProps<{
  post: Post;
}>();

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
          v-if="post.author?.avatarUrl"
          :src="post.author.avatarUrl"
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
      <button class="post-card__menu">
        <q-icon name="more_vert" size="18px" />
      </button>
    </div>

    <div class="post-card__body">
      <h3 class="post-card__title">{{ post.title }}</h3>
      <p class="post-card__content">{{ post.content }}</p>
      <img
        v-if="post.imageUrl"
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

  &__menu {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 32px;
    height: 32px;
    background: transparent;
    border: none;
    border-radius: 50%;
    color: var(--text-muted);
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      background: var(--bg-elevated);
      color: var(--text-primary);
    }
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
