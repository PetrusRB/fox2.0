import {
  MAX_CONTENT_CHARS,
  MAX_IMAGE_CHARS,
  MAX_TITLE_CHARS,
} from "@/config/config";
import { z } from "zod";

export const CreatePostSchema = z.object({
  title: z
    .string()
    .min(1, "O título não pode estar vazio")
    .max(MAX_TITLE_CHARS, "O título não pode ter mais de 256 caracteres"),
  content: z
    .string()
    .min(1, "O conteúdo não pode estar vazio")
    .max(MAX_CONTENT_CHARS, "O conteúdo não pode ter mais de 5000 caracteres"),
  imageUrl: z
    .string()
    .max(MAX_IMAGE_CHARS, "URL da imagem muito longa")
    .optional()
    .or(z.literal("")),
});

export type CreatePostInput = z.infer<typeof CreatePostSchema>;

export function validateCreatePost(data: unknown) {
  return CreatePostSchema.safeParse(data);
}
