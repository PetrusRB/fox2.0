import { MAX_CONTENT_CHARS, MAX_TITLE_CHARS } from "@/config/config";

const HTML_TAG_RE = /<[^>]*>/g;
const DANGEROUS_ATTRS_RE = /\bon\w+\s*=/gi;
const DANGEROUS_SCHEMES_RE = /^\s*(javascript|vbscript|data|file|blob)\s*:/i;

export function stripHtml(input: string): string {
  return input.replace(HTML_TAG_RE, "").replace(DANGEROUS_ATTRS_RE, "").trim();
}

export function sanitizeString(input: string, max: number): string {
  return stripHtml(input).slice(0, max);
}

export function decodeUnicodeEscapes(input: string): string {
  return input.replace(/(\\|\/)u([0-9a-fA-F]{4})/g, (_, _prefix, hex) =>
    String.fromCharCode(parseInt(hex, 16))
  );
}

export function sanitizeUrl(input: string): string {
  const trimmed = input.trim();
  if (!trimmed) return "";
  const decoded = decodeUnicodeEscapes(trimmed);
  const normalized = decoded.replace(/[\t\n\r]+/g, "");
  if (DANGEROUS_SCHEMES_RE.test(normalized)) return "";
  return decoded;
}

export function sanitizePost(
  title: string,
  content: string,
  imageUrl?: string
) {
  return {
    title: sanitizeString(title, MAX_TITLE_CHARS),
    content: sanitizeString(content, MAX_CONTENT_CHARS),
    imageUrl: imageUrl ? sanitizeUrl(imageUrl) : ""
  };
}
