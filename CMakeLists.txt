ghocmake_minimum_required(VERSION 3.16)
project(TurboBook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer clang over gcc to avoid GCC 13 internal compiler error (ICE) 
# with Boost.Beast template instantiations in WebSocket code
if(NOT CMAKE_CXX_COMPILER)
    find_program(CLANG_CXX clang++)
    if(CLANG_CXX)
        set(CMAKE_CXX_COMPILER ${CLANG_CXX})
        message(STATUS "Using clang++ as C++ compiler")
    endif()
endif()

# Find Boost
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

# Common source files
set(COMMON_SOURCES src/OrderBook.cpp src/OrderBookAnalyzer.cpp)

# Add menu executable
add_executable(turbobook src/main.cpp)

# Add separate executables
add_executable(turbobook-binance src/main_binance.cpp ${COMMON_SOURCES} src/BinanceWebSocketClient.cpp)
add_executable(turbobook-alpaca src/main_alpaca.cpp ${COMMON_SOURCES} src/AlpacaWebSocketClient.cpp)

# Include directories for all executables
target_include_directories(turbobook PRIVATE include)
target_include_directories(turbobook-binance PRIVATE include ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
target_include_directories(turbobook-alpaca PRIVATE include ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

# Link libraries (menu app doesn't need networking libraries)
target_link_libraries(turbobook-binance PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(turbobook-alpaca PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Enable warnings for all executables
if(MSVC)
    target_compile_options(turbobook PRIVATE /W4)
    target_compile_options(turbobook-binance PRIVATE /W4)
    target_compile_options(turbobook-alpaca PRIVATE /W4)
else()
    target_compile_options(turbobook PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(turbobook-binance PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(turbobook-alpaca PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add tests if directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()