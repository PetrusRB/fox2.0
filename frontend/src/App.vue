<script setup lang="ts">
import { onMounted } from "vue";
import { useAuth } from "@/composables/useAuth";
import LoginForm from "@/components/auth/LoginForm.vue";
import BuTTon from "@/components/ui/BuTTon.vue";
import MainLayout from "@/layouts/MainLayout.vue";

import { mdiForum } from "@quasar/extras/mdi-v7";

const {
  isAuthenticated,
  restoreSession,
  handleOAuthCallback,
  isLoading,
  error,
  clearError
} = useAuth();

onMounted(async () => {
  const params = new URLSearchParams(window.location.search);
  const code = params.get("code");

  if (code) {
    const ok = await handleOAuthCallback(code);
    if (ok) {
      window.history.replaceState({}, "", window.location.pathname);
    }
    return;
  }

  await restoreSession();
});
</script>

<template>
  <template v-if="isLoading && !isAuthenticated">
    <div class="auth-page">
      <div class="auth-container">
        <div class="auth-branding">
          <div class="auth-branding__content">
            <div class="auth-branding__logo">
              <q-icon name="pets" size="48px" />
            </div>
            <h1 class="auth-branding__title">Fox</h1>
            <p class="auth-branding__subtitle">Entrando...</p>
            <q-spinner-dots size="40px" color="accent" />
          </div>
        </div>
        <div class="auth-form-wrapper">
          <q-spinner-dots size="40px" color="accent" />
        </div>
      </div>
    </div>
  </template>

  <template v-else-if="isAuthenticated">
    <MainLayout />
  </template>

  <template v-else>
    <div class="auth-page">
      <div class="auth-container">
        <!-- Left Side - Branding -->
        <div class="auth-branding">
          <div class="auth-branding__content">
            <div class="auth-branding__logo">
              <q-icon name="pets" size="48px" />
            </div>
            <h1 class="auth-branding__title">Fox</h1>
            <p class="auth-branding__subtitle">
              Uma rede social com foco na comunidade gamer.
            </p>

            <div class="auth-branding__features">
              <div class="auth-feature">
                <q-icon name="lightbulb" size="24px" />
                <span>Expresse ideias</span>
              </div>
              <div class="auth-feature">
                <q-icon :name="mdiForum" size="24px" />
                <span>Crie ou junte-se comunidades</span>
              </div>
              <div class="auth-feature">
                <q-icon name="group" size="24px" />
                <span>Socialize</span>
              </div>
            </div>
          </div>

          <div class="auth-branding__footer">
            <span>&copy; 2026 Fox. Todos os direitos reservados.</span>
          </div>
        </div>

        <!-- Right Side - Auth Forms -->
        <div class="auth-form-wrapper">
          <Transition name="fade" mode="out-in">
            <div v-if="error" class="auth-error" :key="'error'">
              <q-icon
                name="error_outline"
                size="48px"
                class="auth-error__icon"
              />
              <p class="auth-error__message">{{ error.message }}</p>
              <p v-if="error.details" class="auth-error__details">{{
                error.details
              }}</p>
              <BuTTon variant="primary" size="lg" @click="clearError">
                Tentar novamente
              </BuTTon>
            </div>
            <LoginForm v-else key="login" />
          </Transition>
        </div>
      </div>
    </div>
  </template>
</template>

<style lang="scss" scoped>
.auth-page {
  min-height: 100vh;
  background: var(--bg-primary);
}

.auth-container {
  display: flex;
  min-height: 100vh;

  @media (max-width: 1024px) {
    flex-direction: column;
  }
}

.auth-branding {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 48px;
  background: linear-gradient(135deg, var(--zinc-900) 0%, var(--zinc-950) 100%);
  border-right: 1px solid var(--border-color);
  position: relative;
  overflow: hidden;

  &::before {
    content: "";
    position: absolute;
    top: -50%;
    left: -50%;
    width: 200%;
    height: 200%;
    background: radial-gradient(
      circle at 30% 50%,
      rgba(101, 163, 13, 0.08) 0%,
      transparent 50%
    );
    pointer-events: none;
  }

  @media (max-width: 1024px) {
    flex: none;
    padding: 48px 24px;
    border-right: none;
    border-bottom: 1px solid var(--border-color);
  }

  &__content {
    position: relative;
    z-index: 1;
    text-align: center;
    max-width: 400px;
  }

  &__logo {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 80px;
    height: 80px;
    background: var(--accent);
    border-radius: 20px;
    color: var(--zinc-950);
    margin: 0 auto 24px;
    animation: float 3s ease-in-out infinite;
  }

  &__title {
    font-size: 40px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 12px;
    letter-spacing: -0.02em;
  }

  &__subtitle {
    font-size: 18px;
    color: var(--text-secondary);
    line-height: 1.6;
    margin-bottom: 48px;
  }

  &__features {
    display: flex;
    flex-direction: column;
    gap: 20px;
    text-align: left;
  }

  &__footer {
    position: absolute;
    bottom: 24px;
    left: 0;
    right: 0;
    text-align: center;
    font-size: 13px;
    color: var(--text-muted);
  }
}

.auth-feature {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px 20px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-md);
  color: var(--text-secondary);
  transition: all 0.2s ease;

  &:hover {
    background: rgba(255, 255, 255, 0.05);
    border-color: var(--accent);
    color: var(--text-primary);
  }

  .q-icon {
    color: var(--accent);
    flex-shrink: 0;
  }

  span {
    font-size: 15px;
    font-weight: 500;
  }
}

.auth-form-wrapper {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 48px;

  @media (max-width: 1024px) {
    padding: 32px 24px;
  }
}

.auth-error {
  text-align: center;
  max-width: 380px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;

  &__icon {
    color: var(--negative);
  }

  &__message {
    font-size: 18px;
    font-weight: 600;
    color: var(--text-primary);
  }

  &__details {
    font-size: 13px;
    color: var(--text-muted);
    word-break: break-word;
  }
}

@keyframes float {
  0%,
  100% {
    transform: translateY(0);
  }
  50% {
    transform: translateY(-8px);
  }
}

.fade-enter-active,
.fade-leave-active {
  transition:
    opacity 0.2s ease,
    transform 0.2s ease;
}

.fade-enter-from {
  opacity: 0;
  transform: translateY(10px);
}

.fade-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}
</style>
