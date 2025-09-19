#pragma once

#include "Order.h"
#include "PriceLevel.h"
#include <map>
#include <memory>
#include <string>

class OrderBook {
private:
    std::string symbol_;
    std::map<double, std::unique_ptr<PriceLevel>, std::greater<double>> bids_; // Higher prices first
    std::map<double, std::unique_ptr<PriceLevel>> asks_; // Lower prices first

public:
    explicit OrderBook(const std::string& symbol) : symbol_(symbol) {}

    void addOrder(std::shared_ptr<Order> order);
    void cancelOrder(uint64_t orderId);
    void matchOrders();

    // Getters for analysis
    const std::map<double, std::unique_ptr<PriceLevel>, std::greater<double>>& getBids() const { return bids_; }
    const std::map<double, std::unique_ptr<PriceLevel>>& getAsks() const { return asks_; }

    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;

    void printBook() const;
};