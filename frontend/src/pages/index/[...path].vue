<template>
  <div class="not-found">
    <div class="not-found__content">
      <div class="not-found__code">404</div>
      <h1 class="not-found__title">Página não encontrada</h1>
      <p class="not-found__subtitle">
        Oops! Parece que esta página não existe ou foi movida.
      </p>
      <router-link to="/" class="not-found__btn">
        <q-icon name="home" size="20px" />
        <span>Voltar para o início</span>
      </router-link>
    </div>

    <div class="not-found__decoration">
      <div class="not-found__circle not-found__circle--1" />
      <div class="not-found__circle not-found__circle--2" />
      <div class="not-found__circle not-found__circle--3" />
    </div>
  </div>
</template>

<style lang="scss" scoped>
.not-found {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: calc(100vh - var(--navbar-height));
  padding: 48px 24px;
  position: relative;
  overflow: hidden;

  &__content {
    text-align: center;
    position: relative;
    z-index: 1;
    max-width: 500px;
  }

  &__code {
    font-size: clamp(120px, 25vw, 200px);
    font-weight: 800;
    color: var(--accent);
    line-height: 1;
    margin-bottom: 24px;
    text-shadow: 0 0 80px rgba(101, 163, 13, 0.3);
    animation: pulse 3s ease-in-out infinite;
  }

  &__title {
    font-size: 28px;
    font-weight: 700;
    color: var(--text-primary);
    margin-bottom: 12px;
    letter-spacing: -0.02em;
  }

  &__subtitle {
    font-size: 16px;
    color: var(--text-secondary);
    line-height: 1.6;
    margin-bottom: 32px;
  }

  &__btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 14px 28px;
    background: var(--accent);
    color: var(--zinc-950);
    font-size: 15px;
    font-weight: 600;
    border-radius: var(--radius-md);
    text-decoration: none;
    transition: all 0.2s ease;

    &:hover {
      background: var(--accent-hover);
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(101, 163, 13, 0.3);
    }
  }

  // ── Decorative Circles ─────────────────────────────
  &__decoration {
    position: absolute;
    inset: 0;
    pointer-events: none;
    overflow: hidden;
  }

  &__circle {
    position: absolute;
    border-radius: 50%;
    border: 1px solid var(--border-color);
    opacity: 0.3;

    &--1 {
      width: 400px;
      height: 400px;
      top: -100px;
      right: -100px;
      animation: float 6s ease-in-out infinite;
    }

    &--2 {
      width: 300px;
      height: 300px;
      bottom: -80px;
      left: -80px;
      animation: float 8s ease-in-out infinite reverse;
    }

    &--3 {
      width: 200px;
      height: 200px;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      border-color: var(--accent);
      opacity: 0.2;
      animation: pulse 4s ease-in-out infinite;
    }
  }
}

@keyframes float {
  0%,
  100% {
    transform: translateY(0) rotate(0deg);
  }
  50% {
    transform: translateY(-20px) rotate(5deg);
  }
}

@keyframes pulse {
  0%,
  100% {
    opacity: 0.2;
    transform: scale(1);
  }
  50% {
    opacity: 0.4;
    transform: scale(1.05);
  }
}
</style>
