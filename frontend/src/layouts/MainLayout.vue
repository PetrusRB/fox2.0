<script setup lang="ts">
import { ref, computed } from "vue";
import { useQuasar } from "quasar";
import NavBar, { type NavItem, type NavAction } from "@/components/NavBar.vue";
import SidePanel, { type SidebarItem } from "@/components/SidePanel.vue";
import { Avatar } from "@/components/ui";
import { useAuth } from "@/composables/useAuth";
const { user } = useAuth();

const $q = useQuasar();
const drawerOpen = ref(false);
const sidebarCollapsed = ref(false);

const isMobile = computed(() => $q.screen.lt.md);

const navItems: NavItem[] = [
  { key: "home", label: "Home", icon: "home", to: "/" },
  { key: "explore", label: "Explorar", icon: "explore", to: "/explore" },
  {
    key: "notifications",
    label: "Notificações",
    icon: "notifications",
    to: "/notifications"
  },
  { key: "chat", label: "Chat", icon: "chat", to: "/chat" }
];

const navActions: NavAction[] = [
  { key: "create", icon: "add_circle", tooltip: "Criar publicação" }
];

const sidebarItems: SidebarItem[] = [
  {
    key: "home",
    label: "Página Inicial",
    icon: "home",
    to: "/",
    section: "Principal"
  },
  { key: "explore", label: "Explorar", icon: "explore", to: "/explore" },
  {
    key: "notifications",
    label: "Notificações",
    icon: "notifications",
    to: "/notifications",
    badge: 5
  },
  {
    key: "communities",
    label: "Comunidades",
    icon: "diversity_1",
    to: "/messages"
  },
  {
    key: "messages",
    label: "Mensagens",
    icon: "chat_bubble",
    to: "/messages",
    badge: 12
  },
  {
    key: "bookmarks",
    label: "Favoritos",
    icon: "bookmark",
    to: "/bookmarks",
    section: "Coleções"
  },
  { key: "lists", label: "Listas", icon: "list", to: "/lists" },
  { key: "groups", label: "Grupos", icon: "group", to: "/groups" },
  {
    key: "profile",
    label: "Meu Perfil",
    icon: "person",
    to: "/profile",
    section: "Conta"
  },
  {
    key: "settings",
    label: "Configurações",
    icon: "settings",
    to: "/settings"
  }
];

function toggleDrawer() {
  if (isMobile.value) {
    drawerOpen.value = !drawerOpen.value;
  } else {
    sidebarCollapsed.value = !sidebarCollapsed.value;
  }
}
</script>

<template>
  <q-layout view="lHh Lpr lFf">
    <NavBar
      :items="navItems"
      :actions="navActions"
      @toggle-drawer="toggleDrawer"
    />

    <SidePanel
      :items="sidebarItems"
      :collapsed="sidebarCollapsed"
      @toggle-collapse="sidebarCollapsed = !sidebarCollapsed"
    />

    <div
      v-if="isMobile && drawerOpen"
      class="mobile-overlay"
      @click="drawerOpen = false"
    />

    <aside
      v-if="isMobile"
      class="mobile-sidebar"
      :class="{ 'mobile-sidebar--open': drawerOpen }"
    >
      <div class="mobile-sidebar__header">
        <div class="mobile-sidebar__profile">
          <Avatar :src="user?.avatar" size="lg" icon="person" />
          <div class="mobile-sidebar__info">
            <span class="mobile-sidebar__name">{{ user?.name }}</span>
            <span class="mobile-sidebar__handle">@{{ user?.handle }}</span>
          </div>
        </div>
        <button class="mobile-sidebar__close" @click="drawerOpen = false">
          <q-icon name="close" size="20px" />
        </button>
      </div>

      <nav class="mobile-sidebar__nav">
        <template v-for="item in sidebarItems" :key="item.key">
          <div v-if="item.section" class="mobile-sidebar__section">
            {{ item.section }}
          </div>
          <a
            :href="item.to || item.href"
            class="mobile-sidebar__item"
            @click="drawerOpen = false"
          >
            <q-icon :name="item.icon" size="20px" />
            <span>{{ item.label }}</span>
            <span v-if="item.badge" class="mobile-sidebar__badge">
              {{ item.badge > 99 ? "99+" : item.badge }}
            </span>
          </a>
        </template>
      </nav>
    </aside>

    <q-page-container>
      <router-view />
    </q-page-container>
  </q-layout>
</template>

<style lang="scss" scoped>
.mobile-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.6);
  z-index: 950;
  animation: fadeIn 0.2s ease;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.mobile-sidebar {
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  width: 280px;
  max-width: 85vw;
  background: var(--bg-surface);
  z-index: 1000;
  transform: translateX(-100%);
  transition: transform 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  display: flex;
  flex-direction: column;

  &--open {
    transform: translateX(0);
  }

  &__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 16px;
    border-bottom: 1px solid var(--border-color);
  }

  &__profile {
    display: flex;
    align-items: center;
    gap: 12px;
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
  }

  &__info {
    display: flex;
    flex-direction: column;
  }

  &__name {
    font-size: 15px;
    font-weight: 600;
    color: var(--text-primary);
  }

  &__handle {
    font-size: 13px;
    color: var(--text-muted);
  }

  &__close {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 36px;
    height: 36px;
    background: var(--bg-elevated);
    border: none;
    border-radius: 50%;
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      background: var(--border-color);
      color: var(--text-primary);
    }
  }

  &__nav {
    flex: 1;
    overflow-y: auto;
    padding: 8px;
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
    gap: 12px;
    padding: 12px;
    color: var(--text-secondary);
    text-decoration: none;
    border-radius: var(--radius-md);
    font-size: 14px;
    font-weight: 500;
    transition: all 0.15s ease;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-elevated);
    }

    .q-icon {
      flex-shrink: 0;
    }

    span:not(.mobile-sidebar__badge) {
      flex: 1;
    }
  }

  &__badge {
    min-width: 20px;
    height: 20px;
    padding: 0 6px;
    background: var(--negative);
    color: white;
    font-size: 11px;
    font-weight: 600;
    border-radius: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
  }
}

:deep(.q-page-container) {
  padding-top: var(--navbar-height);
  padding-left: var(--sidebar-width);
  transition: padding-left 0.2s ease;

  @media (max-width: 1023px) {
    padding-left: 0;
  }
}
</style>
