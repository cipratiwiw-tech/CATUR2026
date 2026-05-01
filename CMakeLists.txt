cmake_minimum_required(VERSION 3.15)
project(ChessOverlay CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mencari library yang sudah disiapkan oleh vcpkg
find_package(OpenCV REQUIRED)

# Mengambil semua file kode di dalam folder src
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")

# Membuat file .exe
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Menyambungkan OpenCV ke proyek
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})