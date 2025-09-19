#include "OrderBookSimulator.h"
#include <chrono>
#include <thread>

OrderBookSimulator::OrderBookSimulator(OrderBook& book, const std::string& symbol, double basePrice, double priceSpread, double maxQty)
    : orderBook_(book), symbol_(symbol), rng_(std::random_device{}()), priceDist_(basePrice - priceSpread, basePrice + priceSpread),
      qtyDist_(1.0, maxQty), sideDist_(0, 1), running_(false), nextOrderId_(1) {}

void OrderBookSimulator::start() {
    running_ = true;
    std::thread(&OrderBookSimulator::simulationLoop, this).detach();
}

void OrderBookSimulator::stop() {
    running_ = false;
}

void OrderBookSimulator::generateOrder() {
    Side side = (sideDist_(rng_) == 0) ? Side::BUY : Side::SELL;
    double price = priceDist_(rng_);
    double qty = qtyDist_(rng_);
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto order = std::make_shared<Order>(nextOrderId_++, timestamp, side, OrderType::LIMIT, price, qty, symbol_);
    orderBook_.addOrder(order);
}

void OrderBookSimulator::simulationLoop() {
    while (running_) {
        generateOrder();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Generate order every 100ms
    }
}