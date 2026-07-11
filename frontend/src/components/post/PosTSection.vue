<script setup lang="ts">
import { ref, onMounted } from "vue";
import type { Post } from "@/types/post.type";
import PostComp from "./PosT.vue";
import { listFeed } from "@/composables/useSocialClient";

const posts = ref<Post[]>([]);
const loading = ref(true);
const error = ref<string | null>(null);

const mockPosts: Post[] = [
  {
    id: "1",
    author: {
      id: "1",
      username: "joaosilva",
      displayName: "Joao Silva",
      avatarUrl: "",
      bio: "Desenvolvedor apaixonado por tecnologia",
      followersCount: 150,
      followingCount: 45,
      postsCount: 23,
      createdAt: "2024-01-15T10:00:00Z",
    },
    title: "Início do Projeto",
    content:
      "Primeiro dia de projeto! Comecando a organizar toda a estrutura da aplicacao.",
    imageUrl: "",
    likesCount: 24,
    commentsCount: 8,
    isLikedByMe: false,
    createdAt: new Date(Date.now() - 3600000).toISOString(),
  },
  {
    id: "2",
    author: {
      id: "2",
      username: "mariasantos",
      displayName: "Maria Santos",
      avatarUrl: "",
      bio: "Designer UI/UX | Criativa por natureza",
      followersCount: 320,
      followingCount: 120,
      postsCount: 45,
      createdAt: "2024-02-20T14:30:00Z",
    },
    title: "Atualização do Sistema",
    content:
      "Nova atualizacao disponivel! Adicionamos suporte para comentarios e reacoes. Testem e me digam o que acharam!",
    imageUrl: "",
    likesCount: 67,
    commentsCount: 12,
    isLikedByMe: true,
    createdAt: new Date(Date.now() - 7200000).toISOString(),
  },
  {
    id: "3",
    author: {
      id: "3",
      username: "pedrocosta",
      displayName: "Pedro Costa",
      avatarUrl: "",
      bio: "Full-stack developer | Open source enthusiast",
      followersCount: 89,
      followingCount: 67,
      postsCount: 12,
      createdAt: "2024-03-10T09:15:00Z",
    },
    title: "Dicas de Vue.js",
    content:
      "Dicas para melhorar a organizacao dos componentes em Vue.js:\n\n1. Use composables para logica reativa\n2. Separe estilos em arquivos SCSS\n3. Mantenha componentes pequenos e focados\n\n#vuejs #desenvolvimento",
    imageUrl: "",
    likesCount: 45,
    commentsCount: 6,
    isLikedByMe: false,
    createdAt: new Date(Date.now() - 86400000).toISOString(),
  },
  {
    id: "4",
    author: {
      id: "4",
      username: "anamlira",
      displayName: "Ana Lira",
      avatarUrl: "",
      bio: "Tech lead | Mentora de devs juniores",
      followersCount: 512,
      followingCount: 89,
      postsCount: 67,
      createdAt: "2024-01-05T16:45:00Z",
    },
    title: "TypeScript é essencial",
    content:
      "TypeScript e uma ferramenta incrivel para evitar erros comuns. A tipagem forte salva muito tempo de debug no futuro!",
    imageUrl: "",
    likesCount: 89,
    commentsCount: 15,
    isLikedByMe: true,
    createdAt: new Date(Date.now() - 172800000).toISOString(),
  },
  {
    id: "5",
    author: {
      id: "5",
      username: "lucasalmeida",
      displayName: "Lucas Almeida",
      avatarUrl: "",
      bio: "Estudante de Ciencia da Computacao",
      followersCount: 45,
      followingCount: 120,
      postsCount: 8,
      createdAt: "2024-04-01T11:20:00Z",
    },
    title: "Detalhes fazem a diferença",
    content:
      "Design moderno e sobre detalhes. Pequenos ajustes de espacamento, cores e tipografia fazem uma diferenca gigante na interface final.",
    imageUrl: "",
    likesCount: 34,
    commentsCount: 4,
    isLikedByMe: false,
    createdAt: new Date(Date.now() - 259200000).toISOString(),
  },
];

onMounted(async () => {
  try {
    const feedPosts = await listFeed(1, 50);
    posts.value = feedPosts.map((post) => ({
      id: post.id,
      author: post.author
        ? {
            id: post.author.id,
            username: post.author.username,
            displayName: post.author.displayName,
            avatarUrl: post.author.avatar,
            bio: post.author.bio,
            followersCount: post.author.followersCount,
            followingCount: post.author.followingCount,
            postsCount: post.author.postsCount,
            createdAt: post.author.createdAt,
          }
        : undefined,
      title: post.title,
      content: post.content,
      imageUrl: post.imageUrl,
      likesCount: post.likesCount,
      commentsCount: post.commentsCount,
      isLikedByMe: post.isLikedByMe,
      createdAt: post.createdAt,
    }));
  } catch (e) {
    console.warn("gRPC indisponivel, usando mock data:", e);
    posts.value = mockPosts;
  } finally {
    loading.value = false;
  }
});
</script>

<template>
  <div class="post-feed">
    <div v-if="loading" class="post-feed__loading">
      <q-spinner-dots size="40px" color="accent" />
    </div>

    <div v-else-if="error" class="post-feed__error">
      <q-icon name="error_outline" size="48px" />
      <p>{{ error }}</p>
      <q-btn
        outline
        color="accent"
        label="Tentar novamente"
        @click="loading = true"
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
  max-width: 600px;
  margin: 0 auto;
  padding: 20px;

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
