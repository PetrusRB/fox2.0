<script setup lang="ts">
import { ref, computed, onMounted, watch } from "vue";
import { useRoute } from "vue-router";
import { getUserByHandle, listUserPosts } from "@/composables/useSocialClient";
import { useAuth } from "@/composables/useAuth";
import { useProfileData } from "@/composables/useProfileData";
import PosT from "@/components/post/PosT.vue";

const route = useRoute();
const { user: currentUser } = useAuth();

const handle = computed(() => {
  const raw = route.params as Record<string, string | string[] | undefined>;
  const h = Array.isArray(raw.handle) ? raw.handle[0] : raw.handle;
  return h?.replace(/^@/, "") ?? "";
});

const { profile, posts, isLoading, isLoadingPosts, error } =
  useProfileData(handle);

const activeTab = ref<"posts" | "about">("posts");

const isOwnProfile = computed(
  () =>
    !!currentUser.value &&
    !!profile.value &&
    currentUser.value.id === profile.value.id,
);

const initials = computed(() => {
  const name = profile.value?.displayName || "U";
  return name
    .split(" ")
    .map((n) => n[0])
    .join("")
    .toUpperCase()
    .slice(0, 2);
});

const formattedDate = computed(() => {
  if (!profile.value?.createdAt) return "";
  const date = new Date(profile.value.createdAt);
  return date.toLocaleDateString("pt-BR", { month: "long", year: "numeric" });
});

const stats = computed(() => [
  { label: "Posts", value: profile.value?.postsCount ?? 0 },
  { label: "Seguidores", value: profile.value?.followersCount ?? 0 },
  { label: "Seguindo", value: profile.value?.followingCount ?? 0 },
]);
</script>

<template>
  <q-page>
    <div class="profile-page">
      <!-- Loading -->
      <div v-if="isLoading" class="profile-page__loading">
        <q-spinner-dots size="40px" color="accent" />
      </div>

      <!-- Error -->
      <div v-else-if="error" class="profile-page__error">
        <div class="profile-page__error-icon">
          <q-icon name="person_off" size="64px" />
        </div>
        <h2 class="profile-page__error-title">{{ error }}</h2>
        <p class="profile-page__error-text">
          O perfil que você procura não existe ou foi removido.
        </p>
        <router-link to="/" class="profile-page__error-btn">
          <q-icon name="home" size="18px" />
          <span>Voltar ao início</span>
        </router-link>
      </div>

      <!-- Profile -->
      <template v-else-if="profile">
        <!-- Header -->
        <div class="profile-header">
          <div class="profile-header__bg" />

          <div class="profile-header__content">
            <div class="profile-header__avatar">
              <img
                v-if="profile.avatar"
                :src="profile.avatar"
                :alt="profile.displayName"
              />
              <span v-else class="profile-header__initials">{{
                initials
              }}</span>
            </div>

            <div class="profile-header__info">
              <div class="profile-header__name-row">
                <h1 class="profile-header__name">{{ profile.displayName }}</h1>
                <span v-if="isOwnProfile" class="profile-header__badge">
                  <q-icon name="verified" size="16px" />
                  Você
                </span>
              </div>
              <span class="profile-header__handle"
                >@{{ profile.handle || profile.username }}</span
              >
              <p v-if="profile.bio" class="profile-header__bio">
                {{ profile.bio }}
              </p>
              <span v-if="formattedDate" class="profile-header__date">
                <q-icon name="calendar_today" size="14px" />
                Membro desde {{ formattedDate }}
              </span>
            </div>
          </div>

          <!-- Stats -->
          <div class="profile-stats">
            <div
              v-for="stat in stats"
              :key="stat.label"
              class="profile-stats__item"
            >
              <span class="profile-stats__value">{{ stat.value }}</span>
              <span class="profile-stats__label">{{ stat.label }}</span>
            </div>
          </div>
        </div>

        <!-- Tabs -->
        <div class="profile-tabs">
          <button
            class="profile-tabs__btn"
            :class="{ 'profile-tabs__btn--active': activeTab === 'posts' }"
            @click="activeTab = 'posts'"
          >
            <q-icon name="article" size="18px" />
            Posts
          </button>
          <button
            class="profile-tabs__btn"
            :class="{ 'profile-tabs__btn--active': activeTab === 'about' }"
            @click="activeTab = 'about'"
          >
            <q-icon name="info" size="18px" />
            Sobre
          </button>
        </div>

        <!-- Posts Tab -->
        <div v-if="activeTab === 'posts'" class="profile-posts">
          <div v-if="isLoadingPosts" class="profile-posts__loading">
            <q-spinner-dots size="32px" color="accent" />
          </div>

          <template v-else>
            <PosT v-for="post in posts" :key="post.id" :post="post" />

            <div v-if="posts.length === 0" class="profile-posts__empty">
              <q-icon name="article" size="48px" />
              <p>Nenhum post ainda</p>
            </div>
          </template>
        </div>

        <!-- About Tab -->
        <div v-if="activeTab === 'about'" class="profile-about">
          <div class="profile-about__card">
            <h3 class="profile-about__title">
              <q-icon name="person" size="20px" />
              Sobre {{ profile.displayName }}
            </h3>
            <p v-if="profile.bio" class="profile-about__bio">
              {{ profile.bio }}
            </p>
            <p v-else class="profile-about__bio profile-about__bio--muted">
              Este usuário ainda não escreveu uma bio.
            </p>
          </div>

          <div class="profile-about__card">
            <h3 class="profile-about__title">
              <q-icon name="bar_chart" size="20px" />
              Estatísticas
            </h3>
            <div class="profile-about__stats">
              <div class="profile-about__stat">
                <q-icon name="article" size="18px" class="text-accent" />
                <span class="profile-about__stat-value">{{
                  profile.postsCount
                }}</span>
                <span class="profile-about__stat-label">posts</span>
              </div>
              <div class="profile-about__stat">
                <q-icon name="group" size="18px" class="text-accent" />
                <span class="profile-about__stat-value">{{
                  profile.followersCount
                }}</span>
                <span class="profile-about__stat-label">seguidores</span>
              </div>
              <div class="profile-about__stat">
                <q-icon name="group_add" size="18px" class="text-accent" />
                <span class="profile-about__stat-value">{{
                  profile.followingCount
                }}</span>
                <span class="profile-about__stat-label">seguindo</span>
              </div>
            </div>
          </div>
        </div>
      </template>
    </div>
  </q-page>
