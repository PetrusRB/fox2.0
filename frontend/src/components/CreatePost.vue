<script setup lang="ts">
import { ref, computed } from "vue";
import { createPost } from "@/composables/useSocialClient";
import { MAX_TITLE_CHARS, MAX_CONTENT_CHARS } from "@/config/config";
import BuTTon from "./ui/BuTTon.vue";
import Avatar from "./ui/Avatar.vue";
import { useAuth } from "@/composables/useAuth";

const emit = defineEmits<{
  created: [];
}>();

const { user } = useAuth();

const title = ref("");
const content = ref("");
const imageUrl = ref("");
const showImageInput = ref(false);
const loading = ref(false);
const error = ref<string | null>(null);
const success = ref(false);

const titleRemaining = computed(() => MAX_TITLE_CHARS - title.value.length);
const contentRemaining = computed(
  () => MAX_CONTENT_CHARS - content.value.length,
);
const titlePercent = computed(
  () => (title.value.length / MAX_TITLE_CHARS) * 100,
);
const contentPercent = computed(
  () => (content.value.length / MAX_CONTENT_CHARS) * 100,
);

const canSubmit = computed(() => {
  return (
    title.value.trim().length > 0 &&
    content.value.trim().length > 0 &&
    !loading.value
  );
});

function getCounterColor(percent: number): string {
  if (percent > 90) return "var(--negative)";
  if (percent > 70) return "#f59e0b";
  return "var(--text-muted)";
}

async function handleSubmit() {
  if (!canSubmit.value) return;

  loading.value = true;
  error.value = null;
  success.value = false;

  try {
    await createPost(
      title.value.trim(),
      content.value.trim(),
      imageUrl.value.trim() || undefined,
    );
    title.value = "";
    content.value = "";
    imageUrl.value = "";
    showImageInput.value = false;
    success.value = true;
    emit("created");
    setTimeout(() => (success.value = false), 3000);
  } catch (e: any) {
    error.value = e?.message || "Erro ao criar post";
  } finally {
    loading.value = false;
  }
}
</script>

<template>
  <article class="create-post">
    <div class="create-post__header">
      <Avatar :src="user?.avatar" size="md" />
      <span class="create-post__label">Criar post</span>
    </div>

    <div class="create-post__body">
      <input
        v-model="title"
        type="text"
        class="create-post__title"
        placeholder="Titulo do post"
        :maxlength="MAX_TITLE_CHARS"
      />

      <textarea
        v-model="content"
        class="create-post__content"
        placeholder="O que esta pensando?"
        rows="3"
        :maxlength="MAX_CONTENT_CHARS"
      />

      <Transition name="slide">
        <div v-if="showImageInput" class="create-post__image-row">
          <q-icon name="link" size="18px" class="create-post__image-icon" />
          <input
            v-model="imageUrl"
            type="url"
            class="create-post__image-input"
            placeholder="URL da imagem (opcional)"
            :maxlength="2048"
          />
          <button
            class="create-post__image-remove"
            @click="
              imageUrl = '';
              showImageInput = false;
            "
          >
            <q-icon name="close" size="16px" />
          </button>
        </div>
      </Transition>
    </div>

    <div class="create-post__footer">
      <div class="create-post__actions-left">
        <button
          class="create-post__action-btn"
          :class="{ 'create-post__action-btn--active': showImageInput }"
          @click="showImageInput = !showImageInput"
        >
          <q-icon name="image" size="20px" />
        </button>
      </div>

      <div class="create-post__meta">
        <span
          v-if="title.length > 0"
          class="create-post__counter"
          :style="{ color: getCounterColor(titlePercent) }"
        >
          {{ titleRemaining }}
        </span>
        <span
          v-if="content.length > 0"
          class="create-post__counter"
          :style="{ color: getCounterColor(contentPercent) }"
        >
          {{ contentRemaining }}
        </span>
      </div>

      <BuTTon
        variant="primary"
        size="sm"
        :loading="loading"
        :disabled="!canSubmit"
        @click="handleSubmit"
      >
        Postar
      </BuTTon>
    </div>

    <Transition name="fade">
      <div v-if="error" class="create-post__alert create-post__alert--error">
        <q-icon name="error_outline" size="16px" />
        {{ error }}
      </div>
    </Transition>

    <Transition name="fade">
      <div
        v-if="success"
        class="create-post__alert create-post__alert--success"
      >
        <q-icon name="check_circle" size="16px" />
        Post criado com sucesso!
      </div>
    </Transition>
  </article>
