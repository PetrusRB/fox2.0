import { ref } from "vue";

const CDN_ENDPOINT = import.meta.env.VITE_CDN_ENDPOINT as string;

const API_BASE = CDN_ENDPOINT.replace(/\/$/, "");

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

async function computeFileHash(file: File): Promise<string> {
  const buffer = await file.arrayBuffer();
  const hashBuffer = await crypto.subtle.digest("SHA-256", buffer);
  const hashArray = Array.from(new Uint8Array(hashBuffer));
  return hashArray.map(b => b.toString(16).padStart(2, "0")).join("");
}

async function checkExistingImage(
  hash: string
): Promise<{ url: string; fileId: string } | null> {
  try {
    const res = await fetch(
      `${API_BASE}/api/cdn/check?hash=${encodeURIComponent(hash)}`
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
  fileId: string
): Promise<void> {
  try {
    await fetch(`${API_BASE}/api/cdn/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ hash, url, file_id: fileId })
    });
  } catch {}
}

function fileToBase64(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => {
      const dataUrl = reader.result as string;
      const base64 = dataUrl.split(",")[1] ?? "";
      resolve(base64);
    };
    reader.onerror = () => reject(new Error("Falha ao ler arquivo"));
    reader.readAsDataURL(file);
  });
}

export function useImageCdn() {
  const uploading = ref(false);
  const progress = ref<UploadProgress>({ loaded: 0, total: 0, percent: 0 });
  const error = ref<string | null>(null);

  let abortController: AbortController | null = null;

  async function upload(
    file: File,
    options?: { folder?: string; tags?: string[] }
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
          name: file.name
        };
      }

      progress.value = { loaded: 0, total: file.size, percent: 10 };

      const base64 = await fileToBase64(file);

      if (abortController.signal.aborted) {
        throw new DOMException("Upload cancelado", "AbortError");
      }

      progress.value = {
        loaded: Math.round(file.size * 0.3),
        total: file.size,
        percent: 30
      };

      const fileName = file.name.replace(/[^a-zA-Z0-9._-]/g, "_");

      const body: Record<string, unknown> = {
        file: base64,
        fileName
      };
      if (options?.folder) body.folder = options.folder;
      if (options?.tags) body.tags = options.tags;

      const res = await fetch(`${API_BASE}/api/cdn/upload`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
        signal: abortController.signal
      });

      progress.value = {
        loaded: Math.round(file.size * 0.8),
        total: file.size,
        percent: 80
      };

      if (!res.ok) {
        const errData = await res.json().catch(() => null);
        const msg = errData?.error || `Erro ao enviar arquivo (${res.status})`;
        throw new Error(msg);
      }

      const data = await res.json();

      progress.value = { loaded: file.size, total: file.size, percent: 100 };

      await registerImageHash(fileHash, data.url, data.fileId);

      return {
        url: data.url ?? "",
        fileId: data.fileId ?? "",
        filePath: "",
        name: data.name ?? fileName,
        ...(data.width != null ? { width: data.width } : {}),
        ...(data.height != null ? { height: data.height } : {}),
        ...(data.size != null ? { size: data.size } : {}),
        ...(data.thumbnailUrl != null
          ? { thumbnailUrl: data.thumbnailUrl }
          : {})
      };
    } catch (e: any) {
      if (e?.name === "AbortError") {
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
    reset
  };
}
