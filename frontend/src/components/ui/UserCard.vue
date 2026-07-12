<script setup lang="ts">
import type { User } from "@/composables/useAuth";
import Avatar from "./Avatar.vue";

export interface UserCardProps {
  collapsed?: boolean;
  user?: User | null;
}

withDefaults(defineProps<UserCardProps>(), {
  collapsed: false,
  user: null,
});
</script>

<template>
  <div class="user-card" :class="{ 'user-card--collapsed': collapsed }">
    <Avatar :src="user?.avatar" size="lg" />
    <div v-if="!collapsed" class="user-card__info">
      <span class="user-card__name">{{ user?.name }}</span>
      <span v-if="user?.handle" class="user-card__handle"
        >@{{ user.handle }}</span
      >
    </div>
  </div>
</template>

<style lang="scss" scoped>
.user-card {
  display: flex;
  align-items: center;
  padding: 12px;
  border-radius: var(--radius-md);
  transition: background 0.15s ease;

  &:hover {
    background: var(--bg-elevated);
  }

  &--collapsed {
    justify-content: center;
    padding: 8px 0;

    .app-avatar {
      margin-right: 0;
    }
  }

  &__info {
    display: flex;
    flex-direction: column;
    margin-left: 12px;
    min-width: 0;
  }

  &__name {
    font-size: 14px;
    font-weight: 600;
    color: var(--text-primary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &__handle {
    font-size: 12px;
    color: var(--text-muted);
  }
}
</style>
