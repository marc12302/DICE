cmake_minimum_required(VERSION 3.15)
project(dice)

# Setăm standardul C++
set(CMAKE_CXX_STANDARD 11)

# Adăugăm sursele proiectului
add_executable(dice main.cpp)

# Adăugăm librăriile POSIX (unistd, termios) pentru macOS sau Linux
target_link_libraries(dice PRIVATE pthread)
