cmake_minimum_required(VERSION 3.16)
project(TurboBook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

# Add executable
add_executable(turbobook src/main.cpp src/OrderBook.cpp src/OrderBookSimulator.cpp src/OrderBookAnalyzer.cpp src/BinanceWebSocketClient.cpp src/AlpacaWebSocketClient.cpp)

# Include directories
target_include_directories(turbobook PRIVATE include ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

# Link libraries
target_link_libraries(turbobook PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Enable warnings
if(MSVC)
    target_compile_options(turbobook PRIVATE /W4)
else()
    target_compile_options(turbobook PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add tests
enable_testing()
add_subdirectory(tests)