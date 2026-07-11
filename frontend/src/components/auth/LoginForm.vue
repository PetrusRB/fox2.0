<script setup lang="ts">
import { useAuth } from "@/composables/useAuth";
import BuTTon from "@/components/ui/BuTTon.vue";

import {
  fabGoogle,
  fabGithub,
  fabDiscord,
} from "@quasar/extras/fontawesome-v7";
//const emit = defineEmits<{
// switchToRegister: [];
// }>();

const { initiateGoogleLogin, isLoading } = useAuth();
</script>

<template>
  <div class="auth-card">
    <div class="auth-card__header">
      <div class="auth-card__logo">
        <q-icon name="pets" size="32px" />
      </div>
      <h2 class="auth-card__title">Bem-vindo de volta</h2>
      <p class="auth-card__subtitle">
        Entre em alguma conta com provedor para começar a sua jornada
      </p>
    </div>

    <form class="auth-card__form" @submit.prevent="initiateGoogleLogin">
      <BuTTon
        type="submit"
        variant="primary"
        size="lg"
        block
        :icon="fabGoogle"
        :loading="isLoading"
      >
        Entrar com Google
      </BuTTon>
    </form>

    <div class="auth-card__divider">
      <span>ou continue com</span>
    </div>

    <div class="auth-social">
      <BuTTon variant="secondary" size="lg" :icon="fabGithub" />
      <BuTTon variant="secondary" size="lg" :icon="fabDiscord" />
    </div>

    <!-- <p class="auth-card__footer"> -->
    <!--   Não tem uma conta? -->
    <!--   <BuTTon variant="ghost" @click="emit('switchToRegister')"> -->
    <!--     Cadastre-se -->
    <!--   </BuTTon> -->
    <!-- </p> -->
  </div>
</template>

<style lang="scss" scoped>
.auth-card {
  width: 100%;
  max-width: 420px;
  padding: 40px;
  background: var(--bg-surface);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);

  &__header {
    text-align: center;
    margin-bottom: 32px;
  }

  &__logo {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 64px;
    height: 64px;
    background: var(--accent);
    border-radius: 16px;
    color: var(--zinc-950);
    margin: 0 auto 20px;
    animation: float 3s ease-in-out infinite;
  }

  &__title {
    font-size: 24px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 8px;
    letter-spacing: -0.02em;
  }

  &__subtitle {
    font-size: 14px;
    color: var(--text-secondary);
  }

  &__form {
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  &__options {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  &__forgot {
    font-size: 13px;
    color: var(--accent);
    text-decoration: none;

    &:hover {
      color: var(--accent-hover);
    }
  }

  &__divider {
    display: flex;
    align-items: center;
    gap: 16px;
    margin: 24px 0;

    &::before,
    &::after {
      content: "";
      flex: 1;
      height: 1px;
      background: var(--border-color);
    }

    span {
      font-size: 13px;
      color: var(--text-muted);
      white-space: nowrap;
    }
  }

  &__footer {
    text-align: center;
    font-size: 14px;
    color: var(--text-secondary);
    margin-top: 24px;
  }

  &__link {
    color: var(--accent);
    text-decoration: none;
    font-weight: 500;
    background: none;
    border: none;
    cursor: pointer;
    font-size: 14px;

    &:hover {
      color: var(--accent-hover);
    }
  }
}

.auth-checkbox {
  display: flex;
  align-items: center;
  gap: 10px;
  cursor: pointer;

  &__input {
    display: none;

    &:checked + .auth-checkbox__box {
      background: var(--accent);
      border-color: var(--accent);

      &::after {
        opacity: 1;
        transform: scale(1);
      }
    }
  }

  &__box {
    width: 18px;
    height: 18px;
    border: 2px solid var(--border-color);
    border-radius: 4px;
    position: relative;
    transition: all 0.2s ease;

    &::after {
      content: "";
      position: absolute;
      left: 5px;
      top: 1px;
      width: 5px;
      height: 10px;
      border: solid var(--zinc-950);
      border-width: 0 2px 2px 0;
      transform: scale(0);
      opacity: 0;
      transition: all 0.2s ease;
    }
  }

  &__label {
    font-size: 14px;
    color: var(--text-secondary);
  }
}

.auth-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 50px;
  font-size: 15px;
  font-weight: 600;
  background: var(--accent);
  color: var(--zinc-950);
  border: none;
  border-radius: var(--radius-md);
  cursor: pointer;
  transition: all 0.2s ease;

  &:hover:not(:disabled) {
    background: var(--accent-hover);
    transform: translateY(-1px);
    box-shadow: 0 4px 12px rgba(101, 163, 13, 0.3);
  }

  &:disabled {
    opacity: 0.7;
    cursor: not-allowed;
  }
}

.auth-social {
  display: flex;
  justify-content: center;
  gap: 16px;

  &__btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 52px;
    height: 52px;
    background: var(--bg-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-md);
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.2s ease;

    &:hover {
      background: var(--bg-primary);
      border-color: var(--text-muted);
      color: var(--text-primary);
      transform: translateY(-2px);
    }
  }
}

@keyframes float {
  0%,
  100% {
    transform: translateY(0);
  }
  50% {
    transform: translateY(-6px);
  }
}
</style>