</template>

<style lang="scss" scoped>
.create-post {
  width: 100%;
  background: var(--bg-surface);
  border: 1px solid var(--border-color);
  border-radius: var(--radius-lg);
  padding: 20px;
  transition: border-color 0.2s ease;

  &:focus-within {
    border-color: var(--accent);
  }

  &__header {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 16px;
  }

  &__label {
    font-size: 15px;
    font-weight: 600;
    color: var(--text-primary);
  }

  &__body {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  &__title {
    width: 100%;
    background: none;
    border: none;
    outline: none;
    color: var(--text-primary);
    font-size: 17px;
    font-weight: 600;
    font-family: inherit;
    padding: 0;

    &::placeholder {
      color: var(--text-muted);
    }
  }

  &__content {
    width: 100%;
    background: none;
    border: none;
    outline: none;
    color: var(--text-primary);
    font-size: 15px;
    font-family: inherit;
    line-height: 1.6;
    resize: none;
    min-height: 60px;
    padding: 0;

    &::placeholder {
      color: var(--text-muted);
    }
  }

  &__image-row {
    display: flex;
    align-items: center;
    gap: 10px;
    background: var(--bg-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    padding: 0 12px;
    height: 42px;
    transition: border-color 0.2s ease;

    &:focus-within {
      border-color: var(--accent);
    }
  }

  &__image-icon {
    color: var(--text-muted);
    flex-shrink: 0;
  }

  &__image-input {
    flex: 1;
    background: none;
    border: none;
    outline: none;
    color: var(--text-primary);
    font-size: 13px;
    font-family: inherit;
    min-width: 0;

    &::placeholder {
      color: var(--text-muted);
    }
  }

  &__image-remove {
    display: flex;
    align-items: center;
    justify-content: center;
    background: none;
    border: none;
    color: var(--text-muted);
    cursor: pointer;
    padding: 4px;
    flex-shrink: 0;
    border-radius: var(--radius-sm);
    transition: all 0.15s ease;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-primary);
    }
  }

  &__footer {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 16px;
    padding-top: 16px;
    border-top: 1px solid var(--border-color);
  }

  &__actions-left {
    display: flex;
    gap: 4px;
  }

  &__action-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 36px;
    height: 36px;
    background: none;
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    color: var(--text-muted);
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      color: var(--accent);
      border-color: var(--accent);
      background: var(--accent-bg);
    }

    &--active {
      color: var(--accent);
      border-color: var(--accent);
      background: var(--accent-bg);
    }
  }

  &__meta {
    display: flex;
    gap: 8px;
    margin-left: auto;
    margin-right: 12px;
  }

  &__counter {
    font-size: 12px;
    font-variant-numeric: tabular-nums;
    min-width: 28px;
    text-align: right;
  }

  &__alert {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 12px;
    padding: 10px 14px;
    border-radius: var(--radius-sm);
    font-size: 13px;

    &--error {
      background: rgba(239, 68, 68, 0.1);
      color: #ef4444;
      border: 1px solid rgba(239, 68, 68, 0.2);
    }

    &--success {
      background: rgba(34, 197, 94, 0.1);
      color: #22c55e;
      border: 1px solid rgba(34, 197, 94, 0.2);
    }
  }
}

// Transitions
.slide-enter-active,
.slide-leave-active {
  transition: all 0.2s ease;
  overflow: hidden;
}

.slide-enter-from,
.slide-leave-to {
  opacity: 0;
  max-height: 0;
  margin-top: 0;
  padding-top: 0;
}

.slide-enter-to,
.slide-leave-from {
  opacity: 1;
  max-height: 60px;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
