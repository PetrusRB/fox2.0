import { RpcError } from "@protobuf-ts/runtime-rpc";

export interface AppError {
  code: string;
  message: string;
  details?: string;
}

const GRPC_MESSAGES: Record<number, { code: string; message: string }> = {
  1: { code: "CANCELLED", message: "Requisição cancelada. Tente novamente." },
  2: { code: "UNKNOWN", message: "Erro desconhecido. Tente novamente." },
  3: { code: "INVALID_ARGUMENT", message: "Dados inválidos. Verifique os campos." },
  4: { code: "DEADLINE_EXCEEDED", message: "Servidor demorou para responder. Tente novamente." },
  5: { code: "NOT_FOUND", message: "Recurso não encontrado." },
  7: { code: "PERMISSION_DENIED", message: "Você não tem permissão para esta ação." },
  8: { code: "RESOURCE_EXHAUSTED", message: "Limite de recursos atingido. Aguarde um momento." },
  9: { code: "FAILED_PRECONDITION", message: "Operação não permitida no estado atual." },
  10: { code: "ABORTED", message: "Operação abortada. Tente novamente." },
  11: { code: "OUT_OF_RANGE", message: "Dados fora do intervalo esperado." },
  13: { code: "INTERNAL", message: "Erro interno do servidor." },
  14: { code: "UNAVAILABLE", message: "Serviço indisponível. Verifique sua conexão." },
  15: { code: "DATA_LOSS", message: "Perda de dados inesperada." },
  16: { code: "UNAUTHENTICATED", message: "Sessão expirada. Faça login novamente." },
};

export function parseGrpcError(error: unknown): AppError {
  if (error instanceof RpcError) {
    const mapped = GRPC_MESSAGES[error.code];
    if (mapped) {
      return {
        code: mapped.code,
        message: mapped.message,
        details: error.message,
      };
    }
    return {
      code: "RPC_ERROR",
      message: `Erro do servidor (código ${error.code}).`,
      details: error.message,
    };
  }

  if (error instanceof Error) {
    if (error.message.includes("fetch")) {
      return {
        code: "NETWORK_ERROR",
        message: "Não foi possível conectar ao servidor. Verifique sua conexão.",
        details: error.message,
      };
    }
    return {
      code: "CLIENT_ERROR",
      message: "Erro inesperado. Tente novamente.",
      details: error.message,
    };
  }

  return {
    code: "UNKNOWN",
    message: "Erro inesperado. Tente novamente.",
  };
}
