import { MAX_CONTENT_CHARS, MAX_TITLE_CHARS } from "@/config/config";

const HTML_TAG_RE = /<[^>]*>/g;
const DANGEROUS_ATTRS_RE = /\bon\w+\s*=/gi;
const JS_SCHEME_RE = /^\s*javascript\s*:/i;
const DATA_URI_RE = /^\s*data\s*:/i;

export function stripHtml(input: string): string {
  return input.replace(HTML_TAG_RE, "").replace(DANGEROUS_ATTRS_RE, "").trim();
}

export function sanitizeString(input: string, max: number): string {
  return stripHtml(input).slice(0, max);
}

export function sanitizeUrl(input: string): string {
  const trimmed = input.trim();
  if (!trimmed) return "";
  if (JS_SCHEME_RE.test(trimmed)) return "";
  if (DATA_URI_RE.test(trimmed)) return "";
  return trimmed;
}

export function sanitizePost(
  title: string,
  content: string,
  imageUrl?: string,
) {
  return {
    title: sanitizeString(title, MAX_TITLE_CHARS),
    content: sanitizeString(content, MAX_CONTENT_CHARS),
    imageUrl: imageUrl ? sanitizeUrl(imageUrl) : "",
  };
}
