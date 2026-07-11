<div align="center">

<img src="https://img.shields.io/badge/Fox-2.0-65a30d?style=for-the-badge&labelColor=09090b&color=65a30d" />

<h1 style="color:#fafafa; font-size:2.4em; margin:8px 0 4px;">Fox 2.0</h1>

<p style="color:#a1a1aa; font-size:1.05em; margin-bottom:16px;">Uma rede social para expressar ideias e criatividade, construída para a comunidade Gamer.</p>

<img src="https://img.shields.io/badge/Vue-3-42b883?style=flat-square&logo=vue.js&logoColor=white" />
<img src="https://img.shields.io/badge/Quasar-3-1976D2?style=flat-square&logo=quasar&logoColor=white" />
<img src="https://img.shields.io/badge/TypeScript-6.0-3178C6?style=flat-square&logo=typescript&logoColor=white" />
<img src="https://img.shields.io/badge/gRPC-C++-244c5a?style=flat-square&logo=grpc&logoColor=white" />
<img src="https://img.shields.io/badge/Protobuf-3-4A5568?style=flat-square&logo=protocolbuffers&logoColor=white" />
<img src="https://img.shields.io/badge/Google-OAuth-4285F4?style=flat-square&logo=google&logoColor=white" />

</div>

<br/>

---

<br/>

## Visão Geral

<p style="color:#a1a1aa; line-height:1.7;">
Fox 2.0 é uma aplicação full-stack com arquitetura cliente-servidor usando <b style="color:#a3e635;">gRPC-Web</b>. A backend expõe serviços gRPC, o frontend se comunica via protobuf-ts, e toda a autenticação roda pelo <b style="color:#4285F4;">Google OAuth</b>.
</p>

<br/>

## Stack

<table style="width:100%; border-collapse:collapse; background:#18181b; border:1px solid #3f3f46; border-radius:8px; overflow:hidden;">
<thead>
<tr style="background:#27272a; border-bottom:1px solid #3f3f46;">
  <th style="padding:10px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">Camada</th>
  <th style="padding:10px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">Tecnologias</th>
</tr>
</thead>
<tbody>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px; color:#fafafa; font-weight:600;">Frontend</td>
  <td style="padding:10px 16px; color:#d4d4d8;">Vue 3, Quasar 3, TypeScript 6, Pinia, vue-router 5, vue-i18n, Zod, protobuf-ts, gRPC-Web</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px; color:#fafafa; font-weight:600;">Backend</td>
  <td style="padding:10px 16px; color:#d4d4d8;">C++17, gRPC, Protobuf, libcurl, Abseil, CMake</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px; color:#fafafa; font-weight:600;">Protocolo</td>
  <td style="padding:10px 16px; color:#d4d4d8;">gRPC-Web (HTTP/1.1 transport), Protocol Buffers 3</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px; color:#fafafa; font-weight:600;">Auth</td>
  <td style="padding:10px 16px; color:#d4d4d8;">Google OAuth 2.0 (Authorization Code + PKCE)</td>
</tr>
<tr>
  <td style="padding:10px 16px; color:#fafafa; font-weight:600;">Tooling</td>
  <td style="padding:10px 16px; color:#d4d4d8;">oxlint, oxfmt, vue-tsc, protoc-gen-grpc-web</td>
</tr>
</tbody>
</table>

<br/>

## Arquitetura

<pre style="background:#18181b; border:1px solid #3f3f46; border-radius:8px; padding:20px; color:#d4d4d8; font-size:13px; line-height:1.5; overflow-x:auto;">
┌─────────────────────────────────────────────────────────────┐
│                     <span style="color:#a3e635; font-weight:700;">Frontend (SPA)</span>                          │
│   Vue 3 + Quasar 3 + TypeScript + protobuf-ts + Zod        │
│                                                             │
│   ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌────────┐ │
│   │  <span style="color:#fafafa;">NavBar</span>    │  │ <span style="color:#fafafa;">SidePanel</span> │  │  <span style="color:#fafafa;">Posts</span>    │  │ <span style="color:#fafafa;">Auth</span>   │ │
│   └───────────┘  └───────────┘  └───────────┘  └────────┘ │
└─────────────────────────┬───────────────────────────────────┘
                          │  <span style="color:#a3e635;">gRPC-Web (protobuf)</span>
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   <span style="color:#a3e635; font-weight:700;">Backend (C++17)</span>                           │
│   gRPC Server + Protobuf + Abseil + libcurl                 │
│                                                             │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│   │ <span style="color:#fafafa;">AuthService</span>  │  │ <span style="color:#fafafa;">PostService</span> │  │ <span style="color:#fafafa;">UserService</span> │    │
│   │ Login/OAuth  │  │ CRUD Posts   │  │  Profile     │    │
│   └──────────────┘  └──────────────┘  └──────────────┘    │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│   │ <span style="color:#fafafa;">Interaction</span> │  │ <span style="color:#fafafa;">Social</span>      │  │ <span style="color:#fafafa;">Notification</span>│    │
│   │ Likes/Comms  │  │ Follow       │  │  Realtime    │    │
│   └──────────────┘  └──────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────────┘
</pre>

<br/>

## Features

<table style="width:100%; border-collapse:collapse; background:#18181b; border:1px solid #3f3f46; border-radius:8px; overflow:hidden;">
<thead>
<tr style="background:#27272a; border-bottom:1px solid #3f3f46;">
  <th style="padding:12px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">Módulo</th>
  <th style="padding:12px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">Funcionalidades</th>
