cmake_minimum_required(VERSION 3.21)

project(TaskManagerCraft
    VERSION 1.0.0
    DESCRIPTION "Task Manager Craft - Gerenciador de Tarefas em C++20 com Qt6 (MVC)."
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Verificação do compilador
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
        message(FATAL_ERROR
            "GCC ${CMAKE_CXX_COMPILER_VERSION} não suporta C++20 de forma "
            "adequada. Use GCC 11 ou superior.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Opções do projeto
# ---------------------------------------------------------------------------
option(TMC_BUILD_TESTS       "Compilar os testes (tests/)"              ON)
option(TMC_BUILD_BENCHMARKS  "Compilar os benchmarks (benchmarks/)"     OFF)
option(TMC_FETCH_DOCTEST     "Baixar o Doctest (FetchContent) se não houver instalação local" ON)
option(TMC_ENABLE_WARNINGS   "Habilitar avisos extras do compilador"    ON)
option(TMC_ENABLE_SANITIZERS "Habilitar ASan/UBSan (GCC/Clang, Debug)"  OFF)

# ---------------------------------------------------------------------------
# Padrão da linguagem
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Com -std=c++20 estrito (extensões OFF), o MinGW-w64 define __STRICT_ANSI__
# e esconde símbolos como M_PI, strdup, fileno etc. Nesse caso usamos
# -std=gnu++20. Em Linux/macOS/MSVC mantemos o padrão estrito.
if(MINGW)
    set(CMAKE_CXX_EXTENSIONS ON)
else()
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------------------------------------------------------------------------
# Qt6 (Qt Widgets) - Qt cuida de MOC, UIC e RCC automaticamente
# ---------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(TMC_QT_COMPONENTS Widgets)
if(TMC_BUILD_TESTS)
    list(APPEND TMC_QT_COMPONENTS Test)
endif()

# qt_standard_project_setup() e qt_generate_deploy_app_script() exigem Qt >= 6.3
# (o deploy script funciona bem a partir do 6.5 LTS). Para GCC >= 11, prefira um
# Qt recente (6.8+), cujos headers já compilam sem erros com o libstdc++ 15.
find_package(Qt6 6.5 REQUIRED COMPONENTS ${TMC_QT_COMPONENTS})
qt_standard_project_setup()

# ---------------------------------------------------------------------------
# Doctest (framework de testes header-only) - só é necessário com os testes
# ---------------------------------------------------------------------------
# 1) Tenta usar uma instalação existente (vcpkg, Conan, apt, brew, etc.).
# 2) Se não achar e TMC_FETCH_DOCTEST=ON, baixa do GitHub via FetchContent.
# O alvo resultante é sempre doctest::doctest.
if(TMC_BUILD_TESTS)
    find_package(doctest 2.4.9 QUIET CONFIG)

    if(doctest_FOUND)
        set(TMC_DOCTEST_ORIGIN "instalado (${doctest_VERSION})")
    elseif(TMC_FETCH_DOCTEST)
        include(FetchContent)

        FetchContent_Declare(doctest
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG        v2.4.11
            GIT_SHALLOW    ON
        )

        # Não compilar os testes do Doctest nem instalá-lo junto com o projeto
        set(DOCTEST_WITH_TESTS              OFF CACHE BOOL "" FORCE)
        set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
        set(DOCTEST_NO_INSTALL              ON  CACHE BOOL "" FORCE)

        # O CMakeLists do Doctest 2.4.11 declara cmake_minimum_required(3.0),
        # recusado pelo CMake >= 4.0. Esta variável contorna isso (é ignorada
        # por versões mais antigas do CMake).
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
        FetchContent_MakeAvailable(doctest)
        unset(CMAKE_POLICY_VERSION_MINIMUM)

        set(TMC_DOCTEST_ORIGIN "baixado (FetchContent, v2.4.11)")
    else()
        message(FATAL_ERROR
            "Doctest não encontrado. Instale-o (ex.: 'apt install doctest-dev', "
            "'vcpkg install doctest'), informe -Ddoctest_DIR=<caminho>, ou "
            "habilite -DTMC_FETCH_DOCTEST=ON para baixá-lo automaticamente.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Funções auxiliares
# ---------------------------------------------------------------------------

# Aplica avisos, asserts da libstdc++ e sanitizers a um alvo compilável.
function(tmc_configure_target TARGET)
    if(TMC_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()

    # Verificações de limites do libstdc++ (std::vector::operator[], etc.)
    # apenas em Debug; não altera o ABI.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_definitions(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
    endif()

    if(TMC_ENABLE_SANITIZERS)
        if(MSVC OR MINGW)
            # GCC no MinGW-w64 não possui ASan/UBSan.
            message(WARNING
                "TMC_ENABLE_SANITIZERS ignorado: ASan/UBSan não são "
                "suportados pelo GCC no MinGW. Use Linux/WSL.")
        else()
            target_compile_options(${TARGET} PRIVATE
                -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${TARGET} PRIVATE
                -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# Cria uma biblioteca estática. Se não houver .cpp (diretórios ainda vazios
# ou só com headers), cria uma INTERFACE library para não quebrar o configure.
#
#   tmc_add_library(<nome> SOURCES <arquivos...> LINK <libs...>)
function(tmc_add_library NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;LINK" ${ARGN})

    if(ARG_SOURCES)
        add_library(${NAME} STATIC ${ARG_SOURCES})
        set(SCOPE PUBLIC)
        tmc_configure_target(${NAME})
    else()
        add_library(${NAME} INTERFACE)
        set(SCOPE INTERFACE)
    endif()

    # Permite includes no formato "model/entities/Task.h"
    target_include_directories(${NAME} ${SCOPE} ${PROJECT_SOURCE_DIR}/src)
    target_compile_features(${NAME} ${SCOPE} cxx_std_20)

    if(ARG_LINK)
        target_link_libraries(${NAME} ${SCOPE} ${ARG_LINK})
    endif()
endfunction()

# Cria um executável de teste para cada tests/<subdir>/*.cpp (Qt Test e/ou
# Doctest + CTest). Cada .cpp precisa ter o seu próprio main(): use QTEST_MAIN
# (Qt Test) ou defina DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN antes de incluir
# <doctest/doctest.h> (Doctest).
#
#   tmc_add_tests(<subdir> <lib-a-linkar>)
function(tmc_add_tests SUBDIR LIB)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
         ${PROJECT_SOURCE_DIR}/tests/${SUBDIR}/*.cpp)

    foreach(TEST_SRC IN LISTS TEST_SOURCES)
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        set(TARGET tmc_test_${SUBDIR}_${TEST_NAME})

        add_executable(${TARGET} ${TEST_SRC})
        target_link_libraries(${TARGET} PRIVATE ${LIB} Qt6::Test doctest::doctest)
        tmc_configure_target(${TARGET})

        add_test(NAME ${SUBDIR}.${TEST_NAME} COMMAND ${TARGET})
        # Permite rodar testes de interface sem display (CI)
        set_tests_properties(${SUBDIR}.${TEST_NAME} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
endfunction()

# ---------------------------------------------------------------------------
# Model: entidades, serviços, repositórios, estruturas de dados e storage
# (hash, árvores AVL/B, trie, grafos, heap, strings, busca digital, pager)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE TMC_MODEL_SOURCES CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/src/model/*.cpp)

tmc_add_library(tmc_model SOURCES ${TMC_MODEL_SOURCES})

# ---------------------------------------------------------------------------
# View + Controller (Qt Widgets)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE TMC_UI_SOURCES CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/src/view/*.cpp
     ${PROJECT_SOURCE_DIR}/src/controller/*.cpp)

# .ui (Qt Designer) em view/ também entram, caso existam
file(GLOB_RECURSE TMC_UI_FORMS CONFIGURE_DEPENDS
     ${PROJECT_SOURCE_DIR}/src/view/*.ui)

tmc_add_library(tmc_ui
    SOURCES ${TMC_UI_SOURCES} ${TMC_UI_FORMS}
    LINK    tmc_model Qt6::Widgets
)

# ---------------------------------------------------------------------------
# Executável principal
# ---------------------------------------------------------------------------
file(GLOB TMC_QRC_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/resources/*.qrc)

qt_add_executable(TaskManagerCraft
    src/main.cpp
    ${TMC_QRC_FILES}
)

# tmc_ui antes de tmc_model: ordem correta para o ld (GNU) com libs estáticas.
target_link_libraries(TaskManagerCraft PRIVATE tmc_ui tmc_model Qt6::Widgets)
tmc_configure_target(TaskManagerCraft)

set_target_properties(TaskManagerCraft PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE    ON
)

# ---------------------------------------------------------------------------
# Testes (tests/model, tests/controller, tests/view, tests/integration)
# ---------------------------------------------------------------------------
if(TMC_BUILD_TESTS)
    enable_testing()

    tmc_add_tests(model       tmc_model)
    tmc_add_tests(controller  tmc_ui)
    tmc_add_tests(view        tmc_ui)
    tmc_add_tests(integration tmc_ui)
endif()

# ---------------------------------------------------------------------------
# Benchmarks (benchmarks/*.cpp) - um executável por arquivo
# ---------------------------------------------------------------------------
if(TMC_BUILD_BENCHMARKS)
    file(GLOB TMC_BENCH_SOURCES CONFIGURE_DEPENDS
         ${PROJECT_SOURCE_DIR}/benchmarks/*.cpp)

    foreach(BENCH_SRC IN LISTS TMC_BENCH_SOURCES)
        get_filename_component(BENCH_NAME ${BENCH_SRC} NAME_WE)
        add_executable(tmc_bench_${BENCH_NAME} ${BENCH_SRC})
        target_link_libraries(tmc_bench_${BENCH_NAME} PRIVATE tmc_model)
        tmc_configure_target(tmc_bench_${BENCH_NAME})
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Instalação
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS TaskManagerCraft
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET TaskManagerCraft
    OUTPUT_SCRIPT TMC_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${TMC_DEPLOY_SCRIPT})

# ---------------------------------------------------------------------------
# Resumo da configuração
# ---------------------------------------------------------------------------
message(STATUS "---- Task Manager Craft ${PROJECT_VERSION} ----")
message(STATUS "  Compilador : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ std    : ${CMAKE_CXX_STANDARD} (extensões: ${CMAKE_CXX_EXTENSIONS})")
message(STATUS "  Qt         : ${Qt6_VERSION}")
message(STATUS "  Testes     : ${TMC_BUILD_TESTS}")
if(TMC_BUILD_TESTS)
    message(STATUS "  Doctest    : ${TMC_DOCTEST_ORIGIN}")
endif()
message(STATUS "  Benchmarks : ${TMC_BUILD_BENCHMARKS}")
