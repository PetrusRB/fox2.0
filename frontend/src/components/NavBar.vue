<script setup lang="ts">
import { computed, ref } from "vue";
import { useRoute } from "vue-router";
import { useQuasar } from "quasar";
import Avatar from "./ui/Avatar.vue";
import BuTTon from "./ui/BuTTon.vue";
import InpUt from "./ui/InpUt.vue";
import Dropdown, { DropDownItem } from "./ui/DropDown.vue";
import { useAuth } from "@/composables/useAuth";
const { user, logout } = useAuth();

export interface NavItem {
  key: string;
  label: string;
  icon?: string;
  to?: string;
  href?: string;
  external?: boolean;
  caption?: string;
  section?: string;
  visible?: boolean | (() => boolean);
  children?: NavItem[];
  divider?: boolean;
}

export interface NavAction {
  key: string;
  icon: string;
  tooltip: string;
  to?: string;
  href?: string;
  external?: boolean;
  visible?: boolean | (() => boolean);
}

export interface NavBarProps {
  brand?: string;
  brandIcon?: string;
  items?: NavItem[];
  actions?: NavAction[];
}

const props = withDefaults(defineProps<NavBarProps>(), {
  brand: "Fox",
  brandIcon: "pets",
  items: () => [],
  actions: () => []
});

defineEmits<{
  logoClick: [];
  toggleDrawer: [];
}>();

import { matExitToApp } from "@quasar/extras/material-icons";

const menuItems: DropDownItem[] = [
  { key: "settings", label: "Settings", icon: "settings" },
  { key: "divider", label: "", divider: true },
  {
    key: "logout",
    label: "Logout",
    icon: `${matExitToApp}`,
    danger: true
  }
];

async function handleMenuSelect(item: DropDownItem) {
  switch (item.key) {
    case "logout":
      await logout();
      break;
    case "settings":
      console.log("cooming soon");
      break;
  }
}

const route = useRoute();
const $q = useQuasar();
const searchQuery = ref("");

function isVisible(item: { visible?: boolean | (() => boolean) }) {
  if (item.visible === undefined) return true;
  if (typeof item.visible === "function") return item.visible();
  return item.visible;
}

const visibleItems = computed(() => props.items.filter(isVisible));
const visibleActions = computed(() => props.actions.filter(isVisible));

function isActive(item: NavItem) {
  if (item.to && route) {
    return route.path === item.to || route.path.startsWith(item.to + "/");
  }
  return false;
}

const isMobile = computed(() => $q.screen.lt.md);
</script>

<template>
  <header class="topbar">
    <div class="topbar__container">
      <!-- Left: Logo + Brand -->
      <div class="topbar__left">
        <button class="topbar__menu-btn" @click="$emit('toggleDrawer')">
          <q-icon name="menu" size="22px" />
        </button>

        <div class="topbar__brand" @click="$emit('logoClick')">
          <div class="topbar__logo">
            <q-icon :name="brandIcon" size="26px" />
          </div>
          <span v-if="!isMobile" class="topbar__brand-text">{{ brand }}</span>
        </div>
      </div>

      <!-- Center: Search Bar -->
      <div class="topbar__center">
        <InpUt
          v-model="searchQuery"
          placeholder="Pesquise algo aqui..."
          icon="search"
          right-icon="close"
          variant="rounded"
          size="md"
        />
      </div>

      <!-- Right: Nav Links + Actions -->
      <div class="topbar__right">
        <!-- Desktop Nav Links -->
        <nav v-if="!isMobile" class="topbar__nav">
          <template v-for="item in visibleItems" :key="item.key">
            <a
              v-if="!item.children"
              :href="item.to || item.href"
              class="topbar__nav-link"
              :class="{ 'topbar__nav-link--active': isActive(item) }"
              :target="item.external ? '_blank' : undefined"
            >
              <q-icon v-if="item.icon" :name="item.icon" size="20px" />
              <span class="topbar__nav-label">{{ item.label }}</span>
            </a>

            <!-- Dropdown -->
            <div v-else class="topbar__dropdown">
              <button class="topbar__nav-link topbar__dropdown-trigger">
                <q-icon v-if="item.icon" :name="item.icon" size="20px" />
                <span class="topbar__nav-label">{{ item.label }}</span>
                <q-icon name="expand_more" size="16px" />
              </button>
              <div class="topbar__dropdown-menu">
                <template v-for="child in item.children" :key="child.key">
                  <a
                    v-if="!child.divider"
                    :href="child.to || child.href"
                    class="topbar__dropdown-item"
                    :target="child.external ? '_blank' : undefined"
                  >
                    <q-icon v-if="child.icon" :name="child.icon" size="18px" />
                    <span>{{ child.label }}</span>
                  </a>
                  <div v-else class="topbar__dropdown-divider" />
                </template>
              </div>
            </div>
          </template>
        </nav>

        <div v-if="!isMobile" class="topbar__divider" />

        <!-- Action Buttons -->
        <div class="topbar__actions">
          <template v-for="action in visibleActions" :key="action.key">
            <BuTTon variant="icon" v-bind="action" />
          </template>

          <!-- Notifications -->
          <BuTTon variant="icon" icon="notifications" :badge="3" />

          <!-- User Avatar -->
          <Dropdown
            :items="menuItems"
            placement="right"
            @select="handleMenuSelect"
          >
            <template #trigger>
              <div class="topbar__avatar">
                <Avatar :src="user?.avatar" size="md" />
              </div>
            </template>

            <template v-if="user" #header>
              <div class="topbar__user-info">
                <Avatar :src="user.avatar" size="sm" />
                <div class="topbar__user-details">
                  <span class="topbar__user-name">{{ user.name }}</span>
                  <span class="topbar__user-handle">@{{ user.handle }}</span>
                </div>
              </div>
            </template>
          </Dropdown>
        </div>
      </div>
    </div>
  </header>