</tr>
</thead>
<tbody>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:12px 16px;">
    <span style="background:rgba(66,133,244,0.15); color:#4285F4; padding:3px 10px; border-radius:4px; font-size:13px; font-weight:600;">Auth</span>
  </td>
  <td style="padding:12px 16px; color:#d4d4d8; line-height:1.6;">
    Login com Google OAuth 2.0 &nbsp;·&nbsp; Tokens em cookies (access + refresh) &nbsp;·&nbsp; Sessão restaurada automaticamente &nbsp;·&nbsp; Middleware gRPC valida tokens
  </td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:12px 16px;">
    <span style="background:rgba(101,163,13,0.15); color:#a3e635; padding:3px 10px; border-radius:4px; font-size:13px; font-weight:600;">Posts</span>
  </td>
  <td style="padding:12px 16px; color:#d4d4d8; line-height:1.6;">
    Criar posts com título, conteúdo e imagem &nbsp;·&nbsp; Feed paginado &nbsp;·&nbsp; Posts por perfil &nbsp;·&nbsp; Delete (somente autor) &nbsp;·&nbsp; Sanitização XSS
  </td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:12px 16px;">
    <span style="background:rgba(239,68,68,0.15); color:#ef4444; padding:3px 10px; border-radius:4px; font-size:13px; font-weight:600;">Interações</span>
  </td>
  <td style="padding:12px 16px; color:#d4d4d8; line-height:1.6;">
    Like / Unlike com contador &nbsp;·&nbsp; Comentários com paginação &nbsp;·&nbsp; Contadores atualizados em tempo real
  </td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:12px 16px;">
    <span style="background:rgba(168,85,247,0.15); color:#a855f7; padding:3px 10px; border-radius:4px; font-size:13px; font-weight:600;">Social</span>
  </td>
  <td style="padding:12px 16px; color:#d4d4d8; line-height:1.6;">
    Seguir / Deixar de seguir &nbsp;·&nbsp; Lista de seguidores e seguindo &nbsp;·&nbsp; Busca de usuários &nbsp;·&nbsp; Perfis com bio e avatar
  </td>
</tr>
<tr>
  <td style="padding:12px 16px;">
    <span style="background:rgba(234,179,8,0.15); color:#eab308; padding:3px 10px; border-radius:4px; font-size:13px; font-weight:600;">Notificações</span>
  </td>
  <td style="padding:12px 16px; color:#d4d4d8; line-height:1.6;">
    Lista de notificações (likes, comments, follows) &nbsp;·&nbsp; Marcar como lida &nbsp;·&nbsp; Server streaming via gRPC
  </td>
</tr>
</tbody>
</table>

<br/>

## Serviços gRPC

<table style="width:100%; border-collapse:collapse; background:#18181b; border:1px solid #3f3f46; border-radius:8px; overflow:hidden;">
<thead>
<tr style="background:#27272a; border-bottom:1px solid #3f3f46;">
  <th style="padding:10px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">Serviço</th>
  <th style="padding:10px 16px; text-align:left; color:#a1a1aa; font-size:12px; letter-spacing:0.05em; text-transform:uppercase;">RPCs</th>
</tr>
</thead>
<tbody>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">AuthService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">Login</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">PostService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">CreatePost, GetPost, DeletePost, ListFeed, ListUserPosts</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">InteractionService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">ToggleLike, AddComment, DeleteComment, ListLikes, ListComments</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">SocialService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">ToggleFollow, GetFollowers, GetFollowing</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">UserService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">GetProfile, UpdateProfile, SearchUsers</td>
</tr>
<tr>
  <td style="padding:10px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:13px;">NotificationService</code></td>
  <td style="padding:10px 16px; color:#d4d4d8;">ListNotifications, MarkAsRead, WatchNotifications <span style="background:rgba(234,179,8,0.15); color:#eab308; padding:1px 6px; border-radius:3px; font-size:11px; font-weight:600;">stream</span></td>
</tr>
</tbody>
</table>

<br/>

## Como Rodar

### Pré-requisitos

<table style="border-collapse:collapse; background:#18181b; border:1px solid #3f3f46; border-radius:8px; overflow:hidden; width:100%;">
<tbody>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:8px 16px; color:#a1a1aa;">Node.js</td>
  <td style="padding:8px 16px; color:#fafafa; font-weight:500;">>= 22</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:8px 16px; color:#a1a1aa;">CMake</td>
  <td style="padding:8px 16px; color:#fafafa; font-weight:500;">>= 3.14</td>
</tr>
<tr style="border-bottom:1px solid #27272a;">
  <td style="padding:8px 16px; color:#a1a1aa;">C++ Libs</td>
  <td style="padding:8px 16px; color:#fafafa; font-weight:500;">gRPC, Protobuf, libcurl, Abseil</td>
</tr>
<tr>
  <td style="padding:8px 16px; color:#a1a1aa;">Env Vars</td>
  <td style="padding:8px 16px;"><code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:12px;">GOOGLE_CLIENT_ID</code> <code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:12px;">GOOGLE_CLIENT_SECRET</code> <code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:12px;">VITE_GOOGLE_CLIENT_ID</code> <code style="background:#27272a; color:#a3e635; padding:2px 8px; border-radius:4px; font-size:12px;">VITE_GRPC_HOST</code></td>
</tr>
</tbody>
</table>

<br/>

### Frontend

```bash
cd frontend
pnpm install
pnpm web
```

### Backend

```bash
.\b.bat --proxy
```

<br/>

---

<br/>

<div align="center" style="padding:24px 0;">

<p style="color:#52525b; font-size:13px; margin-bottom:8px;">Criado por </p>

<img src="https://avatars.githubusercontent.com/u/179767454?s=48&v=4" />

</div>
