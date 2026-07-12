<script setup lang="ts">
import PosTSection from "@/components/post/PosTSection.vue";
import CreatePost from "@/components/CreatePost.vue";
</script>

<template>
  <q-page>
    <div class="feed-page">
      <CreatePost />
      <PosTSection />
    </div>
  </q-page>
</template>

<style lang="scss" scoped>
.feed-page {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;
  max-width: 620px;
  margin: 0 auto;
  padding: 20px 16px;
}
</style>
