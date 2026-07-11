<script setup lang="ts">
export interface AppAvatarProps {
  icon?: string;
  size?: "sm" | "md" | "lg";
  src?: string;
}

withDefaults(defineProps<AppAvatarProps>(), {
  icon: "search",
  size: "md",
  src: "/placeholderpfp.png",
});
</script>

<template>
  <div class="app-avatar" :class="`app-avatar--${size}`">
    <img v-if="src" :src="src" alt="Avatar" class="app-avatar__img" />
    <q-icon v-else :name="icon" />
  </div>
</template>

<style lang="scss" scoped>
.app-avatar {
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--bg-elevated);
  border: 2px solid var(--border-color);
  border-radius: 50%;
  color: var(--text-secondary);
  flex-shrink: 0;
  transition: all 0.15s ease;
  overflow: hidden;

  &--sm {
    width: 32px;
    height: 32px;
    font-size: 16px;
  }

  &--md {
    width: 36px;
    height: 36px;
    font-size: 18px;

    @media (max-width: 768px) {
      width: 32px;
      height: 32px;
      font-size: 16px;
    }
  }

  &--lg {
    width: 44px;
    height: 44px;
    font-size: 22px;
  }

  &__img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
}
</style>
