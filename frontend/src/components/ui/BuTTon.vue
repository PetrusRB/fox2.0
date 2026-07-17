<script setup lang="ts">
export interface BuTTonProps {
  variant?:
    | "primary"
    | "secondary"
    | "success"
    | "error"
    | "warning"
    | "ghost"
    | "icon";
  size?: "sm" | "md" | "lg";
  icon?: string;
  iconPosition?: "left" | "right";
  badge?: number;
  tooltip?: string;
  to?: string;
  href?: string;
  external?: boolean;
  disabled?: boolean;
  loading?: boolean;
  block?: boolean;
}

const props = withDefaults(defineProps<BuTTonProps>(), {
  variant: "primary",
  size: "md",
  icon: "",
  iconPosition: "left",
  badge: 0,
  tooltip: "",
  to: "",
  href: "",
  external: false,
  disabled: false,
  loading: false,
  block: false
});

defineEmits<{
  click: [event: MouseEvent];
}>();
</script>

<template>
  <component
    :is="to ? 'router-link' : href ? 'a' : 'button'"
    :to="to || undefined"
    :href="href || undefined"
    :target="external ? '_blank' : undefined"
    :title="tooltip || undefined"
    :disabled="disabled || loading"
    :class="[
      'btn',
      `btn--${variant}`,
      `btn--${size}`,
      {
        'btn--block': block,
        'btn--loading': loading,
        'btn--icon-only': variant === 'icon'
      }
    ]"
    @click="$emit('click', $event)"
  >
    <q-spinner
      v-if="loading"
      :size="size === 'sm' ? '14px' : size === 'lg' ? '20px' : '16px'"
    />

    <template v-else>
      <q-icon
        v-if="icon && iconPosition === 'left'"
        :name="icon"
        :size="size === 'sm' ? '16px' : size === 'lg' ? '22px' : '18px'"
        class="btn__icon"
      />

      <slot />

      <q-icon
        v-if="icon && iconPosition === 'right'"
        :name="icon"
        :size="size === 'sm' ? '16px' : size === 'lg' ? '22px' : '18px'"
        class="btn__icon"
      />
    </template>

    <span v-if="badge" class="btn__badge">
      {{ badge > 99 ? "99+" : badge }}
    </span>
  </component>
</template>

<style lang="scss" scoped>
.btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  font-weight: 500;
  border: none;
  cursor: pointer;
  transition: all 0.15s ease;
  text-decoration: none;
  position: relative;
  white-space: nowrap;

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
    pointer-events: none;
  }

  // Tamanhos
  &--sm {
    height: 32px;
    padding: 0 12px;
    font-size: 13px;
    border-radius: var(--radius-sm);
  }

  &--md {
    height: 40px;
    padding: 0 16px;
    font-size: 14px;
    border-radius: var(--radius-md);
  }

  &--lg {
    height: 48px;
    padding: 0 24px;
    font-size: 15px;
    border-radius: var(--radius-md);
  }

  // Icoonee
  &--icon {
    padding: 0;
    background: transparent;
    color: var(--text-secondary);
    border-radius: 50%;

    &.btn--sm {
      width: 32px;
      height: 32px;
    }

    &.btn--md {
      width: 40px;
      height: 40px;
    }

    &.btn--lg {
      width: 48px;
      height: 48px;
    }

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }
  }

  // Variantes
  &--primary {
    background: var(--accent);
    color: var(--zinc-950);

    &:hover:not(:disabled) {
      background: var(--accent-hover);
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(101, 163, 13, 0.3);
    }

    &:active:not(:disabled) {
      transform: translateY(0);
    }
  }

  &--secondary {
    background: var(--bg-elevated);
    color: var(--text-primary);
    border: 1px solid var(--border-color);

    &:hover:not(:disabled) {
      background: var(--bg-primary);
      border-color: var(--text-muted);
    }
  }

  &--success {
    background: #22c55e;
    color: white;

    &:hover:not(:disabled) {
      background: #16a34a;
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(34, 197, 94, 0.3);
    }
  }

  &--error {
    background: var(--negative);
    color: white;

    &:hover:not(:disabled) {
      background: #dc2626;
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
    }
  }

  &--warning {
    background: #f59e0b;
    color: var(--zinc-950);

    &:hover:not(:disabled) {
      background: #d97706;
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(245, 158, 11, 0.3);
    }
  }

  &--ghost {
    background: transparent;
    color: var(--text-secondary);

    &:hover:not(:disabled) {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }
  }

  // ── Block ────────────────────────────────────────
  &--block {
    width: 100%;
  }

  // ── Loading ──────────────────────────────────────
  &--loading {
    pointer-events: none;
  }
}

.btn__icon {
  flex-shrink: 0;
}

.btn__badge {
  position: absolute;
  top: -4px;
  right: -4px;
  min-width: 18px;
  height: 18px;
  padding: 0 5px;
  background: var(--negative);
  color: white;
  font-size: 11px;
  font-weight: 600;
  border-radius: 9px;
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