</template>

<style lang="scss" scoped>
.profile-page {
  max-width: 680px;
  margin: 0 auto;
  padding: 20px 16px;
  min-height: calc(100vh - var(--navbar-height));

  &__loading {
    display: flex;
    justify-content: center;
    padding: 80px 0;
  }

  &__error {
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    padding: 80px 24px;
  }

  &__error-icon {
    color: var(--text-muted);
    margin-bottom: 24px;
    opacity: 0.5;
  }

  &__error-title {
    font-size: 24px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 8px;
  }

  &__error-text {
    font-size: 15px;
    color: var(--text-secondary);
    margin-bottom: 32px;
    max-width: 400px;
  }

  &__error-btn {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 12px 24px;
    background: var(--accent);
    color: var(--zinc-950);
    font-size: 14px;
    font-weight: 600;
    border-radius: var(--radius-md);
    text-decoration: none;
    transition: all 0.2s ease;

    &:hover {
      background: var(--accent-hover);
      transform: translateY(-1px);
      box-shadow: 0 4px 12px rgba(101, 163, 13, 0.3);
    }
  }
}

/* ── Header ─────────────────────────────────────── */
.profile-header {
  background: var(--bg-surface);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  overflow: hidden;
  margin-bottom: 16px;

  &__bg {
    height: 120px;
    background: linear-gradient(
      135deg,
      var(--accent) 0%,
      var(--avocado-700) 50%,
      var(--zinc-800) 100%
    );
    position: relative;

    &::after {
      content: "";
      position: absolute;
      inset: 0;
      background: url("data:image/svg+xml,%3Csvg width='60' height='60' viewBox='0 0 60 60' xmlns='http://www.w3.org/2000/svg'%3E%3Cg fill='none' fill-rule='evenodd'%3E%3Cg fill='%23ffffff' fill-opacity='0.05'%3E%3Cpath d='M36 34v-4h-2v4h-4v2h4v4h2v-4h4v-2h-4zm0-30V0h-2v4h-4v2h4v4h2V6h4V4h-4zM6 34v-4H4v4H0v2h4v4h2v-4h4v-2H6zM6 4V0H4v4H0v2h4v4h2V6h4V4H6z'/%3E%3C/g%3E%3C/g%3E%3C/svg%3E");
      opacity: 0.4;
    }
  }

  &__content {
    display: flex;
    gap: 20px;
    padding: 0 24px;
    margin-top: -40px;
    position: relative;
    z-index: 1;

    @media (max-width: 520px) {
      flex-direction: column;
      align-items: center;
      text-align: center;
      padding: 0 16px;
    }
  }

  &__avatar {
    width: 100px;
    height: 100px;
    border-radius: 50%;
    border: 4px solid var(--bg-surface);
    background: linear-gradient(135deg, var(--accent), var(--avocado-500));
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
    overflow: hidden;
    margin-top: -20px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.4);

    img {
      width: 100%;
      height: 100%;
      object-fit: cover;
    }
  }

  &__initials {
    font-size: 36px;
    font-weight: 700;
    color: var(--zinc-950);
  }

  &__info {
    flex: 1;
    padding-top: 48px;
    min-width: 0;

    @media (max-width: 520px) {
      padding-top: 8px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
  }

  &__name-row {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;

    @media (max-width: 520px) {
      justify-content: center;
    }
  }

  &__name {
    font-size: 22px;
    font-weight: 700;
    color: var(--text-primary);
    line-height: 1.3;
  }

  &__badge {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 3px 10px;
    background: var(--accent-bg);
    color: var(--accent);
    font-size: 12px;
    font-weight: 600;
    border-radius: 999px;
    border: 1px solid rgba(101, 163, 13, 0.2);
  }

  &__handle {
    display: block;
    font-size: 14px;
    color: var(--text-muted);
    margin-top: 2px;
  }

  &__bio {
    font-size: 15px;
    color: var(--text-secondary);
    line-height: 1.6;
    margin-top: 12px;
    max-width: 500px;
  }

  &__date {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 13px;
    color: var(--text-muted);
    margin-top: 12px;
  }
}

