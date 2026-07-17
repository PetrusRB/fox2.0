<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from "vue";

export interface DropDownItem {
  key: string;
  label: string;
  icon?: string;
  danger?: boolean;
  disabled?: boolean;
  divider?: boolean;
}

const props = withDefaults(
  defineProps<{
    items?: DropDownItem[];
    placement?: "left" | "right";
  }>(),
  {
    items: () => [],
    placement: "right",
  },
);

const emit = defineEmits<{
  select: [item: DropDownItem];
}>();

const isOpen = ref(false);
const menuRef = ref<HTMLElement | null>(null);
const triggerRef = ref<HTMLElement | null>(null);
const activeIndex = ref(-1);

const selectableItems = computed(() =>
  props.items.filter((item) => !item.divider && !item.disabled),
);

const visibleItems = computed(() => props.items);

function toggle() {
  isOpen.value = !isOpen.value;
  if (isOpen.value) activeIndex.value = -1;
}

function close() {
  isOpen.value = false;
  activeIndex.value = -1;
}

function handleSelect(item: DropDownItem) {
  if (item.disabled || item.divider) return;
  emit("select", item);
  close();
}

function handleClickOutside(e: MouseEvent) {
  if (
    menuRef.value &&
    !menuRef.value.contains(e.target as Node) &&
    triggerRef.value &&
    !triggerRef.value.contains(e.target as Node)
  ) {
    close();
  }
}

function handleKeydown(e: KeyboardEvent) {
  if (!isOpen.value) return;

  switch (e.key) {
    case "Escape":
      close();
      break;
    case "ArrowDown":
      e.preventDefault();
      moveActive(1);
      break;
    case "ArrowUp":
      e.preventDefault();
      moveActive(-1);
      break;
    case "Enter":
      e.preventDefault();
      if (activeIndex.value >= 0) {
        handleSelect(selectableItems.value[activeIndex.value]);
      }
      break;
  }
}

function moveActive(direction: number) {
  const len = selectableItems.value.length;
  if (len === 0) return;
  activeIndex.value =
    (activeIndex.value + direction + len) % len;
}

onMounted(() => {
  document.addEventListener("mousedown", handleClickOutside);
  document.addEventListener("keydown", handleKeydown);
});

onBeforeUnmount(() => {
  document.removeEventListener("mousedown", handleClickOutside);
  document.removeEventListener("keydown", handleKeydown);
});
</script>

<template>
  <div class="drop-down" :class="`drop-down--${placement}`">
    <button ref="triggerRef" class="drop-down__trigger" @click="toggle">
      <slot name="trigger">
        <q-icon name="more_vert" size="18px" />
      </slot>
    </button>

    <Transition name="drop-down">
      <div v-if="isOpen" ref="menuRef" class="drop-down__menu">
        <template v-if="$slots.header">
          <div class="drop-down__header">
            <slot name="header" />
          </div>
          <div class="drop-down__divider" />
        </template>

        <template v-for="item in visibleItems" :key="item.key">
          <div v-if="item.divider" class="drop-down__divider" />
          <button
            v-else
            class="drop-down__item"
            :class="{
              'drop-down__item--danger': item.danger,
              'drop-down__item--disabled': item.disabled,
              'drop-down__item--active': selectableItems.indexOf(item) === activeIndex,
            }"
            :disabled="item.disabled"
            @click="handleSelect(item)"
          >
            <q-icon
              v-if="item.icon"
              :name="item.icon"
              size="18px"
              class="drop-down__item-icon"
            />
            <span>{{ item.label }}</span>
          </button>
        </template>

        <slot />
      </div>
    </Transition>
  </div>
</template>

<style lang="scss" scoped>
.drop-down {
  position: relative;

  &__trigger {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 32px;
    height: 32px;
    background: transparent;
    border: none;
    border-radius: 50%;
    color: var(--text-muted);
    cursor: pointer;
    transition: all 0.15s ease;

    &:hover {
      background: var(--bg-elevated);
      color: var(--text-primary);
    }
  }

  &__menu {
    position: absolute;
    top: calc(100% + 4px);
    min-width: 180px;
    padding: 6px;
    background: var(--bg-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-md);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
    z-index: 100;
  }

  &--right &__menu {
    right: 0;
  }

  &--left &__menu {
    left: 0;
  }

  &__item {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    padding: 10px 12px;
    background: transparent;
    border: none;
    border-radius: var(--radius-sm);
    color: var(--text-secondary);
    font-size: 14px;
    font-family: inherit;
    text-align: left;
    cursor: pointer;
    transition: all 0.12s ease;

    &:hover {
      color: var(--text-primary);
      background: var(--bg-primary);
    }

    &--active {
      color: var(--text-primary);
      background: var(--bg-primary);
    }

    &--danger {
      color: #ef4444;

      &:hover {
        background: rgba(239, 68, 68, 0.1);
        color: #ef4444;
      }
    }

    &--disabled {
      opacity: 0.4;
      cursor: not-allowed;
      pointer-events: none;
    }
  }

  &__item-icon {
    flex-shrink: 0;
  }

  &__header {
    padding: 4px 0;
  }

  &__divider {
    height: 1px;
    background: var(--border-color);
    margin: 6px 0;
  }
}

.drop-down-enter-active,
.drop-down-leave-active {
  transition: all 0.15s ease;
}

.drop-down-enter-from,
.drop-down-leave-to {
  opacity: 0;
  transform: translateY(-4px);
}
</style>
