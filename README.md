# TurboBook

A high-performance order book processing framework for algorithmic trading.

## Features

- Real-time order book management
- WebSocket integration with Binance (crypto) and Alpaca (stocks)
- Order imbalance analysis
- Volume-weighted average price (VWAP) calculations
- Low-latency C++ implementation

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./turbobook
```

## Configuration

For Alpaca integration, replace the placeholders in `main.cpp` with your API keys:

```cpp
std::string alpacaApiKey = "YOUR_ALPACA_API_KEY";
std::string alpacaApiSecret = "YOUR_ALPACA_API_SECRET";
```

## Dependencies

- Boost (Beast, Asio, Property Tree)
- OpenSSL
- CMake

## Architecture

- `OrderBook`: Core order book with matching engine
- `OrderBookAnalyzer`: Imbalance and VWAP analysis
- `BinanceWebSocketClient`: Real-time crypto data from Binance
- `AlpacaWebSocketClient`: Real-time stock data from Alpaca
- `OrderBookSimulator`: Simulated data for testing