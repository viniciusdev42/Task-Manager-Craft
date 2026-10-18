# syntax=docker/dockerfile:1

# =============================================================================
# Task Manager Craft - Dockerfile multi-stage
#
# Requisitos extraídos do CMakeLists.txt:
#   - CMake >= 3.21
#   - GCC >= 11 (C++20)
#   - Qt6 >= 6.5 (Widgets; Test apenas quando TMC_BUILD_TESTS=ON)
#
# Debian 13 (trixie) traz GCC 14, CMake 3.31 e Qt 6.8, atendendo a tudo.
# (Ubuntu 24.04 NÃO serve: traz Qt 6.4, abaixo do mínimo exigido.)
#
# Estágios:
#   deps     -> toolchain + Qt6 (cache de dependências)
#   build    -> compila o executável (Release, sem testes)
#   test     -> compila e roda os testes (ctest, QT_QPA_PLATFORM=offscreen)
#   runtime  -> imagem final enxuta (padrão)
#
# Uso:
#   docker build -t taskmanagercraft .
#   docker build --target test -t taskmanagercraft:test .
# =============================================================================

ARG DEBIAN_VERSION=trixie-slim

# -----------------------------------------------------------------------------
# deps: ferramentas de compilação e Qt6
# -----------------------------------------------------------------------------
FROM debian:${DEBIAN_VERSION} AS deps

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        qt6-base-dev \
        qt6-base-dev-tools \
        libgl1-mesa-dev \
        libxkbcommon-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# -----------------------------------------------------------------------------
# build: executável principal (Release, sem testes)
# -----------------------------------------------------------------------------
FROM deps AS build

ARG BUILD_TYPE=Release

COPY . /src

# TMC_BUILD_TESTS=OFF: Qt6::Test não é necessário e o build fica mais rápido.
RUN cmake -S . -B /build -G Ninja \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DTMC_BUILD_TESTS=OFF \
        -DTMC_BUILD_BENCHMARKS=OFF \
 && cmake --build /build --target TaskManagerCraft --parallel

# -----------------------------------------------------------------------------
# test: compila tudo com testes e executa o CTest
# -----------------------------------------------------------------------------
FROM deps AS test

COPY . /src

RUN cmake -S . -B /build-test -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DTMC_BUILD_TESTS=ON \
        -DTMC_BUILD_BENCHMARKS=OFF \
 && cmake --build /build-test --parallel

# Os testes já definem QT_QPA_PLATFORM=offscreen via CMake; repetimos por segurança.
ENV QT_QPA_PLATFORM=offscreen
CMD ["ctest", "--test-dir", "/build-test", "--output-on-failure", "--parallel"]

# -----------------------------------------------------------------------------
# runtime: imagem final (apenas bibliotecas de execução do Qt)
# -----------------------------------------------------------------------------
FROM debian:${DEBIAN_VERSION} AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Nomes com sufixo t64 seguem a convenção do Debian 13.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        libqt6core6t64 \
        libqt6gui6t64 \
        libqt6widgets6t64 \
        libxcb-cursor0 \
        libxkbcommon-x11-0 \
        libgl1 \
        libegl1 \
        fonts-dejavu-core \
 && rm -rf /var/lib/apt/lists/*

# Usuário sem privilégios
RUN useradd --create-home --shell /usr/sbin/nologin tmc

# Copiamos o binário direto (em vez de "cmake --install") porque o script de
# deploy do Qt tenta empacotar o Qt do sistema, o que é desnecessário aqui:
# a imagem já contém as bibliotecas Qt de execução.
COPY --from=build /build/TaskManagerCraft /usr/local/bin/TaskManagerCraft

USER tmc
WORKDIR /home/tmc

ENTRYPOINT ["/usr/local/bin/TaskManagerCraft"]

# -----------------------------------------------------------------------------
# Exemplo de execução (Linux com X11):
#
#   xhost +local:docker
#   docker run --rm \
#       -e DISPLAY=$DISPLAY \
#       -v /tmp/.X11-unix:/tmp/.X11-unix \
#       taskmanagercraft
#
# Sem display (apenas verificar que o app sobe):
#   docker run --rm -e QT_QPA_PLATFORM=offscreen taskmanagercraft
# -----------------------------------------------------------------------------
