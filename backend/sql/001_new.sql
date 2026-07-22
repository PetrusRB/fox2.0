-- Tabelas
CREATE TABLE IF NOT EXISTS users (
  id            text PRIMARY KEY,
  username      text NOT NULL DEFAULT '',
  handle        text NOT NULL DEFAULT '',
  display_name  text NOT NULL DEFAULT '',
  avatar        text NOT NULL DEFAULT '',
  bio           text NOT NULL DEFAULT '',
  followers_count   int NOT NULL DEFAULT 0,
  following_count   int NOT NULL DEFAULT 0,
  posts_count       int NOT NULL DEFAULT 0,
  created_at    timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS posts (
  id            text PRIMARY KEY DEFAULT gen_random_uuid()::text,
  author_id     text NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  title         text NOT NULL,
  content       text NOT NULL,
  image_url     text NOT NULL DEFAULT '',
  likes_count   int NOT NULL DEFAULT 0,
  comments_count int NOT NULL DEFAULT 0,
  created_at    timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS likes (
  user_id  text NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  post_id  text NOT NULL REFERENCES posts(id) ON DELETE CASCADE,
  created_at timestamptz NOT NULL DEFAULT now(),
  PRIMARY KEY (user_id, post_id)
);

-- Índices
CREATE INDEX IF NOT EXISTS idx_posts_created_at ON posts (created_at DESC);
CREATE INDEX IF NOT EXISTS idx_posts_author_id ON posts (author_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_likes_post_id ON likes (post_id);
CREATE INDEX IF NOT EXISTS idx_likes_user_id ON likes (user_id);

-- rpc's (importantisimos, muito muito muito mesmo kk)
CREATE OR REPLACE FUNCTION get_feed(p_user_id text, p_limit int, p_offset int)
RETURNS SETOF json AS $$
  SELECT json_build_object(
    'id', p.id,
    'title', p.title,
    'content', p.content,
    'image_url', p.image_url,
    'likes_count', p.likes_count,
    'comments_count', p.comments_count,
    'created_at', p.created_at,
    'author', json_build_object(
      'id', u.id, 'username', u.username, 'handle', u.handle,
      'display_name', u.display_name, 'avatar', u.avatar,
      'bio', u.bio, 'followers_count', u.followers_count,
      'following_count', u.following_count, 'posts_count', u.posts_count,
      'created_at', u.created_at
    ),
    'is_liked_by_me', EXISTS(
      SELECT 1 FROM likes l WHERE l.post_id = p.id AND l.user_id = p_user_id
    )
  )
  FROM posts p
  JOIN users u ON u.id = p.author_id
  ORDER BY p.created_at DESC
  LIMIT p_limit OFFSET p_offset;
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION toggle_like(p_user_id text, p_post_id text)
RETURNS json AS $$
DECLARE
  v_liked boolean;
  v_count int;
BEGIN
  IF EXISTS (SELECT 1 FROM likes WHERE user_id = p_user_id AND post_id = p_post_id) THEN
    DELETE FROM likes WHERE user_id = p_user_id AND post_id = p_post_id;
    UPDATE posts SET likes_count = GREATEST(likes_count - 1, 0) WHERE id = p_post_id;
    v_liked := false;
  ELSE
    INSERT INTO likes (user_id, post_id) VALUES (p_user_id, p_post_id);
    UPDATE posts SET likes_count = likes_count + 1 WHERE id = p_post_id;
    v_liked := true;
  END IF;

  SELECT likes_count INTO v_count FROM posts WHERE id = p_post_id;

  RETURN json_build_object(
    'is_liked', v_liked,
    'likes_count', v_count
  );
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_user_posts(p_user_id text, p_author_id text, p_limit int, p_offset int)
RETURNS SETOF json AS $$
  SELECT json_build_object(
    'id', p.id,
    'title', p.title,
    'content', p.content,
    'image_url', p.image_url,
    'likes_count', p.likes_count,
    'comments_count', p.comments_count,
    'created_at', p.created_at,
    'author', json_build_object(
      'id', u.id, 'username', u.username, 'handle', u.handle,
      'display_name', u.display_name, 'avatar', u.avatar,
      'bio', u.bio, 'followers_count', u.followers_count,
      'following_count', u.following_count, 'posts_count', u.posts_count,
      'created_at', u.created_at
    ),
    'is_liked_by_me', EXISTS(
      SELECT 1 FROM likes l WHERE l.post_id = p.id AND l.user_id = p_user_id
    )
  )
  FROM posts p
  JOIN users u ON u.id = p.author_id
  WHERE p.author_id = p_author_id
  ORDER BY p.created_at DESC
  LIMIT p_limit OFFSET p_offset;
$$ LANGUAGE sql;

CREATE OR REPLACE FUNCTION get_post(p_user_id text, p_post_id text)
RETURNS json AS $$
  SELECT json_build_object(
    'id', p.id,
    'title', p.title,
    'content', p.content,
    'image_url', p.image_url,
    'likes_count', p.likes_count,
    'comments_count', p.comments_count,
    'created_at', p.created_at,
    'author', json_build_object(
      'id', u.id, 'username', u.username, 'handle', u.handle,
      'display_name', u.display_name, 'avatar', u.avatar,
      'bio', u.bio, 'followers_count', u.followers_count,
      'following_count', u.following_count, 'posts_count', u.posts_count,
      'created_at', u.created_at
    ),
    'is_liked_by_me', EXISTS(
      SELECT 1 FROM likes l WHERE l.post_id = p.id AND l.user_id = p_user_id
    )
  )
  FROM posts p
  JOIN users u ON u.id = p.author_id
  WHERE p.id = p_post_id;
$$ LANGUAGE sql;
