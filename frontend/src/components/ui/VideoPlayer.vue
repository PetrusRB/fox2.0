<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch, nextTick } from "vue";

export interface ViDeoPlayerProps {
  src: string;
  poster?: string;
  muted?: boolean;
  loop?: boolean;
  autoload?: boolean;
}

const props = withDefaults(defineProps<ViDeoPlayerProps>(), {
  poster: "",
  muted: false,
  loop: false,
  autoload: true
});

const emit = defineEmits<{
  play: [];
  pause: [];
  ended: [];
  error: [message: string];
}>();

const containerEl = ref<HTMLDivElement | null>(null);
const videoEl = ref<HTMLVideoElement | null>(null);
const controlsEl = ref<HTMLDivElement | null>(null);
const progressEl = ref<HTMLDivElement | null>(null);
const playedBarEl = ref<HTMLDivElement | null>(null);
const bufferedBarEl = ref<HTMLDivElement | null>(null);
const thumbEl = ref<HTMLDivElement | null>(null);
const hoverTimeEl = ref<HTMLSpanElement | null>(null);
const volumeSliderEl = ref<HTMLInputElement | null>(null);

const playing = ref(false);
const loading = ref(false);
const error = ref<string | null>(null);
const isPiP = ref(false);
const isFullscreen = ref(false);
const playbackRate = ref(1);
const volume = ref(1);
const mutedState = ref(false);
const timeDisplay = ref("0:00 / 0:00");

let currentTime = 0;
let duration = 0;
let bufferedEnd = 0;
let lastDisplayedSecond = -1;
let rafId = 0;
let controlsTimer: ReturnType<typeof setTimeout> | null = null;
let isSeeking = false;
let showSpeedMenu = false;

const SPEEDS = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 2];

