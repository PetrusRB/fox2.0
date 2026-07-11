<div align="center">

<img src="https://img.shields.io/badge/Fox-2.0-65a30d?style=for-the-badge&labelColor=09090b&color=65a30d" />

<h1>Fox 2.0</h1>

**Uma rede social para expressar ideias e criatividade, construída para a comunidade Gamer.**

<br/>

<img src="https://img.shields.io/badge/Vue-3-42b883?style=flat-square&logo=vue.js&logoColor=white" />
<img src="https://img.shields.io/badge/Quasar-3-1976D2?style=flat-square&logo=quasar&logoColor=white" />
<img src="https://img.shields.io/badge/TypeScript-6.0-3178C6?style=flat-square&logo=typescript&logoColor=white" />
<img src="https://img.shields.io/badge/gRPC-C++-244c5a?style=flat-square&logo=grpc&logoColor=white" />
<img src="https://img.shields.io/badge/Protobuf-3-4A5568?style=flat-square&logo=protocolbuffers&logoColor=white" />
<img src="https://img.shields.io/badge/Google-OAuth-4285F4?style=flat-square&logo=google&logoColor=white" />

</div>

---

<br/>

## Visão Geral

Fox 2.0 é uma aplicação full-stack com arquitetura cliente-servidor usando **gRPC-Web**. A backend expõe serviços gRPC e o frontend se comunica via protobuf-ts, e toda a autenticação roda pelo Google OAuth.

<br/>

## Stack

<table>
<tr>
<td><b>Frontend</b></td>
<td>Vue 3, Quasar 3, TypeScript 6, Pinia, vue-router 5, vue-i18n, Zod, protobuf-ts, gRPC-Web</td>
</tr>
<tr>
<td><b>Backend</b></td>
<td>C++17, gRPC, Protobuf, libcurl, Abseil, CMake</td>
</tr>
<tr>
<td><b>Protocolo</b></td>
<td>gRPC-Web (HTTP/1.1 transport), Protocol Buffers 3</td>
</tr>
<tr>
<td><b>Auth</b></td>
<td>Google OAuth 2.0 (Authorization Code + PKCE)</td>
</tr>
<tr>

<br/>

## Arquitetura

```
┌─────────────────────────────────────────────────────────┐
│                      Frontend (SPA)                      │
│  Vue 3 + Quasar 3 + TypeScript + protobuf-ts + Zod      │
│                                                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │  NavBar   │  │ SidePanel│  │  Posts   │  │  Auth   │ │
│  │          │  │          │  │          │  │         │ │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘ │
└──────────────────────┬──────────────────────────────────┘
                       │  gRPC-Web (protobuf)
                       ▼
┌─────────────────────────────────────────────────────────┐
│                    Backend (C++17)                       │
│  gRPC Server + Protobuf + Abseil + libcurl              │
│                                                          │
│  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐ │
│  │  AuthService   │ │  PostService  │ │ UserService   │ │
│  │  Login/OAuth   │ │  CRUD Posts   │ │  Profile      │ │
│  └───────────────┘ └───────────────┘ └───────────────┘ │
│  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐ │
│  │InteractionSvc │ │ SocialService │ │NotificationSvc│ │
│  │ Likes/Comments│ │ Follow/Unfollow│ │  Realtime     │ │
│  └───────────────┘ └───────────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────┘
```

<br/>

## Features

### Autenticação

- Login com Google OAuth 2.0
- Tokens armazenados em cookies (access + refresh)
- Sessão restaurada automaticamente ao recarregar
- Middleware gRPC valida tokens em cada request

### Posts

- Criar posts com título, conteúdo e imagem (URL)
- Feed paginado com infinite scroll
- Posts por usuário (perfil)
- Deletar posts (somente autor)
- Sanitização de XSS e URLs no backend

### Interações

- Like / Unlike em posts com contador em tempo real
- Comentários com paginação
- Contadores de likes e comentários atualizados

### Social

- Seguir / Deixar de seguir usuários
- Lista de seguidores e seguindo
- Busca de usuários
- Perfis com bio, avatar, contadores

### Notificações

- Lista de notificações (likes, comentários, follows)
- Marcar como lida
- Watch Notifications via **gRPC server streaming**

<br/>

## Serviços gRPC

<table>
<tr><td><b>Serviço</b></td><td><b>RPCs</b></td></tr>
<tr><td><code>AuthService</code></td><td>Login</td></tr>
<tr><td><code>PostService</code></td><td>CreatePost, GetPost, DeletePost, ListFeed, ListUserPosts</td></tr>
<tr><td><code>InteractionService</code></td><td>ToggleLike, AddComment, DeleteComment, ListLikes, ListComments</td></tr>
<tr><td><code>SocialService</code></td><td>ToggleFollow, GetFollowers, GetFollowing</td></tr>
<tr><td><code>UserService</code></td><td>GetProfile, UpdateProfile, SearchUsers</td></tr>
<tr><td><code>NotificationService</code></td><td>ListNotifications, MarkAsRead, WatchNotifications (stream)</td></tr>
</table>

<br/>

## Como Rodar

### Pré-requisitos

- Node.js >= 22
- CMake >= 3.14
- gRPC + Protobuf (C++)
- libcurl
- Variáveis de ambiente: `GOOGLE_CLIENT_ID`, `GOOGLE_CLIENT_SECRET`, `VITE_GOOGLE_CLIENT_ID`, `VITE_GRPC_HOST`

### Frontend

```bash
cd frontend
pnpm install
pnpm web
```

### Backend

```bash
.\b.bat --proxy (automaticamente já compila, gerar os arquivos necessarios e roda)
```

<br/>

---

<div align="center">

Criado pelo **PetrusRB**

</div>
