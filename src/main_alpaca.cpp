#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include "OrderBook.h"
#include "OrderBookAnalyzer.h"
#include "AlpacaWebSocketClient.h"

int main() {
    std::cout << "TurboBook - Alpaca Stock Market Data Feed" << std::endl;
    std::cout << "=========================================" << std::endl;

    // Check for API credentials
    std::string alpacaApiKey = std::getenv("ALPACA_API_KEY") ? std::getenv("ALPACA_API_KEY") : "";
    std::string alpacaApiSecret = std::getenv("ALPACA_API_SECRET") ? std::getenv("ALPACA_API_SECRET") : "";
    
    if (alpacaApiKey.empty() || alpacaApiSecret.empty()) {
        std::cerr << "Error: Missing Alpaca API credentials!" << std::endl;
        std::cerr << "Please set the following environment variables:" << std::endl;
        std::cerr << "  export ALPACA_API_KEY=your_api_key" << std::endl;
        std::cerr << "  export ALPACA_API_SECRET=your_api_secret" << std::endl;
        return 1;
    }

    std::cout << "\nConnecting to Alpaca WebSocket for real-time stock data..." << std::endl;

    // Create stock order book
    auto stockBook = std::make_shared<OrderBook>("AAPL");

    // Initialize Alpaca client
    AlpacaWebSocketClient alpacaClient(alpacaApiKey, alpacaApiSecret);
    alpacaClient.setOrderBook(stockBook);

    // Connect to AAPL stream
    std::cout << "Connecting to Alpaca WebSocket for AAPL..." << std::endl;
    alpacaClient.connect("AAPL");

    // Give connection time to establish
    std::this_thread::sleep_for(std::chrono::seconds(3));

    if (!alpacaClient.isConnected()) {
        std::cerr << "Failed to connect to Alpaca WebSocket. Please check your credentials." << std::endl;
        return 1;
    }

    std::cout << "Successfully connected to Alpaca!" << std::endl;
    std::cout << "\nCollecting live AAPL market data..." << std::endl;
    std::cout << "Press Ctrl+C to stop or wait 60 seconds for automatic analysis.\n" << std::endl;
    
    // Collect data for 60 seconds with periodic status updates
    for (int i = 0; i < 60; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Show connection status every 10 seconds
        if ((i + 1) % 10 == 0) {
            std::cout << "Time: " << (i + 1) << "s - Alpaca Status: " 
                      << (alpacaClient.isConnected() ? "Connected ✓" : "Disconnected ✗")
                      << std::endl;
        }
    }

    // Analyze the stock order book
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "AAPL Order Book Analysis" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    stockBook->printBook();
    
    OrderBookAnalyzer stockAnalyzer(*stockBook);
    auto stockImbalances = stockAnalyzer.detectImbalances(5);
    
    std::cout << "\nOrder Book Imbalances (Top 5):" << std::endl;
    if (stockImbalances.empty()) {
        std::cout << "No significant imbalances detected." << std::endl;
    } else {
        for (const auto& imb : stockImbalances) {
            std::cout << "Price: $" << std::fixed << std::setprecision(2) << imb.price 
                      << ", Imbalance Ratio: " << std::fixed << std::setprecision(2) << imb.imbalanceRatio << std::endl;
        }
    }

    // Show order flow imbalance
    auto orderFlowImbalance = stockAnalyzer.getOrderFlowImbalance();
    std::cout << "\nOrder Flow Imbalance: " << std::fixed << std::setprecision(4) << orderFlowImbalance << std::endl;

    // Disconnect gracefully
    std::cout << "\nDisconnecting from Alpaca..." << std::endl;
    alpacaClient.disconnect();

    std::cout << "Alpaca session completed successfully." << std::endl;
    return 0;
}