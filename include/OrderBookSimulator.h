#pragma once

#include "OrderBook.h"
#include <random>
#include <thread>
#include <atomic>

class OrderBookSimulator {
private:
    OrderBook& orderBook_;
    std::string symbol_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> priceDist_;
    std::uniform_real_distribution<double> qtyDist_;
    std::uniform_int_distribution<int> sideDist_;
    std::atomic<bool> running_;
    uint64_t nextOrderId_;

public:
    OrderBookSimulator(OrderBook& book, const std::string& symbol, double basePrice = 100.0, double priceSpread = 10.0, double maxQty = 1000.0);

    void start();
    void stop();
    void generateOrder();

private:
    void simulationLoop();
};