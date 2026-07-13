<script setup lang="ts">
import { onMounted } from "vue";
import PostComp from "./PosT.vue";
import { usePostStore } from "@/stores/post-store";
import { storeToRefs } from "pinia";

const postStore = usePostStore();
const { posts, isLoading, error } = storeToRefs(postStore);

onMounted(() => {
  postStore.fetchPosts();
});
</script>

<template>
  <div class="post-feed">
    <div v-if="isLoading" class="post-feed__loading">
      <q-spinner-dots size="40px" color="accent" />
    </div>

    <div v-else-if="error" class="post-feed__error">
      <q-icon name="error_outline" size="48px" />
      <p>{{ error }}</p>
      <q-btn
        outline
        color="accent"
        label="Tentar novamente"
        @click="postStore.fetchPosts()"
      />
    </div>

    <template v-else>
      <template v-if="posts.length">
        <PostComp v-for="post in posts" :key="post.id" :post="post" />
      </template>
      <template v-else>
        <p class="post-feed__empty">
          Opa, não temos nenhum post ainda... (Por em quanto :D)
        </p>
      </template>
    </template>
  </div>
</template>

<style lang="scss" scoped>
.post-feed {
  display: flex;
  flex-direction: column;
  gap: 16px;
  width: 100%;

  &__empty {
    color: var(--text-secondary);
  }
  &__loading {
    display: flex;
    justify-content: center;
    padding: 48px;
  }

  &__error {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    padding: 48px;
    text-align: center;
    color: var(--text-muted);

    p {
      font-size: 16px;
      color: var(--text-secondary);
    }
  }
}
</style>
