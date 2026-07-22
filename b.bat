@echo off
setlocal enabledelayedexpansion
title API Build
color 0A

:: Config
set "ROOT=%~dp0"
set "BACKEND=%ROOT%backend"
set "PROTO_DIR=%BACKEND%\proto"
set "BUILD_DIR=%BACKEND%\build"
set "GEN_DIR=%BACKEND%\src\gen"
set "MSYS2=C:\msys64\mingw64\bin"
set "PROTOC=%MSYS2%\protoc.exe"
set "GRPC_PLUGIN=%MSYS2%\grpc_cpp_plugin.exe"
set "PROTOC_GEN_TS=%ROOT%frontend\node_modules\.bin\protoc-gen-ts.cmd"
set "GRPCWEBPROXY=%ROOT%proxios.exe"
set "DEPLOY=%ROOT%deploy"

:: Parse args
set "MODE=full"
set "NOBUILD=0"
:parse_args
if "%~1"=="" goto :done_args
if "%~1"=="--direct" set "MODE=direct"
if "%~1"=="--proxy" set "MODE=proxy"
if "%~1"=="--prod" set "MODE=prod"
if "%~1"=="--gen" set "MODE=gen"
if "%~1"=="--no-build" set "NOBUILD=1"
shift
goto :parse_args
:done_args

if "%MODE%"=="direct" goto :direct
if "%MODE%"=="proxy" goto :proxy
if "%MODE%"=="prod" goto :prod
if "%MODE%"=="gen" goto :gen

:: =========================================================
::  FULL BUILD
:: =========================================================
echo ========================================
echo          Crown API - Build (v1.0)
echo ========================================
echo.

:: Check deps
if not exist "%PROTOC%" (
    echo [ERRO] protoc nao encontrado!
    echo   pacman -S mingw-w64-x86_64-protobuf
    pause
    exit /b 1
)
if not exist "%GRPC_PLUGIN%" (
    echo [ERRO] grpc_cpp_plugin nao encontrado!
    echo   pacman -S mingw-w64-x86_64-grpc
    pause
    exit /b 1
)

:gen
:: 1. Proto backend
echo [1/5] Gerando proto backend (C++)...
if not exist "%GEN_DIR%" mkdir "%GEN_DIR%"
"%PROTOC%" --grpc_out="%GEN_DIR%" --cpp_out="%GEN_DIR%" -I "%PROTO_DIR%" --plugin=protoc-gen-grpc="%GRPC_PLUGIN%" "%PROTO_DIR%\social.proto"
if !errorlevel! neq 0 (
    echo [ERRO] Falha ao gerar proto backend.
    pause
    exit /b 1
)
echo [OK] Proto backend pronto.
echo.

:: 2. Proto frontend
echo [2/5] Gerando proto frontend (TypeScript)...
set "FRONTEND_GEN=%ROOT%frontend\src\proto"
if not exist "%FRONTEND_GEN%" mkdir "%FRONTEND_GEN%"

if exist "%PROTOC_GEN_TS%" (
    "%PROTOC%" --ts_out="%FRONTEND_GEN%" -I "%PROTO_DIR%" --plugin=protoc-gen-ts="%PROTOC_GEN_TS%" "%PROTO_DIR%\social.proto"
    if !errorlevel! neq 0 (
        echo [ERRO] Falha ao gerar proto frontend.
        pause
        exit /b 1
    )
    echo [OK] Proto frontend pronto.
) else (
    echo [AVISO] protoc-gen-ts nao encontrado, pulando frontend.
)
echo.

if "%MODE%"=="gen" (
    echo ========================================
    echo    Protos gerados com sucesso!
    echo ========================================
    pause
    exit /b 0
)

if "%NOBUILD%"=="1" goto :run

:: 3. CMake
echo [3/5] Configurando CMake...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "%BACKEND%" > "%BUILD_DIR%\cmake.log" 2>&1
if !errorlevel! neq 0 (
    echo [ERRO] Falha no CMake. Ultimas linhas do log:
    echo ----------------------------------------
    powershell -NoProfile -Command "Get-Content -Tail 40 '%BUILD_DIR%\cmake.log'"
    echo ----------------------------------------
    echo Log completo em: %BUILD_DIR%\cmake.log
    pause
    exit /b 1
)
echo [OK] CMake configurado.
echo.

:: 4. Build
echo [4/5] Compilando...
ninja > "%BUILD_DIR%\build.log" 2>&1
if !errorlevel! neq 0 (
    echo [ERRO] Falha na compilacao. Ultimas linhas do log:
    echo ----------------------------------------
    powershell -NoProfile -Command "Get-Content -Tail 40 '%BUILD_DIR%\build.log'"
    echo ----------------------------------------
    echo Log completo em: %BUILD_DIR%\build.log
    pause
    exit /b 1
)
echo [OK] Build concluido.
echo.

:: 5. Run
goto :run

