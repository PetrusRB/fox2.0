<script setup lang="ts">
import { ref, computed, watch } from "vue";
import { useImageCdn } from "@/composables/useImageCdn";
import ViDeoPlayer from "./VideoPlayer.vue";

export interface FiLeUploadProps {
  accept?: string[];
  folder?: string;
  tags?: string[];
  maxSizes?: Record<string, number>;
  modelValue?: string;
  disabled?: boolean;
}

const MB = 1024 * 1024;

const props = withDefaults(defineProps<FiLeUploadProps>(), {
  accept: () => [
    "image/png",
    "image/jpeg",
    "image/gif",
    "image/webp",
    "video/mp4"
  ],
  folder: "/posts",
  tags: () => [],
  maxSizes: () => ({
    "image/*": 10,
    "video/*": 25
  }),
  modelValue: "",
  disabled: false
});

const emit = defineEmits<{
  "update:modelValue": [url: string];
  uploaded: [url: string];
  error: [message: string];
  fileSelected: [file: File];
}>();

const {
  uploading,
  progress,
  error: uploadError,
  upload,
  abort,
  reset
} = useImageCdn();

const fileInput = ref<HTMLInputElement | null>(null);
const isDragOver = ref(false);
const previewUrl = ref<string | null>(null);
const previewIsVideo = ref(false);
const selectedFile = ref<File | null>(null);
const localError = ref<string | null>(null);

const acceptString = computed(() => props.accept.join(","));

const progressPercent = computed(() => progress.value.percent / 100);

const progressColor = computed(() => {
  const p = progress.value.percent;
  if (p < 30) return "var(--accent)";
  if (p < 70) return "var(--avocado-400)";
  if (p < 100) return "var(--avocado-300)";
  return "#22c55e";
});

const hasPreview = computed(() => previewUrl.value !== null);
const hasValue = computed(() => props.modelValue.length > 0);

function formatSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function getMaxSizeBytes(file: File): number {
  const type = file.type;

  if (props.maxSizes[type] !== undefined) {
    return props.maxSizes[type] * MB;
  }

  const major = type.split("/")[0];
  const wildcard = `${major}/*`;
  if (props.maxSizes[wildcard] !== undefined) {
    return props.maxSizes[wildcard] * MB;
  }

  return 10 * MB;
}

function validateFile(file: File): string | null {
  if (!props.accept.includes(file.type)) {
    const exts = props.accept
      .map(t => t.split("/")[1]?.toUpperCase() || t)
      .join(", ");
    return `Tipo de arquivo nao permitido. Aceitos: ${exts}`;
  }
  const limit = getMaxSizeBytes(file);
  if (file.size > limit) {
    return `Arquivo muito grande. Maximo: ${formatSize(limit)}`;
  }
  return null;
}

function generatePreview(file: File) {
  if (previewUrl.value) URL.revokeObjectURL(previewUrl.value);
  const isVideo = file.type.startsWith("video/");
  if (isVideo || file.type.startsWith("image/")) {
    previewUrl.value = URL.createObjectURL(file);
    previewIsVideo.value = isVideo;
  } else {
    previewUrl.value = null;
    previewIsVideo.value = false;
  }
}

function handleFile(file: File) {
  localError.value = null;

  const validationError = validateFile(file);
  if (validationError) {
    localError.value = validationError;
    emit("error", validationError);
    return;
  }

  selectedFile.value = file;
  generatePreview(file);
  emit("fileSelected", file);
}

function onFileInputChange(event: Event) {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  if (file) handleFile(file);
  input.value = "";
}

function onDrop(event: DragEvent) {
  isDragOver.value = false;
  if (props.disabled || uploading.value) return;
  const file = event.dataTransfer?.files[0];
  if (file) handleFile(file);
}

function onDragOver(event: DragEvent) {
  event.preventDefault();
  if (!props.disabled && !uploading.value) isDragOver.value = true;
}

function onDragLeave() {
  isDragOver.value = false;
}

function openFilePicker() {
  if (props.disabled || uploading.value) return;
  fileInput.value?.click();
}

