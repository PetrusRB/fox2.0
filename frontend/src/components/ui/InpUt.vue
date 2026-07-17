<script setup lang="ts">
import { computed, ref } from "vue";

export interface InpUtProps {
  modelValue?: string;
  placeholder?: string;
  type?: "text" | "email" | "password";
  icon?: string;
  rightIcon?: string;
  label?: string;
  size?: "sm" | "md" | "lg";
  variant?: "rounded" | "default";
  error?: boolean;
  errorMessage?: string;
  disabled?: boolean;
  loading?: boolean;
}

const props = withDefaults(defineProps<InpUtProps>(), {
  modelValue: "",
  placeholder: "",
  type: "text",
  icon: "",
  rightIcon: "",
  label: "",
  size: "md",
  variant: "default",
  error: false,
  errorMessage: "",
  disabled: false,
  loading: false
});

const emit = defineEmits<{
  "update:modelValue": [value: string];
  enter: [value: string];
}>();

const isFocused = ref(false);
const showPassword = ref(false);

const inputType = computed(() => {
  if (props.type === "password" && showPassword.value) return "text";
  return props.type;
});

function onInput(event: Event) {
  const value = (event.target as HTMLInputElement).value;
  emit("update:modelValue", value);
}

function onKeydown(event: KeyboardEvent) {
  if (event.key === "Enter") {
    emit("enter", props.modelValue);
  }
}

function togglePassword() {
  showPassword.value = !showPassword.value;
}
</script>

<template>
  <div
    class="inputos"
    :class="[
      `inputos--${size}`,
      `inputos--${variant}`,
      {
        'inputos--active': isFocused,
        'inputos--error': error,
        'inputos--disabled': disabled
      }
    ]"
  >
    <label v-if="label" class="inputos__label">{{ label }}</label>

    <div class="inputos__wrapper">
      <q-icon v-if="icon" :name="icon" size="18px" class="inputos__icon" />

      <input
        :value="modelValue"
        :type="inputType"
        :placeholder="placeholder"
        :disabled="disabled"
        class="inputos__field"
        @input="onInput"
        @focus="isFocused = true"
        @blur="isFocused = false"
        @keydown="onKeydown"
      />

      <button
        v-if="type === 'password'"
        type="button"
        class="inputos__action"
        @click="togglePassword"
      >
        <q-icon
          :name="showPassword ? 'visibility_off' : 'visibility'"
          size="18px"
        />
      </button>

      <button
        v-else-if="rightIcon && modelValue"
        type="button"
        class="inputos__action"
        @click="emit('update:modelValue', '')"
      >
        <q-icon :name="rightIcon" size="16px" />
      </button>

      <q-spinner v-if="loading" size="18px" class="inputos__spinner" />
    </div>

    <span v-if="errorMessage" class="inputos__error">{{ errorMessage }}</span>
  </div>
</template>

<style lang="scss" scoped>
.inputos {
  display: flex;
  flex-direction: column;
  gap: 6px;
  width: 100%;

  // ── Sizes ────────────────────────────────────────
  &--sm {
    .inputos__wrapper {
      height: 38px;
      padding: 0 12px;
      font-size: 13px;
    }
  }

  &--md {
    .inputos__wrapper {
      height: 44px;
      padding: 0 14px;
      font-size: 14px;
    }
  }

  &--lg {
    .inputos__wrapper {
      height: 50px;
      padding: 0 16px;
      font-size: 15px;
    }
  }

  // ── Variants ─────────────────────────────────────
  &--rounded .inputos__wrapper {
    border-radius: 20px;
  }

  &--default .inputos__wrapper {
    border-radius: var(--radius-md);
  }

  // ── States ───────────────────────────────────────
  &--active .inputos__wrapper {
    border-color: var(--accent);
    background: var(--bg-primary);
    box-shadow: 0 0 0 3px rgba(101, 163, 13, 0.1);
  }

  &--error .inputos__wrapper {
    border-color: var(--negative);

    &:focus-within {
      box-shadow: 0 0 0 3px rgba(239, 68, 68, 0.1);
    }
  }

  &--disabled {
    opacity: 0.6;
    pointer-events: none;
  }
}

.inputos__label {
  font-size: 14px;
  font-weight: 500;
  color: var(--text-primary);
}

.inputos__wrapper {
  display: flex;
  align-items: center;
  gap: 10px;
  width: 100%;
  background: var(--bg-elevated);
  border: 1px solid var(--border-color);
  transition: all 0.2s ease;
}

.inputos__icon {
  color: var(--text-muted);
  flex-shrink: 0;
}

.inputos__field {
  flex: 1;
  background: none;
  border: none;
  outline: none;
  color: var(--text-primary);
  height: 100%;
  font-size: inherit;

  &::placeholder {
    color: var(--text-muted);
  }

  &:disabled {
    cursor: not-allowed;
  }
}

.inputos__action {
  display: flex;
  align-items: center;
  justify-content: center;
  background: none;
  border: none;
  color: var(--text-muted);
  cursor: pointer;
  padding: 4px;
  flex-shrink: 0;
  transition: color 0.15s ease;

  &:hover {
    color: var(--text-primary);
  }
}

.inputos__spinner {
  color: var(--accent);
  flex-shrink: 0;
}

.inputos__error {
  font-size: 12px;
  color: var(--negative);
}
</style>
