#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <atomic>
#include <mutex>
#include "OrderBook.h"
#include "OrderBookAnalyzer.h"
#include "BinanceWebSocketClient.h"

struct SymbolAnalysis {
    std::string symbol;
    std::shared_ptr<OrderBook> orderBook;
    std::unique_ptr<OrderBookAnalyzer> analyzer;
    std::unique_ptr<BinanceWebSocketClient> client;
    std::atomic<bool> isActive;
    std::mutex dataMutex;

    SymbolAnalysis(const std::string& sym)
        : symbol(sym), orderBook(std::make_shared<OrderBook>(sym)), isActive(false) {
        analyzer = std::make_unique<OrderBookAnalyzer>(*orderBook);
        client = std::make_unique<BinanceWebSocketClient>();
        client->setOrderBook(orderBook);
    }
};

class MultiSymbolAnalyzer {
private:
    std::vector<std::unique_ptr<SymbolAnalysis>> symbols_;
    std::atomic<bool> running_;
    std::thread analysisThread_;

public:
    MultiSymbolAnalyzer(const std::vector<std::string>& symbols) : running_(false) {
        for (const auto& symbol : symbols) {
            symbols_.push_back(std::make_unique<SymbolAnalysis>(symbol));
        }
    }

    ~MultiSymbolAnalyzer() {
        stop();
    }

    void start() {
        running_ = true;

        // Start all WebSocket connections
        for (auto& symbol : symbols_) {
            symbol->isActive = true;
            symbol->client->connect(symbol->symbol);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Stagger connections
        }

        // Give connections time to establish
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Start analysis thread
        analysisThread_ = std::thread(&MultiSymbolAnalyzer::analysisLoop, this);
    }

    void stop() {
        running_ = false;
        if (analysisThread_.joinable()) {
            analysisThread_.join();
        }

        for (auto& symbol : symbols_) {
            symbol->isActive = false;
            symbol->client->disconnect();
        }
    }

private:
    void analysisLoop() {
        while (running_) {
            printAnalysisHeader();
            for (auto& symbol : symbols_) {
                if (symbol->isActive) {
                    printSymbolAnalysis(*symbol);
                }
            }
            std::cout << std::string(200, '=') << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Update every 5 seconds
        }
    }

    void printAnalysisHeader() {
        std::cout << std::string(200, '=') << std::endl;
        std::cout << std::left
                  << std::setw(8) << "Symbol"
                  << std::setw(10) << "BestBid"
                  << std::setw(10) << "BestAsk"
                  << std::setw(8) << "Spread"
                  << std::setw(10) << "BidVWAP"
                  << std::setw(10) << "AskVWAP"
                  << std::setw(12) << "OrderFlow"
                  << std::setw(10) << "Liquidity"
                  << std::setw(10) << "OptBid"
                  << std::setw(10) << "OptAsk"
                  << std::setw(8) << "Conf"
                  << std::setw(10) << "Momentum"
                  << std::setw(10) << "Volatility"
                  << std::setw(8) << "Status"
                  << std::endl;
        std::cout << std::string(200, '-') << std::endl;
    }

    void printSymbolAnalysis(SymbolAnalysis& symbol) {
        std::lock_guard<std::mutex> lock(symbol.dataMutex);

        double bestBid = symbol.orderBook->getBestBid();
        double bestAsk = symbol.orderBook->getBestAsk();
        double spread = symbol.orderBook->getSpread();

        if (bestBid <= 0 || bestAsk <= 0) {
            std::cout << std::left << std::setw(8) << symbol.symbol
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(8) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(12) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(8) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(10) << "N/A"
                      << std::setw(8) << "No Data"
                      << std::endl;
            return;
        }

        // Calculate all metrics
        double bidVWAP = symbol.analyzer->calculateVWAP(Side::BUY, 5);
        double askVWAP = symbol.analyzer->calculateVWAP(Side::SELL, 5);
        double orderFlow = symbol.analyzer->getOrderFlowImbalance();
        double liquidity = symbol.analyzer->calculateLiquidityScore();

        auto optimal = symbol.analyzer->calculateOptimalQuotes(0.0, 0.5);
        auto momentum = symbol.analyzer->calculatePriceMomentum();
        auto volatility = symbol.analyzer->calculateVolatility();

        std::string status = symbol.client->isConnected() ? "Connected" : "Disconnected";

        std::cout << std::left
                  << std::setw(8) << symbol.symbol
                  << std::fixed << std::setprecision(2)
                  << std::setw(10) << bestBid
                  << std::setw(10) << bestAsk
                  << std::setw(8) << spread
                  << std::setw(10) << bidVWAP
                  << std::setw(10) << askVWAP
                  << std::setw(12) << std::setprecision(3) << orderFlow
                  << std::setw(10) << std::setprecision(2) << liquidity
                  << std::setw(10) << std::setprecision(2) << optimal.suggestedBid
                  << std::setw(10) << std::setprecision(2) << optimal.suggestedAsk
                  << std::setw(8) << std::setprecision(2) << optimal.confidenceScore
                  << std::setw(10) << std::setprecision(4) << momentum.shortTermMomentum
                  << std::setw(10) << std::setprecision(4) << volatility.realizedVolatility
                  << std::setw(8) << status
                  << std::endl;
    }
};

int main() {
    std::cout << "TurboBook - Multi-Symbol Market Making Analysis" << std::endl;
    std::cout << "===============================================" << std::endl;

    // Define symbols to analyze
    std::vector<std::string> symbols = {
        "btcusdt", "ethusdt", "bnbusdt", "adausdt", "solusdt"
    };

    std::cout << "\nStarting multi-symbol analysis for:" << std::endl;
    for (const auto& symbol : symbols) {
        std::cout << "  - " << symbol << std::endl;
    }
    std::cout << "\nConnecting to Binance WebSocket streams..." << std::endl;

    MultiSymbolAnalyzer analyzer(symbols);
    analyzer.start();

    std::cout << "\nAnalysis running... Press Ctrl+C to stop." << std::endl;
    std::cout << "Updates every 5 seconds.\n" << std::endl;

    // Keep running until interrupted
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}