async function startUpload() {
  if (!selectedFile.value || uploading.value) return;

  try {
    const result = await upload(selectedFile.value, {
      folder: props.folder,
      tags: props.tags
    });
    emit("update:modelValue", result.url);
    emit("uploaded", result.url);
  } catch (e: any) {
    if (e?.name !== "AbortError" && e?.name !== "ImageKitAbortError") {
      const msg = uploadError.value || "Erro ao fazer upload";
      emit("error", msg);
    }
  }
}

function cancelUpload() {
  abort();
}

function removeFile() {
  if (previewUrl.value) URL.revokeObjectURL(previewUrl.value);
  previewUrl.value = null;
  previewIsVideo.value = false;
  selectedFile.value = null;
  localError.value = null;
  reset();
}

watch(
  () => props.modelValue,
  val => {
    if (!val && selectedFile.value) {
      removeFile();
    }
  }
);
</script>

<template>
  <div class="file-upload">
    <input
      ref="fileInput"
      type="file"
      :accept="acceptString"
      class="file-upload__input"
      @change="onFileInputChange"
    />

    <div
      v-if="!hasPreview && !hasValue"
      class="file-upload__dropzone"
      :class="{
        'file-upload__dropzone--active': isDragOver,
        'file-upload__dropzone--disabled': disabled || uploading
      }"
      @click="openFilePicker"
      @drop.prevent="onDrop"
      @dragover="onDragOver"
      @dragleave="onDragLeave"
    >
      <q-icon name="cloud_upload" size="36px" class="file-upload__icon" />
      <span class="file-upload__label">
        Arraste um arquivo ou clique para selecionar
      </span>
      <span class="file-upload__hint">
        Formatos aceitos:
        {{ accept.map(t => t.split("/")[1]?.toUpperCase() || t).join(", ") }}
      </span>
    </div>

    <div v-else class="file-upload__preview">
      <div class="file-upload__preview-content">
        <ViDeoPlayer
          v-if="previewUrl && previewIsVideo"
          :src="previewUrl"
          class="file-upload__preview-video"
        />
        <img
          v-else-if="previewUrl"
          :src="previewUrl"
          class="file-upload__preview-img"
          alt="Preview"
        />
        <div v-else-if="selectedFile" class="file-upload__file-info">
          <q-icon
            name="insert_drive_file"
            size="32px"
            class="file-upload__file-icon"
          />
          <div class="file-upload__file-details">
            <span class="file-upload__file-name">{{ selectedFile.name }}</span>
            <span class="file-upload__file-size">{{
              formatSize(selectedFile.size)
            }}</span>
          </div>
        </div>
        <div v-else-if="hasValue" class="file-upload__url-info">
          <q-icon name="link" size="20px" class="file-upload__file-icon" />
          <span class="file-upload__file-name">Arquivo enviado</span>
        </div>
      </div>

      <div v-if="uploading" class="file-upload__progress-section">
        <div class="file-upload__progress-header">
          <span class="file-upload__progress-label">Enviando...</span>
          <span class="file-upload__progress-value"
            >{{ progress.percent }}%</span
          >
        </div>
        <q-linear-progress
          :value="progressPercent"
          :color="progressColor"
          size="6px"
          rounded
          class="file-upload__progress-bar"
        />
        <div class="file-upload__progress-meta">
          <span
            >{{ formatSize(progress.loaded) }} /
            {{ formatSize(progress.total) }}</span
          >
        </div>
      </div>

      <div v-if="!uploading && !hasValue" class="file-upload__preview-actions">
        <button
          class="file-upload__action-btn file-upload__action-btn--upload"
          @click.stop="startUpload"
        >
          <q-icon name="upload" size="16px" />
          Enviar
        </button>
        <button
          class="file-upload__action-btn file-upload__action-btn--cancel"
          @click.stop="removeFile"
        >
          <q-icon name="close" size="16px" />
        </button>
      </div>

      <div v-if="uploading" class="file-upload__preview-actions">
        <button
          class="file-upload__action-btn file-upload__action-btn--cancel"
          @click.stop="cancelUpload"
        >
          <q-icon name="cancel" size="16px" />
          Cancelar
        </button>
      </div>

      <button
        v-if="!uploading && (hasPreview || hasValue)"
        class="file-upload__remove-btn"
        @click.stop="removeFile"
      >
        <q-icon name="close" size="18px" />
      </button>
    </div>

    <!-- <Transition name="fade"> -->
    <!--   <div v-if="localError" class="file-upload__error"> -->
    <!--     <q-icon name="error_outline" size="16px" /> -->
    <!--     {{ localError }} -->
    <!--   </div> -->
    <!-- </Transition> -->
  </div>
