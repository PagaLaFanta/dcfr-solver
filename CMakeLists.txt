cmake_minimum_required(VERSION 3.16)
project(dcfr_river_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compilar para cualquier procesador de esta arquitectura, no para el de la
# maquina que compila.
#
# -march=native mira el procesador de quien compila y usa todo lo que tenga. Eso
# esta muy bien para uso propio y es una trampa para publicar: un binario hecho
# en una maquina de CI con AVX-512 muere con "instruccion ilegal" en el equipo de
# quien se lo descargue. Se enciende en el workflow de release y en ningun otro
# sitio.
option(PORTABLE "Binario para distribuir: sin -march=native" OFF)

add_executable(solver src/main.cpp)
target_include_directories(solver PRIVATE src)

find_package(Threads REQUIRED)
target_link_libraries(solver PRIVATE Threads::Threads)

if(WIN32)
  target_link_libraries(solver PRIVATE ws2_32)   # the local web UI's socket
endif()

if(MINGW)
  # Windows has no single system libstdc++: Git for Windows and friends put
  # their own (older, ABI-incompatible) libstdc++-6.dll on PATH, and a
  # dynamically linked binary will happily load it and crash. Link it in.
  target_link_options(solver PRIVATE -static)
endif()

if(MSVC)
  # /utf-8 no es un detalle: los fuentes estan en UTF-8 y los mensajes llevan
  # acentos y enes dentro. Sin esto MSVC los lee en la pagina de codigos de la
  # maquina, avisa con un C4819 que con /W4 se pierde entre los demas, y saca un
  # binario que escribe "Ãrbol" por pantalla. Compilando con g++ no pasa, asi
  # que es justo el fallo que solo se ve en la build que nadie mira.
  target_compile_options(solver PRIVATE /O2 /W4 /permissive- /EHsc /utf-8)
  # CRT estatico: si no, el binario pide vcruntime140.dll y quien se lo descargue
  # no tiene por que tenerla.
  set_property(TARGET solver PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_compile_options(solver PRIVATE -O3 -Wall -Wextra)
  # -march=native is not available on every toolchain / arch, so probe it.
  if(NOT PORTABLE)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" HAS_MARCH_NATIVE)
    if(HAS_MARCH_NATIVE)
      target_compile_options(solver PRIVATE -march=native)
    endif()
  endif()
endif()

# Tests:  cmake --build build --target test_eval test_solve
foreach(t test_eval test_solve)
  add_executable(${t} EXCLUDE_FROM_ALL src/${t}.cpp)
  target_include_directories(${t} PRIVATE src)
  if(MINGW)
    target_link_options(${t} PRIVATE -static)
  endif()
  if(MSVC)
    target_compile_options(${t} PRIVATE /O2 /utf-8)
  else()
    target_compile_options(${t} PRIVATE -O2)
  endif()
  find_package(Threads REQUIRED)
  target_link_libraries(${t} PRIVATE Threads::Threads)
endforeach()