function formatTime(sec: number): string {
  if (!isFinite(sec)) return "0:00";
  const h = Math.floor(sec / 3600);
  const m = Math.floor((sec % 3600) / 60);
  const s = Math.floor(sec % 60);
  if (h > 0)
    return `${h}:${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
  return `${m}:${String(s).padStart(2, "0")}`;
}

function updateTimeDisplay() {
  const s = Math.floor(currentTime);
  if (s !== lastDisplayedSecond) {
    lastDisplayedSecond = s;
    timeDisplay.value = `${formatTime(currentTime)} / ${formatTime(duration)}`;
  }
}

function updateProgressBar() {
  const el = videoEl.value;
  if (!el || isSeeking) return;

  currentTime = el.currentTime;
  duration = el.duration || 0;

  const pct = duration > 0 ? (currentTime / duration) * 100 : 0;
  if (playedBarEl.value) playedBarEl.value.style.width = `${pct}%`;
  if (thumbEl.value) thumbEl.value.style.left = `${pct}%`;

  updateTimeDisplay();
}

function updateBufferedBar() {
  const el = videoEl.value;
  if (!el || el.buffered.length === 0) return;
  bufferedEnd = el.buffered.end(el.buffered.length - 1);
  const pct = duration > 0 ? (bufferedEnd / duration) * 100 : 0;
  if (bufferedBarEl.value) bufferedBarEl.value.style.width = `${pct}%`;
}

function startRafLoop() {
  stopRafLoop();
  const tick = () => {
    updateProgressBar();
    rafId = requestAnimationFrame(tick);
  };
  rafId = requestAnimationFrame(tick);
}

function stopRafLoop() {
  if (rafId) {
    cancelAnimationFrame(rafId);
    rafId = 0;
  }
}

function showControls() {
  const el = controlsEl.value;
  if (!el) return;
  el.classList.remove("video-player__controls--hidden");
  scheduleHideControls();
}

function hideControls() {
  const el = controlsEl.value;
  if (!el || !playing.value || isSeeking) return;
  el.classList.add("video-player__controls--hidden");
}

function scheduleHideControls(ms = 3000) {
  clearTimeoutIfAny();
  controlsTimer = setTimeout(hideControls, ms);
}

function clearTimeoutIfAny() {
  if (controlsTimer) {
    clearTimeout(controlsTimer);
    controlsTimer = null;
  }
}

function onMouseMove() {
  showControls();
}

function onMouseLeave() {
  if (playing.value) scheduleHideControls(800);
  hideHoverTime();
}

function showHoverTime(e: MouseEvent) {
  const bar = progressEl.value;
  const hoverEl = hoverTimeEl.value;
  if (!bar || !hoverEl || !duration) return;
  const rect = bar.getBoundingClientRect();
  const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  hoverEl.textContent = formatTime(ratio * duration);
  hoverEl.style.left = `${e.clientX - rect.left}px`;
  hoverEl.style.opacity = "1";
}

function hideHoverTime() {
  const el = hoverTimeEl.value;
  if (el) el.style.opacity = "0";
}

function togglePlay() {
  const el = videoEl.value;
  if (!el || loading.value) return;
  if (el.paused) el.play().catch(() => {});
  else el.pause();
}

function seekFromClick(e: MouseEvent) {
  const bar = progressEl.value;
  if (!bar || !duration) return;
  const rect = bar.getBoundingClientRect();
  const ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
  videoEl.value!.currentTime = ratio * duration;
  updateProgressBar();
}

function onProgressMouseDown(e: MouseEvent) {
  isSeeking = true;
  seekFromClick(e);

  const onMove = (ev: MouseEvent) => {
    const bar = progressEl.value;
    if (!bar || !duration) return;
    const rect = bar.getBoundingClientRect();
    const ratio = Math.max(
      0,
      Math.min(1, (ev.clientX - rect.left) / rect.width)
    );
    const pct = ratio * 100;
    if (playedBarEl.value) playedBarEl.value.style.width = `${pct}%`;
    if (thumbEl.value) thumbEl.value.style.left = `${pct}%`;
    const s = Math.floor(ratio * duration);
    if (s !== lastDisplayedSecond) {
      lastDisplayedSecond = s;
      timeDisplay.value = `${formatTime(ratio * duration)} / ${formatTime(duration)}`;
    }
  };

  const onUp = (ev: MouseEvent) => {
    isSeeking = false;
    seekFromClick(ev);
    document.removeEventListener("mousemove", onMove);
    document.removeEventListener("mouseup", onUp);
    if (playing.value) scheduleHideControls();
  };

  document.addEventListener("mousemove", onMove);
  document.addEventListener("mouseup", onUp);
}

function toggleMute() {
  const el = videoEl.value;
  if (!el) return;
  el.muted = !el.muted;
}

function onVolumeInput(e: Event) {
  const el = videoEl.value;
  if (!el) return;
  const val = parseFloat((e.target as HTMLInputElement).value);
  el.volume = val;
  if (val > 0) el.muted = false;
}

function toggleSpeedMenu() {
  showSpeedMenu = !showSpeedMenu;
  const menu = containerEl.value?.querySelector(".video-player__speed-menu");
  if (menu) {
    (menu as HTMLElement).style.opacity = showSpeedMenu ? "1" : "0";
    (menu as HTMLElement).style.pointerEvents = showSpeedMenu ? "auto" : "none";
  }
}

function setSpeed(rate: number) {
  const el = videoEl.value;
  if (!el) return;
  el.playbackRate = rate;
  playbackRate.value = rate;
  showSpeedMenu = false;
  const menu = containerEl.value?.querySelector(
    ".video-player__speed-menu"
  ) as HTMLElement;
  if (menu) {
    menu.style.opacity = "0";
    menu.style.pointerEvents = "none";
  }
}

async function toggleFullscreen() {
  const container = containerEl.value;
  if (!container) return;
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await container.requestFullscreen();
  } catch {}
}

async function togglePiP() {
  const el = videoEl.value;
  if (!el) return;
  try {
    if (document.pictureInPictureElement) await document.exitPictureInPicture();
    else if (document.pictureInPictureEnabled)
      await el.requestPictureInPicture();
  } catch {}
}

function onPlay() {
  playing.value = true;
  loading.value = false;
  showControls();
  startRafLoop();
  emit("play");
}

function onPause() {
  playing.value = false;
  stopRafLoop();
  updateProgressBar();
  showControls();
  clearTimeoutIfAny();
  emit("pause");
}

function onTimeUpdate() {
  updateTimeDisplay();
}

function onLoadedMetadata() {
  const el = videoEl.value;
  if (el) {
    duration = el.duration;
    loading.value = false;
    updateTimeDisplay();
  }
}

function onProgress() {
  updateBufferedBar();
}

function onWaiting() {
  loading.value = true;
}

function onCanPlay() {
  loading.value = false;
}

function onEnded() {
  playing.value = false;
  currentTime = 0;
  stopRafLoop();
  updateProgressBar();
  showControls();
  clearTimeoutIfAny();
  emit("ended");
}

function onError() {
  const el = videoEl.value;
  if (el?.error) {
    const codes: Record<number, string> = {
      1: "Carregamento abortado",
      2: "Erro de rede",
      3: "Falha ao decodificar o video",
      4: "Formato nao suportado"
    };
    error.value = codes[el.error.code] || "Erro ao reproduzir video";
  }
  loading.value = false;
  emit("error", error.value || "Erro desconhecido");
}

function onVolumeChange() {
  const el = videoEl.value;
  if (el) {
    volume.value = el.volume;
    mutedState.value = el.muted;
  }
}

function onEnterPiP() {
  isPiP.value = true;
}
function onLeavePiP() {
  isPiP.value = false;
}

function onFullscreenChange() {
  isFullscreen.value = !!document.fullscreenElement;
}
let listeners: AbortController | null = null;

function bindEvents(el: HTMLVideoElement) {
  unbindEvents();
  listeners = new AbortController();
  const s = listeners.signal;

  el.addEventListener("play", onPlay, { signal: s });
  el.addEventListener("pause", onPause, { signal: s });
  el.addEventListener("timeupdate", onTimeUpdate, { signal: s });
  el.addEventListener("loadedmetadata", onLoadedMetadata, { signal: s });
  el.addEventListener("progress", onProgress, { signal: s });
  el.addEventListener("waiting", onWaiting, { signal: s });
  el.addEventListener("canplay", onCanPlay, { signal: s });
  el.addEventListener("ended", onEnded, { signal: s });
  el.addEventListener("error", onError, { signal: s });
  el.addEventListener("volumechange", onVolumeChange, { signal: s });
  el.addEventListener("enterpictureinpicture", onEnterPiP, { signal: s });
  el.addEventListener("leavepictureinpicture", onLeavePiP, { signal: s });
}

function unbindEvents() {
  listeners?.abort();
  listeners = null;
}

const KEYS = new Map<string, (el: HTMLVideoElement) => void>([
  [" ", el => (el.paused ? el.play().catch(() => {}) : el.pause())],
  ["k", el => (el.paused ? el.play().catch(() => {}) : el.pause())],
  [
    "m",
    el => {
      el.muted = !el.muted;
    }
  ],
  [
    "ArrowLeft",
    el => {
      el.currentTime = Math.max(0, el.currentTime - 5);
    }
  ],
  [
    "ArrowRight",
    el => {
      el.currentTime = Math.min(el.duration || 0, el.currentTime + 5);
    }
  ],
  [
    "ArrowUp",
    el => {
      el.currentTime = Math.min(1, el.volume + 0.1);
    }
  ],
  [
    "ArrowDown",
    el => {
      el.currentTime = Math.max(0, el.volume - 0.1);
    }
  ]
]);

function onKeyDown(ev: KeyboardEvent) {
  const el = videoEl.value;
  if (!el) return;
  if ((ev.target as HTMLElement).tagName === "INPUT") return;

  const handler = KEYS.get(ev.key);
  if (handler) {
    ev.preventDefault();
    handler(el);
    showControls();
  }
}

function onClickOutside(e: MouseEvent) {
  if (
    showSpeedMenu &&
    !(e.target as HTMLElement).closest(".video-player__speed")
  ) {
    setSpeed(playbackRate.value);
  }
}

onMounted(async () => {
  await nextTick();
  const el = videoEl.value;
  if (!el) return;

  el.muted = props.muted;
  mutedState.value = props.muted;
  el.preload = props.autoload ? "auto" : "metadata";

  bindEvents(el);

  if (props.src && el.src !== props.src) {
    el.src = props.src;
    el.load();
  }

  document.addEventListener("fullscreenchange", onFullscreenChange);
  document.addEventListener("click", onClickOutside);
});

onBeforeUnmount(() => {
  const el = videoEl.value;
  if (el) unbindEvents();
  stopRafLoop();
  clearTimeoutIfAny();
  document.removeEventListener("fullscreenchange", onFullscreenChange);
  document.removeEventListener("click", onClickOutside);
  if (document.fullscreenElement) document.exitFullscreen().catch(() => {});
});

watch(
  () => props.src,
  newSrc => {
    const el = videoEl.value;
    if (!el || !newSrc || el.src === newSrc) return;

    stopRafLoop();
    playing.value = false;
    currentTime = 0;
    duration = 0;
    bufferedEnd = 0;
    lastDisplayedSecond = -1;
    timeDisplay.value = "0:00 / 0:00";
    if (playedBarEl.value) playedBarEl.value.style.width = "0%";
    if (bufferedBarEl.value) bufferedBarEl.value.style.width = "0%";
    if (thumbEl.value) thumbEl.value.style.left = "0%";
    el.src = newSrc;
    el.load();
    loading.value = true;
    error.value = null;
  }
);
</script>

<template>
  <div
    ref="containerEl"
    class="video-player"
    :class="{ 'video-player--fullscreen': isFullscreen }"
    @mousemove="onMouseMove"
    @mouseleave="onMouseLeave"
    @keydown="onKeyDown"
    tabindex="0"
  >
    <video
      ref="videoEl"
      class="video-player__video"
      :poster="poster || undefined"
      :loop="loop"
      playsinline
      @click="togglePlay"
    />

    <!-- Big Play -->
    <button
      v-show="!playing && !loading"
      class="video-player__big-play"
      @click="togglePlay"
    >
      <q-icon name="play_arrow" size="56px" />
    </button>

    <!-- Loading -->
    <div v-show="loading && playing" class="video-player__loading">
      <q-spinner size="40px" color="accent" />
    </div>

    <!-- Error -->
    <div v-if="error" class="video-player__error">
      <q-icon name="error_outline" size="20px" />
      <span>{{ error }}</span>
    </div>

    <!-- Controls -->
    <div ref="controlsEl" class="video-player__controls">
      <!-- Progress -->
      <div
        ref="progressEl"
        class="video-player__progress"
        @mousedown="onProgressMouseDown"
        @mousemove="showHoverTime"
        @mouseleave="hideHoverTime"
      >
        <div class="video-player__progress-track">
          <div ref="bufferedBarEl" class="video-player__progress-buffered" />
          <div ref="playedBarEl" class="video-player__progress-played" />
          <div ref="thumbEl" class="video-player__progress-thumb" />
        </div>
        <span ref="hoverTimeEl" class="video-player__hover-time" />
      </div>

      <!-- Bottom -->
      <div class="video-player__bottom">
        <div class="video-player__left">
          <button
            class="video-player__btn"
            :title="playing ? 'Pausar (K)' : 'Reproduzir (K)'"
            @click="togglePlay"
          >
            <q-icon :name="playing ? 'pause' : 'play_arrow'" size="22px" />
          </button>

          <div class="video-player__volume">
            <button
              class="video-player__btn"
              :title="mutedState ? 'Desativar mudo (M)' : 'Ativar mudo (M)'"
              @click="toggleMute"
            >
              <q-icon
                :name="
                  mutedState || volume === 0
                    ? 'volume_off'
                    : volume < 0.5
                      ? 'volume_down'
                      : 'volume_up'
                "
                size="20px"
              />
            </button>
            <div class="video-player__volume-slider">
              <input
                ref="volumeSliderEl"
                type="range"
                class="video-player__slider"
                min="0"
                max="1"
                step="0.05"
                :value="mutedState ? 0 : volume"
                @input="onVolumeInput"
              />
            </div>
          </div>

          <span class="video-player__time">{{ timeDisplay }}</span>
        </div>

        <div class="video-player__right">
          <div class="video-player__speed">
            <button
              class="video-player__btn video-player__btn--speed"
              title="Velocidade"
              @click.stop="toggleSpeedMenu"
            >
              {{ playbackRate === 1 ? "1x" : `${playbackRate}x` }}
            </button>
            <div class="video-player__speed-menu">
              <button
                v-for="rate in SPEEDS"
                :key="rate"
                class="video-player__speed-item"
                :class="{
                  'video-player__speed-item--active': playbackRate === rate
                }"
                @click="setSpeed(rate)"
              >
                {{ rate === 1 ? "Normal" : `${rate}x` }}
              </button>
            </div>
          </div>

          <button
            class="video-player__btn"
            title="Picture-in-Picture (P)"
            @click="togglePiP"
          >
            <q-icon name="picture_in_picture_alt" size="20px" />
          </button>

          <button
            class="video-player__btn"
            :title="isFullscreen ? 'Sair da tela cheia (F)' : 'Tela cheia (F)'"
            @click="toggleFullscreen"
          >
            <q-icon
              :name="isFullscreen ? 'fullscreen_exit' : 'fullscreen'"
              size="22px"
            />
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<style lang="scss" scoped>
.video-player {
  position: relative;
  width: 100%;
  background: #000;
  border-radius: var(--radius-lg);
  overflow: hidden;
  outline: none;
  user-select: none;

  &--fullscreen {
    border-radius: 0;
  }

  // ── Video ──────────────────────────────────────
  &__video {
    display: block;
    width: 100%;
    max-height: 480px;
    object-fit: contain;
    background: #000;
    cursor: pointer;

    &::-webkit-media-controls {
      display: none !important;
    }
  }

  // ── Big Play ───────────────────────────────────
  &__big-play {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    align-items: center;
    justify-content: center;
    width: 72px;
    height: 72px;
    border-radius: 50%;
    background: rgba(0, 0, 0, 0.6);
    backdrop-filter: blur(8px);
    border: 2px solid rgba(255, 255, 255, 0.15);
    color: white;
    cursor: pointer;
    transition: all 0.2s ease;
    z-index: 5;

    .q-icon {
      margin-left: 3px;
    }

    &:hover {
      background: var(--accent);
      border-color: var(--accent);
      transform: translate(-50%, -50%) scale(1.08);
    }
  }

  // ── Loading ────────────────────────────────────
  &__loading {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    z-index: 5;
    pointer-events: none;
  }

  // ── Error ──────────────────────────────────────
  &__error {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 12px 20px;
    background: rgba(0, 0, 0, 0.8);
    backdrop-filter: blur(8px);
    border-radius: var(--radius-md);
    color: #ef4444;
    font-size: 13px;
    font-weight: 500;
    z-index: 5;
  }

  // ── Controls ───────────────────────────────────
  &__controls {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    background: linear-gradient(transparent, rgba(0, 0, 0, 0.85));
    padding: 32px 12px 8px;
    z-index: 10;
    transition: opacity 0.25s ease;

    &--hidden {
      opacity: 0;
      pointer-events: none;
    }
  }

  // ── Progress ───────────────────────────────────
  &__progress {
    position: relative;
    width: 100%;
    height: 18px;
    display: flex;
    align-items: center;
    cursor: pointer;
    margin-bottom: 4px;

    &:hover {
      .video-player__progress-track {
        height: 5px;
      }

      .video-player__progress-thumb {
        opacity: 1;
        transform: translate(-50%, -50%) scale(1);
      }
    }
  }

  &__progress-track {
    position: relative;
    width: 100%;
    height: 3px;
    background: rgba(255, 255, 255, 0.2);
    border-radius: 2px;
    transition: height 0.15s ease;
  }

  &__progress-buffered {
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    width: 0;
    background: rgba(255, 255, 255, 0.25);
    border-radius: 2px;
  }

  &__progress-played {
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    width: 0;
    background: var(--accent);
    border-radius: 2px;
  }

  &__progress-thumb {
    position: absolute;
    top: 50%;
    left: 0;
    width: 13px;
    height: 13px;
    background: white;
    border-radius: 50%;
    transform: translate(-50%, -50%) scale(0);
    opacity: 0;
    transition: all 0.15s ease;
    z-index: 2;
    pointer-events: none;
    box-shadow: 0 0 6px rgba(0, 0, 0, 0.4);
  }

  &__hover-time {
    position: absolute;
    bottom: 100%;
    transform: translateX(-50%);
    padding: 3px 7px;
    background: rgba(0, 0, 0, 0.85);
    border-radius: 4px;
    color: white;
    font-size: 11px;
    font-weight: 500;
    font-variant-numeric: tabular-nums;
    white-space: nowrap;
    pointer-events: none;
    margin-bottom: 6px;
    opacity: 0;
    transition: opacity 0.12s ease;
  }

  // ── Bottom ─────────────────────────────────────
  &__bottom {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
  }

  &__left,
  &__right {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  // ── Buttons ────────────────────────────────────
  &__btn {
    display: flex;
    align-items: center;
    justify-content: center;
    min-width: 34px;
    height: 34px;
    padding: 0 6px;
    border: none;
    border-radius: var(--radius-sm);
    background: transparent;
    color: rgba(255, 255, 255, 0.85);
    cursor: pointer;
    transition: all 0.15s ease;
    font-family: inherit;

    &:hover {
      background: rgba(255, 255, 255, 0.1);
      color: white;
    }

    &--speed {
      font-size: 13px;
      font-weight: 600;
      min-width: 40px;
      font-variant-numeric: tabular-nums;
    }
  }

  // ── Volume ─────────────────────────────────────
  &__volume {
    display: flex;
    align-items: center;
    gap: 2px;

    &:hover .video-player__volume-slider {
      width: 80px;
      opacity: 1;
    }
  }

  &__volume-slider {
    width: 0;
    opacity: 0;
    overflow: hidden;
    transition: all 0.2s ease;
  }

  &__slider {
    -webkit-appearance: none;
    appearance: none;
    width: 80px;
    height: 4px;
    background: rgba(255, 255, 255, 0.2);
    border-radius: 2px;
    outline: none;
    cursor: pointer;

    &::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 12px;
      height: 12px;
      background: white;
      border-radius: 50%;
      cursor: pointer;
      box-shadow: 0 0 4px rgba(0, 0, 0, 0.3);
    }

    &::-moz-range-thumb {
      width: 12px;
      height: 12px;
      background: white;
      border-radius: 50%;
      cursor: pointer;
      border: none;
    }

    &::-moz-range-track {
      height: 4px;
      background: rgba(255, 255, 255, 0.2);
      border-radius: 2px;
    }
  }

  // ── Time ───────────────────────────────────────
  &__time {
    font-size: 13px;
    font-weight: 500;
    color: rgba(255, 255, 255, 0.85);
    font-variant-numeric: tabular-nums;
    white-space: nowrap;
    padding: 0 4px;
  }

  // ── Speed ──────────────────────────────────────
  &__speed {
    position: relative;
  }

  &__speed-menu {
    position: absolute;
    bottom: 100%;
    right: 0;
    min-width: 110px;
    padding: 4px;
    background: rgba(15, 15, 18, 0.95);
    backdrop-filter: blur(12px);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: var(--radius-md);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.5);
    margin-bottom: 8px;
    z-index: 20;
    opacity: 0;
    pointer-events: none;
    transition: opacity 0.15s ease;
  }

  &__speed-item {
    display: block;
    width: 100%;
    padding: 7px 12px;
    background: none;
    border: none;
    border-radius: var(--radius-sm);
    color: rgba(255, 255, 255, 0.7);
    font-size: 13px;
    font-weight: 500;
    font-family: inherit;
    text-align: left;
    cursor: pointer;
    transition: all 0.12s ease;

    &:hover {
      background: rgba(255, 255, 255, 0.08);
      color: white;
    }

    &--active {
      color: var(--accent);
      font-weight: 600;

      &:hover {
        color: var(--accent);
      }
    }
  }
}
</style>

<style>
.video-player:fullscreen,
.video-player:-webkit-full-screen {
  width: 100vw;
  height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background: #000;
  border-radius: 0;
  overflow: hidden;
}

.video-player:fullscreen .video-player__video,
.video-player:-webkit-full-screen .video-player__video {
  flex: 1;
  width: 100%;
  max-height: none;
  min-height: 0;
  height: auto;
  object-fit: contain;
}

.video-player:fullscreen .video-player__controls,
.video-player:-webkit-full-screen .video-player__controls {
  flex-shrink: 0;
  width: 100%;
  padding-bottom: 16px;
}
</style>
