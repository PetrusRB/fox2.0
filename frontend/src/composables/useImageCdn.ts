import { upload as imagekitUpload } from "@imagekit/javascript";
import { ref } from "vue";
import { z } from "zod";

const IMAGEKIT_PUBLIC_KEY = import.meta.env.VITE_IMAGEKIT_PUBLIC_KEY as string;
const SIGNATURE_ENDPOINT = import.meta.env
  .VITE_IMAGEKIT_SIGNATURE_ENDPOINT as string;

const SIGNATURE_BASE = SIGNATURE_ENDPOINT.replace(
  /\/api\/imagekit\/signature$/,
  "",
);

const SignatureSchema = z.object({
  signature: z.string().min(1),
  token: z.string().min(1),
  expire: z.number().positive(),
});

type CachedSignature = z.infer<typeof SignatureSchema>;

export interface UploadProgress {
  loaded: number;
  total: number;
  percent: number;
}

export interface UploadResult {
  url: string;
  fileId: string;
  filePath: string;
  name: string;
  width?: number;
  height?: number;
  size?: number;
  thumbnailUrl?: string;
}

const MAX_RETRIES = 3;
const RETRY_BASE_DELAY_MS = 500;

async function fetchSignatureWithRetry(attempt = 0): Promise<CachedSignature> {
  const res = await fetch(SIGNATURE_ENDPOINT);
  if (!res.ok) {
    if (attempt < MAX_RETRIES) {
      const delay = RETRY_BASE_DELAY_MS * 2 ** attempt;
      await new Promise((r) => setTimeout(r, delay));
      return fetchSignatureWithRetry(attempt + 1);
    }
    throw new Error(
      "Falha ao obter assinatura de upload apos varias tentativas",
    );
  }

  const data = await res.json();
  const parsed = SignatureSchema.safeParse(data);
  if (!parsed.success) {
    throw new Error("Resposta de assinatura com formato invalido");
  }
  return parsed.data;
}

async function computeFileHash(file: File): Promise<string> {
  const buffer = await file.arrayBuffer();
  const hashBuffer = await crypto.subtle.digest("SHA-256", buffer);
  const hashArray = Array.from(new Uint8Array(hashBuffer));
  return hashArray.map((b) => b.toString(16).padStart(2, "0")).join("");
}

async function checkExistingImage(
  hash: string,
): Promise<{ url: string; fileId: string } | null> {
  try {
    const res = await fetch(
      `${SIGNATURE_BASE}/api/imagekit/check?hash=${encodeURIComponent(hash)}`,
    );
    if (!res.ok) return null;
    const data = await res.json();
    if (data.found && data.url && data.file_id) {
      return { url: data.url, fileId: data.file_id };
    }
  } catch {}
  return null;
}

async function registerImageHash(
  hash: string,
  url: string,
  fileId: string,
): Promise<void> {
  try {
    await fetch(`${SIGNATURE_BASE}/api/imagekit/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ hash, url, file_id: fileId }),
    });
  } catch {}
}

export function useImageCdn() {
  const uploading = ref(false);
  const progress = ref<UploadProgress>({ loaded: 0, total: 0, percent: 0 });
  const error = ref<string | null>(null);

  let cachedSignature: CachedSignature | null = null;
  let abortController: AbortController | null = null;

  function isSignatureValid(sig: CachedSignature): boolean {
    return Date.now() / 1000 < sig.expire - 120;
  }

  async function getSignature(): Promise<CachedSignature> {
    if (cachedSignature && isSignatureValid(cachedSignature)) {
      return cachedSignature;
    }
    cachedSignature = await fetchSignatureWithRetry();
    return cachedSignature;
  }

  async function upload(
    file: File,
    options?: { folder?: string; tags?: string[] },
  ): Promise<UploadResult> {
    uploading.value = true;
    error.value = null;
    progress.value = { loaded: 0, total: 0, percent: 0 };

    abortController = new AbortController();

    try {
      const fileHash = await computeFileHash(file);

      const existing = await checkExistingImage(fileHash);
      if (existing) {
        progress.value = { loaded: 1, total: 1, percent: 100 };
        return {
          url: existing.url,
          fileId: existing.fileId,
          filePath: "",
          name: file.name,
        };
      }

      const { signature, token, expire } = await getSignature();
      const fileName = file.name.replace(/[^a-zA-Z0-9._-]/g, "_");

      const response = await imagekitUpload({
        file,
        fileName,
        publicKey: IMAGEKIT_PUBLIC_KEY,
        signature,
        token,
        expire,
        ...(options?.folder ? { folder: options.folder } : {}),
        ...(options?.tags ? { tags: options.tags } : {}),
        useUniqueFileName: true,
        onProgress: (event: ProgressEvent) => {
          progress.value.loaded = event.loaded;
          progress.value.total = event.total;
          progress.value.percent =
            event.total > 0
              ? Math.round((event.loaded / event.total) * 100)
              : 0;
        },
        abortSignal: abortController.signal,
      });

      const resultUrl = response.url ?? "";
      const resultFileId = response.fileId ?? "";

      await registerImageHash(fileHash, resultUrl, resultFileId);

      return {
        url: resultUrl,
        fileId: resultFileId,
        filePath: response.filePath ?? "",
        name: response.name ?? fileName,
        ...(response.width != null ? { width: response.width } : {}),
        ...(response.height != null ? { height: response.height } : {}),
        ...(response.size != null ? { size: response.size } : {}),
        ...(response.thumbnailUrl != null
          ? { thumbnailUrl: response.thumbnailUrl }
          : {}),
      };
    } catch (e: any) {
      if (e?.name === "AbortError" || e?.name === "ImageKitAbortError") {
        error.value = "Upload cancelado";
      } else {
        error.value = e?.message || "Erro ao fazer upload";
      }
      throw e;
    } finally {
      uploading.value = false;
      abortController = null;
    }
  }

  function abort() {
    abortController?.abort();
  }

  function reset() {
    uploading.value = false;
    progress.value = { loaded: 0, total: 0, percent: 0 };
    error.value = null;
    abortController = null;
  }

  return {
    uploading,
    progress,
    error,
    upload,
    abort,
    reset,
  };
}