</template>

<style lang="scss" scoped>
.file-upload {
  display: flex;
  flex-direction: column;
  gap: 8px;
  width: 100%;

  &__input {
    display: none;
  }

  // ── Dropzone ──────────────────────────────────────
  &__dropzone {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 32px 20px;
    border: 2px dashed var(--border-color);
    border-radius: var(--radius-lg);
    background: var(--bg-elevated);
    cursor: pointer;
    transition: all 0.2s ease;

    &:hover:not(&--disabled) {
      border-color: var(--accent);
      background: var(--accent-bg);
    }

    &--active {
      border-color: var(--accent);
      background: var(--accent-bg);
      transform: scale(1.01);
    }

    &--disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
  }

  &__icon {
    color: var(--text-muted);
  }

  &__label {
    font-size: 14px;
    font-weight: 500;
    color: var(--text-primary);
  }

  &__hint {
    font-size: 12px;
    color: var(--text-muted);
  }

  // ── Preview ───────────────────────────────────────
  &__preview {
    position: relative;
    border: 1px solid var(--border-color);
    border-radius: var(--radius-lg);
    background: var(--bg-elevated);
    overflow: hidden;
  }

  &__preview-content {
    width: 100%;
    max-height: 300px;
    overflow: hidden;
  }

  &__preview-img {
    width: 100%;
    height: auto;
    max-height: 300px;
    object-fit: cover;
    display: block;
  }

  &__preview-video {
    width: 100%;
  }

  &__file-info,
  &__url-info {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 20px;
  }

  &__file-icon {
    color: var(--accent);
    flex-shrink: 0;
  }

  &__file-details {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
  }

  &__file-name {
    font-size: 14px;
    font-weight: 500;
    color: var(--text-primary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &__file-size {
    font-size: 12px;
    color: var(--text-muted);
  }

  // ── Progress ──────────────────────────────────────
  &__progress-section {
    padding: 12px 16px;
    border-top: 1px solid var(--border-color);
  }

  &__progress-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 8px;
  }

  &__progress-label {
    font-size: 13px;
    font-weight: 500;
    color: var(--text-primary);
  }

  &__progress-value {
    font-size: 13px;
    font-weight: 600;
    color: var(--accent);
    font-variant-numeric: tabular-nums;
  }

  &__progress-bar {
    border-radius: 3px;
  }

  &__progress-meta {
    display: flex;
    justify-content: flex-end;
    margin-top: 4px;

    span {
      font-size: 11px;
      color: var(--text-muted);
      font-variant-numeric: tabular-nums;
    }
  }

  // ── Actions ───────────────────────────────────────
  &__preview-actions {
    display: flex;
    gap: 8px;
    padding: 12px 16px;
    border-top: 1px solid var(--border-color);
  }

  &__action-btn {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    border-radius: var(--radius-sm);
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    border: 1px solid var(--border-color);
    transition: all 0.15s ease;

    &--upload {
      background: var(--accent);
      color: var(--zinc-950);
      border-color: var(--accent);

      &:hover {
        background: var(--accent-hover);
      }
    }

    &--cancel {
      background: transparent;
      color: var(--text-secondary);

      &:hover {
        background: var(--bg-elevated);
        color: var(--text-primary);
      }
    }
  }

  &__remove-btn {
    position: absolute;
    top: 8px;
    right: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 28px;
    height: 28px;
    border-radius: 50%;
    background: rgba(0, 0, 0, 0.6);
    backdrop-filter: blur(4px);
    border: none;
    color: white;
    cursor: pointer;
    transition: all 0.15s ease;
    z-index: 1;

    &:hover {
      background: rgba(239, 68, 68, 0.8);
      transform: scale(1.1);
    }
  }

  // ── Error ─────────────────────────────────────────
  &__error {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 14px;
    border-radius: var(--radius-sm);
    font-size: 13px;
    background: rgba(239, 68, 68, 0.1);
    color: #ef4444;
    border: 1px solid rgba(239, 68, 68, 0.2);
  }
}

// ── Transitions ────────────────────────────────────
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