/* ── Stats ──────────────────────────────────────── */
.profile-stats {
  display: flex;
  justify-content: center;
  gap: 32px;
  padding: 20px 24px;
  margin-top: 16px;
  border-top: 1px solid var(--border-color);

  @media (max-width: 400px) {
    gap: 20px;
    padding: 16px;
  }

  &__item {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    cursor: default;
  }

  &__value {
    font-size: 20px;
    font-weight: 700;
    color: var(--text-primary);
  }

  &__label {
    font-size: 13px;
    color: var(--text-muted);
  }
}

/* ── Tabs ───────────────────────────────────────── */
.profile-tabs {
  display: flex;
  gap: 4px;
  padding: 4px;
  background: var(--bg-surface);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  margin-bottom: 16px;

  &__btn {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px 16px;
    background: transparent;
    border: none;
    border-radius: var(--radius-md);
    color: var(--text-muted);
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.2s ease;

    &:hover {
      color: var(--text-secondary);
      background: var(--bg-elevated);
    }

    &--active {
      color: var(--accent);
      background: var(--accent-bg);

      &:hover {
        color: var(--accent);
        background: var(--accent-bg);
      }
    }
  }
}

/* ── Posts ──────────────────────────────────────── */
.profile-posts {
  display: flex;
  flex-direction: column;
  gap: 12px;

  &__loading {
    display: flex;
    justify-content: center;
    padding: 48px 0;
  }

  &__empty {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 12px;
    padding: 64px 24px;
    color: var(--text-muted);
    background: var(--bg-surface);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-lg);

    p {
      font-size: 15px;
    }
  }
}

/* ── About ──────────────────────────────────────── */
.profile-about {
  display: flex;
  flex-direction: column;
  gap: 12px;

  &__card {
    background: var(--bg-surface);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-lg);
    padding: 24px;
  }

  &__title {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 16px;
    font-weight: 600;
    color: var(--text-primary);
    margin-bottom: 16px;
  }

  &__bio {
    font-size: 15px;
    color: var(--text-secondary);
    line-height: 1.7;

    &--muted {
      color: var(--text-muted);
      font-style: italic;
    }
  }

  &__stats {
    display: flex;
    gap: 32px;

    @media (max-width: 400px) {
      gap: 20px;
      flex-wrap: wrap;
    }
  }

  &__stat {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  &__stat-value {
    font-size: 18px;
    font-weight: 700;
    color: var(--text-primary);
  }

  &__stat-label {
    font-size: 14px;
    color: var(--text-muted);
  }
}
</style>
