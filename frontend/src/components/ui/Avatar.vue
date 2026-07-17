<script setup lang="ts">
import { computed } from "vue";

export interface AppAvatarProps {
  icon?: string;
  size?: "sm" | "md" | "lg";
  src?: string | undefined;
}

const props = withDefaults(defineProps<AppAvatarProps>(), {
  icon: "search",
  size: "md",
  src: ""
});

const PLACEHOLDER = "/placeholderpfp.png";
const resolvedSrc = computed(() => props.src || PLACEHOLDER);
</script>

<template>
  <div class="app-avatar" :class="`app-avatar--${size}`">
    <img
      v-if="resolvedSrc"
      :src="resolvedSrc"
      alt="Avatar"
      class="app-avatar__img"
      @error="($event.target as HTMLImageElement).src = PLACEHOLDER"
    />
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
