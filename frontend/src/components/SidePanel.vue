<script setup lang="ts">
import { computed } from "vue";
import { useRoute } from "vue-router";
import { useQuasar } from "quasar";
import UserCard from "./ui/UserCard.vue";
import { useAuth } from "@/composables/useAuth";
const { user } = useAuth();

export interface SidebarItem {
  key: string;
  label: string;
  icon: string;
  to?: string;
  href?: string;
  external?: boolean;
  badge?: number;
  section?: string;
}

export interface SidebarProps {
  items?: SidebarItem[];
  collapsed?: boolean;
}

const props = withDefaults(defineProps<SidebarProps>(), {
  items: () => [],
  collapsed: false,
});

defineEmits<{
  toggleCollapse: [];
  itemClick: [item: SidebarItem];
}>();

const route = useRoute();
const $q = useQuasar();

function isActive(item: SidebarItem) {
  if (item.to && route) {
    return route.path === item.to || route.path.startsWith(item.to + "/");
  }
  return false;
}

const isMobile = computed(() => $q.screen.lt.md);
const isTablet = computed(() => $q.screen.md);
</script>

<template>
  <aside
    class="sidebar"
    :class="{
      'sidebar--collapsed': collapsed && !isMobile,
      'sidebar--mobile': isMobile,
    }"
  >
    <div class="sidebar__content">
      <!-- User Profile Card -->
      <UserCard :user="user" :collapsed="collapsed && !isMobile" />

      <div class="sidebar__divider" />

      <!-- Navigation Items -->
      <nav class="sidebar__nav">
        <template v-for="item in items" :key="item.key">
          <!-- Section Label -->
          <div
            v-if="item.section && (!collapsed || isMobile)"
            class="sidebar__section"
          >
            {{ item.section }}
          </div>

          <!-- Nav Item -->
          <a
            :href="item.to || item.href"
            class="sidebar__item"
            :class="{
              'sidebar__item--active': isActive(item),
              'sidebar__item--icon-only': collapsed && !isMobile,
            }"
            :target="item.external ? '_blank' : undefined"
            :title="item.label"
            @click="$emit('itemClick', item)"
          >
            <div class="sidebar__item-icon">
              <q-icon :name="item.icon" size="22px" />
              <span v-if="item.badge" class="sidebar__item-badge">
                {{ item.badge > 99 ? "99+" : item.badge }}
              </span>
            </div>
            <span v-if="!collapsed || isMobile" class="sidebar__item-label">
              {{ item.label }}
            </span>
          </a>
        </template>
      </nav>

      <div class="sidebar__divider" />

      <!-- Quick Actions -->
      <div class="sidebar__actions">
        <button class="sidebar__action-btn" title="Configurações">
          <q-icon name="settings" size="20px" />
          <span v-if="!collapsed || isMobile">Configurações</span>
        </button>
      </div>
    </div>

    <!-- Collapse Toggle (Desktop only) -->
    <button
      v-if="!isMobile"
      class="sidebar__toggle"
      @click="$emit('toggleCollapse')"
    >
      <q-icon
        :name="collapsed ? 'chevron_right' : 'chevron_left'"
        size="18px"
      />
    </button>
  </aside>
</template>

<style lang="scss" scoped>
.sidebar {
  position: fixed;
  top: var(--navbar-height);
  left: 0;
  bottom: 0;
  width: var(--sidebar-width);
  background: var(--bg-surface);
  border-right: 1px solid var(--border-color);
  z-index: 900;
  display: flex;
  flex-direction: column;
  transition:
    width 0.2s ease,
    transform 0.2s ease;

  &--collapsed {
    width: 72px;

    .sidebar__content {
      padding: 12px 8px;
    }

    .sidebar__profile {
      justify-content: center;
      padding: 8px 0;
    }

    .sidebar__avatar {
      margin-right: 0;
    }

    .sidebar__item {
      justify-content: center;
      padding: 12px;
    }

    .sidebar__item-icon {
      margin-right: 0;
    }

    .sidebar__actions {
      padding: 0;
    }

    .sidebar__action-btn {
      justify-content: center;
      padding: 12px;

      span {
        display: none;
      }
    }

    .sidebar__toggle {
      right: -12px;
    }
  }

  &--mobile {
    transform: translateX(-100%);

    &.sidebar--open {
      transform: translateX(0);
    }
  }

  &__content {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: 16px 12px;
  }

  &__profile {
    display: flex;
    align-items: center;
    padding: 12px;
    border-radius: var(--radius-md);
    transition: background 0.15s ease;

    &:hover {
      background: var(--bg-elevated);
    }
  }

  &__avatar {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 44px;
    height: 44px;
    background: linear-gradient(135deg, var(--accent), var(--avocado-500));
    border-radius: 50%;
    color: var(--zinc-950);
    flex-shrink: 0;
    margin-right: 12px;
  }

  &__profile-info {
    display: flex;
    flex-direction: column;
    min-width: 0;
  }

  &__profile-name {
    font-size: 14px;
    font-weight: 600;
    color: var(--text-primary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &__profile-handle {
    font-size: 12px;
    color: var(--text-muted);
  }

  &__divider {
    height: 1px;
    background: var(--border-color);
    margin: 12px 0;
  }

  &__nav {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  &__section {
    font-size: 11px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--text-muted);
    font-weight: 500;
    padding: 16px 12px 8px;
  }

  &__item {
    display: flex;
    align-items: center;
    padding: 10px 12px;
    color: var(--text-secondary);
    text-decoration: none;
    border-radius: var(--radius-md);
    font-size: 14px;
    font-weight: 500;
    transition: all 0.15s ease;
    position: relative;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }

    &--active {
      color: var(--accent);
      background: var(--accent-bg);

      &:hover {
        color: var(--accent-hover);
        background: var(--accent-bg);
      }

      .sidebar__item-icon {
        color: var(--accent);
      }
    }

    &--icon-only {
      justify-content: center;
      padding: 12px;
    }
  }

  &__item-icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 24px;
    height: 24px;
    margin-right: 12px;
    position: relative;
    flex-shrink: 0;

    .sidebar__item--icon-only & {
      margin-right: 0;
    }
  }

  &__item-badge {
    position: absolute;
    top: -4px;
    right: -8px;
    min-width: 18px;
    height: 18px;
    padding: 0 5px;
    background: var(--negative);
    color: white;
    font-size: 10px;
    font-weight: 600;
    border-radius: 9px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  &__item-label {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;

    .sidebar__item--icon-only & {
      display: none;
    }
  }

  &__actions {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  &__action-btn {
    display: flex;
    align-items: center;
    width: 100%;
    padding: 10px 12px;
    color: var(--text-secondary);
    background: none;
    border: none;
    border-radius: var(--radius-md);
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }

    .q-icon {
      margin-right: 12px;
    }
  }

  &__toggle {
    position: absolute;
    top: 50%;
    right: -14px;
    transform: translateY(-50%);
    width: 28px;
    height: 28px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--bg-elevated);
    border: 1px solid var(--border-color);
    border-radius: 50%;
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.15s ease;
    opacity: 0;
    z-index: 10;

    .sidebar:hover & {
      opacity: 1;
    }

    &:hover {
      background: var(--accent);
      border-color: var(--accent);
      color: var(--zinc-950);
    }
  }

  // Mobile overlay
  &--mobile {
    &::before {
      content: "";
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(0, 0, 0, 0.5);
      opacity: 0;
      visibility: hidden;
      transition: all 0.2s ease;
      z-index: -1;
    }

    &.sidebar--open::before {
      opacity: 1;
      visibility: visible;
    }
  }
}
</style>
