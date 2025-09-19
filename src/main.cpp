#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "OrderBook.h"
#include "OrderBookSimulator.h"
#include "OrderBookAnalyzer.h"
#include "BinanceWebSocketClient.h"
#include "AlpacaWebSocketClient.h"

void benchmarkOrderBook() {
    OrderBook book("AAPL");
    const int numOrders = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numOrders; ++i) {
        Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        double price = 150.0 + (i % 20 - 10) * 0.1;
        double qty = 10.0 + (i % 100);
        auto order = std::make_shared<Order>(i, i, side, OrderType::LIMIT, price, qty, "AAPL");
        book.addOrder(order);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Processed " << numOrders << " orders in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average time per order: " << duration.count() / static_cast<double>(numOrders) << " microseconds" << std::endl;
}

int main() {
    std::cout << "TurboBook - High-Performance Order Book Processing Framework" << std::endl;

    // Benchmark
    std::cout << "\nBenchmarking order processing..." << std::endl;
    benchmarkOrderBook();

    std::cout << "\nConnecting to real-time market data..." << std::endl;

    // Create order books
    auto cryptoBook = std::make_shared<OrderBook>("btcusdt");  // Match the symbol case with Binance
    auto stockBook = std::make_shared<OrderBook>("AAPL");

    // Binance for crypto
    std::cout << "Connecting to Binance WebSocket..." << std::endl;
    BinanceWebSocketClient binanceClient;
    binanceClient.setOrderBook(cryptoBook);
    binanceClient.connect("btcusdt");

    // Give Binance a moment to connect
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Alpaca for stocks
    std::cout << "Connecting to Alpaca WebSocket..." << std::endl;
    
    // Check for environment variables or use placeholder values
    std::string alpacaApiKey = std::getenv("ALPACA_API_KEY") ? std::getenv("ALPACA_API_KEY") : "YOUR_ALPACA_API_KEY";
    std::string alpacaApiSecret = std::getenv("ALPACA_API_SECRET") ? std::getenv("ALPACA_API_SECRET") : "YOUR_ALPACA_API_SECRET";
    
    if (alpacaApiKey == "YOUR_ALPACA_API_KEY" || alpacaApiSecret == "YOUR_ALPACA_API_SECRET") {
        std::cout << "Warning: Using placeholder Alpaca API credentials. Set ALPACA_API_KEY and ALPACA_API_SECRET environment variables for real data." << std::endl;
    }
    
    AlpacaWebSocketClient alpacaClient(alpacaApiKey, alpacaApiSecret);
    alpacaClient.setOrderBook(stockBook);
    alpacaClient.connect("AAPL");

    // Give connections time to establish and receive some data
    std::cout << "\nCollecting market data for 30 seconds..." << std::endl;
    
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Show connection status every 5 seconds
        if ((i + 1) % 5 == 0) {
            std::cout << "Time: " << (i + 1) << "s - Binance: " 
                      << (binanceClient.isConnected() ? "Connected" : "Disconnected")
                      << ", Alpaca: " 
                      << (alpacaClient.isConnected() ? "Connected" : "Disconnected") 
                      << std::endl;
        }
    }

    // Analyze crypto book
    std::cout << "\nCrypto Order Book (BTCUSDT):" << std::endl;
    cryptoBook->printBook();
    OrderBookAnalyzer cryptoAnalyzer(*cryptoBook);
    auto cryptoImbalances = cryptoAnalyzer.detectImbalances(3);
    std::cout << "Imbalances:" << std::endl;
    for (const auto& imb : cryptoImbalances) {
        std::cout << "Price: " << imb.price << ", Ratio: " << imb.imbalanceRatio << std::endl;
    }

    // Analyze stock book
    std::cout << "\nStock Order Book (AAPL):" << std::endl;
    stockBook->printBook();
    OrderBookAnalyzer stockAnalyzer(*stockBook);
    auto stockImbalances = stockAnalyzer.detectImbalances(3);
    std::cout << "Imbalances:" << std::endl;
    for (const auto& imb : stockImbalances) {
        std::cout << "Price: " << imb.price << ", Ratio: " << imb.imbalanceRatio << std::endl;
    }

    // Disconnect gracefully
    std::cout << "\nDisconnecting..." << std::endl;
    binanceClient.disconnect();
    alpacaClient.disconnect();

    std::cout << "TurboBook session completed successfully." << std::endl;
    return 0;
}