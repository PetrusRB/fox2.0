<script setup lang="ts">
import { ref, computed } from "vue";
import { useAuth } from "@/composables/useAuth";
import InpUt from "@/components/ui/InpUt.vue";
import BuTTon from "@/components/ui/BuTTon.vue";
import {
  fabGoogle,
  fabGithub,
  fabDiscord,
} from "@quasar/extras/fontawesome-v7";

const emit = defineEmits<{
  switchToLogin: [];
}>();

const { register, isLoading } = useAuth();

const name = ref("");
const email = ref("");
const handle = ref("");
const password = ref("");
const confirmPassword = ref("");
const acceptTerms = ref(false);

const passwordStrength = computed(() => {
  const p = password.value;
  if (!p) return { level: 0, label: "", color: "" };
  let score = 0;
  if (p.length >= 8) score++;
  if (/[a-z]/.test(p) && /[A-Z]/.test(p)) score++;
  if (/\d/.test(p)) score++;
  if (/[^a-zA-Z0-9]/.test(p)) score++;

  if (score <= 1) return { level: 1, label: "Fraca", color: "var(--negative)" };
  if (score <= 2) return { level: 2, label: "Média", color: "var(--zinc-400)" };
  if (score <= 3) return { level: 3, label: "Forte", color: "var(--accent)" };
  return { level: 4, label: "Muito forte", color: "var(--accent)" };
});

const isFormValid = computed(() => {
  return (
    name.value &&
    email.value &&
    password.value &&
    confirmPassword.value &&
    password.value === confirmPassword.value &&
    acceptTerms.value
  );
});

const passwordMismatch = computed(() => {
  return !!confirmPassword.value && password.value !== confirmPassword.value;
});

async function handleRegister() {
  if (!isFormValid.value) return;
  await register(name.value, handle.value, email.value, password.value);
}
</script>

<template>
  <div class="auth-card">
    <div class="auth-card__header">
      <div class="auth-card__logo">
        <q-icon name="pets" size="32px" />
      </div>
      <h2 class="auth-card__title">Criar sua conta</h2>
      <p class="auth-card__subtitle">Preencha os dados abaixo para começar</p>
    </div>

    <form class="auth-card__form" @submit.prevent="handleRegister">
      <InpUt
        v-model="name"
        label="Nome completo"
        icon="person"
        placeholder="Seu nome"
        size="lg"
      />

      <InpUt
        v-model="email"
        label="Email"
        type="email"
        icon="mail"
        placeholder="seu@email.com"
        size="lg"
      />

      <div class="auth-field-group">
        <InpUt
          v-model="password"
          label="Senha"
          type="password"
          icon="lock"
          placeholder="Crie uma senha"
          size="lg"
        />
        <div v-if="password" class="auth-password-strength">
          <div class="auth-password-strength__bar">
            <div
              class="auth-password-strength__fill"
              :style="{
                width: `${(passwordStrength.level / 4) * 100}%`,
                background: passwordStrength.color,
              }"
            />
          </div>
          <span
            class="auth-password-strength__label"
            :style="{ color: passwordStrength.color }"
          >
            {{ passwordStrength.label }}
          </span>
        </div>
      </div>

      <InpUt
        v-model="confirmPassword"
        label="Confirmar senha"
        type="password"
        icon="lock"
        placeholder="Confirme sua senha"
        size="lg"
        :error="passwordMismatch"
        :error-message="passwordMismatch ? 'As senhas não coincidem' : ''"
      />

      <label class="auth-checkbox">
        <input
          v-model="acceptTerms"
          type="checkbox"
          class="auth-checkbox__input"
        />
        <span class="auth-checkbox__box" />
        <span class="auth-checkbox__label">
          Li e aceito os
          <a href="#" class="auth-checkbox__link">Termos de Uso</a>
          e a
          <a href="#" class="auth-checkbox__link">Política de Privacidade</a>
        </span>
      </label>

      <BuTTon type="submit" variant="primary" size="lg" block :loading="isLoading" :disabled="!isFormValid">
        Criar conta
      </BuTTon>
    </form>

    <div class="auth-card__divider">
      <span>ou cadastre-se com</span>
    </div>

    <div class="auth-social">
      <BuTTon variant="secondary" size="lg" :icon="fabGoogle" />
      <BuTTon variant="secondary" size="lg" :icon="fabGithub" />
      <BuTTon variant="secondary" size="lg" :icon="fabDiscord" />
    </div>

    <p class="auth-card__footer">
      Já tem uma conta?
      <BuTTon variant="ghost" @click="emit('switchToLogin')">
        Entrar
      </BuTTon>
    </p>
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
    margin-bottom: 28px;
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
    gap: 16px;
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

.auth-field-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.auth-password-strength {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-top: 2px;

  &__bar {
    flex: 1;
    height: 4px;
    background: var(--zinc-700);
    border-radius: 2px;
    overflow: hidden;
  }

  &__fill {
    height: 100%;
    border-radius: 2px;
    transition: all 0.3s ease;
  }

  &__label {
    font-size: 12px;
    font-weight: 500;
    min-width: 70px;
  }
}

.auth-checkbox {
  display: flex;
  align-items: flex-start;
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
    flex-shrink: 0;
    margin-top: 2px;

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
    font-size: 13px;
    color: var(--text-secondary);
    line-height: 1.5;
  }

  &__link {
    color: var(--accent);
    text-decoration: none;

    &:hover {
      text-decoration: underline;
    }
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
  margin-top: 4px;

  &:hover:not(:disabled) {
    background: var(--accent-hover);
    transform: translateY(-1px);
    box-shadow: 0 4px 12px rgba(101, 163, 13, 0.3);
  }

  &:disabled {
    opacity: 0.5;
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
