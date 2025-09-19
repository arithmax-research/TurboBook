#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include "OrderBook.h"
#include "OrderBookAnalyzer.h"
#include "BinanceWebSocketClient.h"

int main() {
    std::cout << "TurboBook - Binance Crypto Market Data Feed" << std::endl;
    std::cout << "============================================" << std::endl;

    std::cout << "\nConnecting to Binance WebSocket for real-time crypto data..." << std::endl;

    // Create crypto order book
    auto cryptoBook = std::make_shared<OrderBook>("btcusdt");

    // Initialize Binance client
    BinanceWebSocketClient binanceClient;
    binanceClient.setOrderBook(cryptoBook);

    // Connect to BTCUSDT stream
    std::cout << "Connecting to Binance WebSocket for BTCUSDT..." << std::endl;
    binanceClient.connect("btcusdt");

    // Give connection time to establish
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (!binanceClient.isConnected()) {
        std::cerr << "Failed to connect to Binance WebSocket. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Successfully connected to Binance!" << std::endl;
    std::cout << "\nCollecting live BTCUSDT market data..." << std::endl;
    std::cout << "Press Ctrl+C to stop or wait 60 seconds for automatic analysis.\n" << std::endl;
    
    // Collect data for 60 seconds with periodic status updates
    for (int i = 0; i < 60; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Show connection status every 10 seconds
        if ((i + 1) % 10 == 0) {
            std::cout << "Time: " << (i + 1) << "s - Binance Status: " 
                      << (binanceClient.isConnected() ? "Connected ✓" : "Disconnected ✗")
                      << std::endl;
        }
    }

    // Analyze the crypto order book
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "BTCUSDT Order Book Analysis" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    cryptoBook->printBook();
    
    OrderBookAnalyzer cryptoAnalyzer(*cryptoBook);
    auto cryptoImbalances = cryptoAnalyzer.detectImbalances(5);
    
    std::cout << "\nOrder Book Imbalances (Top 5):" << std::endl;
    if (cryptoImbalances.empty()) {
        std::cout << "No significant imbalances detected." << std::endl;
    } else {
        for (const auto& imb : cryptoImbalances) {
            std::cout << "Price: $" << imb.price 
                      << ", Imbalance Ratio: " << std::fixed << std::setprecision(2) << imb.imbalanceRatio << std::endl;
        }
    }

    // Show order flow imbalance
    auto orderFlowImbalance = cryptoAnalyzer.getOrderFlowImbalance();
    std::cout << "\nOrder Flow Imbalance: " << std::fixed << std::setprecision(4) << orderFlowImbalance << std::endl;

    // Disconnect gracefully
    std::cout << "\nDisconnecting from Binance..." << std::endl;
    binanceClient.disconnect();

    std::cout << "Binance session completed successfully." << std::endl;
    return 0;
}