</template>

<style lang="scss" scoped>
.topbar {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  height: var(--navbar-height);
  background: var(--bg-surface);
  border-bottom: 1px solid var(--border-color);
  z-index: 1000;
  backdrop-filter: blur(12px);
  background: rgba(24, 24, 27, 0.95);

  &__container {
    display: flex;
    align-items: center;
    height: 100%;
    padding: 0 16px;
    max-width: 1400px;
    margin: 0 auto;
    gap: 8px;

    @media (max-width: 768px) {
      padding: 0 12px;
      gap: 4px;
    }
  }

  &__left {
    display: flex;
    align-items: center;
    gap: 12px;
    min-width: fit-content;

    @media (max-width: 768px) {
      gap: 8px;
    }
  }

  &__menu-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 36px;
    height: 36px;
    border: none;
    background: transparent;
    color: var(--text-secondary);
    border-radius: var(--radius-sm);
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      background: var(--bg-elevated);
      color: var(--text-primary);
    }

    @media (min-width: 1024px) {
      display: none;
    }
  }

  &__brand {
    display: flex;
    align-items: center;
    gap: 10px;
    cursor: pointer;
    text-decoration: none;
  }

  &__logo {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 36px;
    height: 36px;
    background: var(--accent);
    border-radius: var(--radius-md);
    color: var(--zinc-950);
    transition: transform 0.2s ease;

    &:hover {
      transform: scale(1.05);
    }
  }

  &__brand-text {
    font-size: 18px;
    font-weight: 700;
    color: var(--text-primary);
    letter-spacing: -0.02em;
  }

  &__center {
    flex: 1;
    display: flex;
    justify-content: center;
    padding: 0 24px;
    max-width: 480px;
    margin: 0 auto;

    @media (max-width: 768px) {
      display: none;
    }
  }

  &__right {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-left: auto;

    @media (max-width: 768px) {
      gap: 4px;
    }
  }

  &__nav {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  &__nav-link {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    color: var(--text-secondary);
    text-decoration: none;
    border-radius: var(--radius-sm);
    font-size: 14px;
    font-weight: 500;
    transition: all 0.15s ease;
    background: none;
    border: none;
    cursor: pointer;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }

    &--active {
      color: var(--accent);

      &:hover {
        color: var(--accent-hover);
        background: var(--accent-bg);
      }
    }
  }

  &__nav-label {
    @media (max-width: 1024px) {
      display: none;
    }
  }

  &__dropdown {
    position: relative;

    &:hover .topbar__dropdown-menu {
      opacity: 1;
      visibility: visible;
      transform: translateY(0);
    }
  }

  &__dropdown-trigger {
    .q-icon:last-child {
      transition: transform 0.2s ease;
    }

    &:hover .q-icon:last-child {
      transform: rotate(180deg);
    }
  }

  &__dropdown-menu {
    position: absolute;
    top: 100%;
    left: 50%;
    transform: translateX(-50%) translateY(4px);
    min-width: 200px;
    padding: 6px;
    background: var(--bg-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-md);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
    opacity: 0;
    visibility: hidden;
    transition: all 0.2s ease;
    z-index: 100;
  }

  &__dropdown-item {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 12px;
    color: var(--text-secondary);
    text-decoration: none;
    border-radius: var(--radius-sm);
    font-size: 14px;
    transition: all 0.12s ease;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-primary);
    }
  }

  &__dropdown-divider {
    height: 1px;
    background: var(--border-color);
    margin: 6px 0;
  }

  &__divider {
    width: 1px;
    height: 24px;
    background: var(--border-color);
    margin: 0 8px;

    @media (max-width: 768px) {
      display: none;
    }
  }

  &__actions {
    display: flex;
    align-items: center;
    gap: 4px;

    @media (max-width: 768px) {
      gap: 2px;
    }
  }

  &__action-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 38px;
    height: 38px;
    color: var(--text-secondary);
    border-radius: 50%;
    text-decoration: none;
    transition: all 0.15s ease;
    background: none;
    border: none;
    cursor: pointer;
    position: relative;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }
  }

  &__notification-btn {
    position: relative;
  }

  &__badge {
    position: absolute;
    top: 4px;
    right: 4px;
    min-width: 16px;
    height: 16px;
    padding: 0 4px;
    background: var(--negative);
    color: white;
    font-size: 10px;
    font-weight: 600;
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  &__avatar {
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
  }

  &__user-info {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  &__user-details {
    display: flex;
    flex-direction: column;
    min-width: 0;
  }

  &__user-name {
    font-size: 14px;
    font-weight: 600;
    color: var(--text-primary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &__user-handle {
    font-size: 12px;
    color: var(--text-muted);
  }
}
</style>