:: =========================================================
::  DIRECT MODE (skip build, just run server)
:: =========================================================
:direct
echo ========================================
echo    Executando server gRPC...
echo    Ctrl+C para parar
echo ========================================
echo.
goto :run

:: =========================================================
::  PROXY MODE (build + grpcwebproxy + server) - DEV
:: =========================================================
:proxy
if not exist "%GRPCWEBPROXY%" (
    echo [ERRO] proxios.exe nao encontrado!
    pause
    exit /b 1
)

echo ========================================
echo    Crown API - Proxy Mode (v1.0)
echo ========================================
echo.

if "%NOBUILD%"=="1" goto :proxy_start

:: Check deps
if not exist "%PROTOC%" (
    echo [ERRO] protoc nao encontrado!
    echo   pacman -S mingw-w64-x86_64-protobuf
    pause
    exit /b 1
)
if not exist "%GRPC_PLUGIN%" (
    echo [ERRO] grpc_cpp_plugin nao encontrado!
    echo   pacman -S mingw-w64-x86_64-grpc
    pause
    exit /b 1
)

:: 1. Proto backend
echo [1/5] Gerando proto backend (C++)...
if not exist "%GEN_DIR%" mkdir "%GEN_DIR%"
"%PROTOC%" --grpc_out="%GEN_DIR%" --cpp_out="%GEN_DIR%" -I "%PROTO_DIR%" --plugin=protoc-gen-grpc="%GRPC_PLUGIN%" "%PROTO_DIR%\social.proto"
if !errorlevel! neq 0 (
    echo [ERRO] Falha ao gerar proto backend.
    pause
    exit /b 1
)
echo [OK] Proto backend pronto.
echo.

:: 2. Proto frontend
echo [2/5] Gerando proto frontend (TypeScript)...
set "FRONTEND_GEN=%ROOT%frontend\src\proto"
if not exist "%FRONTEND_GEN%" mkdir "%FRONTEND_GEN%"

if exist "%PROTOC_GEN_TS%" (
    "%PROTOC%" --ts_out="%FRONTEND_GEN%" -I "%PROTO_DIR%" --plugin=protoc-gen-ts="%PROTOC_GEN_TS%" "%PROTO_DIR%\social.proto"
    if !errorlevel! neq 0 (
        echo [ERRO] Falha ao gerar proto frontend. rapaz, agora deu o carai mermo.
        pause
        exit /b 1
    )
    echo [OK] Proto frontend pronto.
) else (
    echo [AVISO] protoc-gen-ts nao encontrado, pulando frontend.
)
echo.

:: 3. CMake
echo [3/5] Configurando CMake...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "%BACKEND%" > "%BUILD_DIR%\cmake.log" 2>&1
if !errorlevel! neq 0 (
    echo [ERRO] Falha no CMake. se lasco rapaz, tomou no olho da jaca. Ultimas linhas do log:
    echo ----------------------------------------
    powershell -NoProfile -Command "Get-Content -Tail 40 '%BUILD_DIR%\cmake.log'"
    echo ----------------------------------------
    echo Log completo em: %BUILD_DIR%\cmake.log
    pause
    exit /b 1
)
echo [OK] CMake configurado.
echo.

:: 4. Build
echo [4/5] Compilando...
ninja > "%BUILD_DIR%\build.log" 2>&1
if !errorlevel! neq 0 (
    echo [ERRO] Falha na compilacao. tomou no olho da jaca. Ultimas linhas do log:
    echo ----------------------------------------
    powershell -NoProfile -Command "Get-Content -Tail 40 '%BUILD_DIR%\build.log'"
    echo ----------------------------------------
    echo Log completo em: %BUILD_DIR%\build.log
    pause
    exit /b 1
)
echo [OK] Build concluido.
echo.

:: 5. Start proxy + server
:proxy_start
echo [5/5] Iniciando o binario do proxy + server...
echo.

echo [1/2] Iniciando binario do proxy...
start "proxios" /min "%GRPCWEBPROXY%" --backend_addr=localhost:50051 --server_http_debug_port=8082 --allow_all_origins --run_tls_server=false
timeout /t 2 /nobreak >nul

echo [2/2] Iniciando servidor gRPC na porta 50051...
echo.
echo ========================================
echo    Proxy:    http://localhost:8082
echo    gRPC:     localhost:50051
echo ========================================
echo.
goto :run

:: =========================================================
::  PROD MODE (Docker)
:: =========================================================
:prod
echo ========================================
echo    Deploy com Docker Compose
echo ========================================
echo.
echo Precisa do docker instalado (obviamente)
pause
exit /b 0

:: =========================================================
::  RUN SERVER
:: =========================================================
:run
cd /d "%ROOT%"
"%BUILD_DIR%\server.exe"
if !errorlevel! neq 0 (
    echo.
    echo [ERRO] Server encerrou com codigo: !errorlevel!
)
pause
