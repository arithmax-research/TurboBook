#include "OrderBook.h"
#include <iostream>
#include <algorithm>

void OrderBook::addOrder(std::shared_ptr<Order> order) {
    if (order->symbol != symbol_) return; // Only handle orders for this symbol

    if (order->side == Side::BUY) {
        auto it = bids_.find(order->price);
        if (it == bids_.end()) {
            bids_[order->price] = std::make_unique<PriceLevel>(order->price);
        }
        bids_[order->price]->addOrder(order);
    } else {
        auto it = asks_.find(order->price);
        if (it == asks_.end()) {
            asks_[order->price] = std::make_unique<PriceLevel>(order->price);
        }
        asks_[order->price]->addOrder(order);
    }

    // Trigger matching after adding
    matchOrders();
}

void OrderBook::cancelOrder(uint64_t orderId) {
    // Find and remove from both sides
    for (auto& [price, level] : bids_) {
        level->removeOrder(orderId);
        if (level->orders.empty()) {
            bids_.erase(price);
            break;
        }
    }
    for (auto& [price, level] : asks_) {
        level->removeOrder(orderId);
        if (level->orders.empty()) {
            asks_.erase(price);
            break;
        }
    }
}

void OrderBook::matchOrders() {
    while (!bids_.empty() && !asks_.empty()) {
        auto bestBid = bids_.begin();
        auto bestAsk = asks_.begin();

        if (bestBid->first >= bestAsk->first) {
            // Match orders
            auto& bidOrders = bestBid->second->orders;
            auto& askOrders = bestAsk->second->orders;

            while (!bidOrders.empty() && !askOrders.empty()) {
                auto& bidOrder = bidOrders.front();
                auto& askOrder = askOrders.front();

                double matchQty = std::min(bidOrder->quantity, askOrder->quantity);
                bidOrder->quantity -= matchQty;
                askOrder->quantity -= matchQty;

                std::cout << "Matched: " << matchQty << " @ " << bestBid->first << std::endl;

                if (bidOrder->quantity == 0) bidOrders.pop_front();
                if (askOrder->quantity == 0) askOrders.pop_front();
            }

            // Remove empty levels
            if (bidOrders.empty()) bids_.erase(bestBid);
            if (askOrders.empty()) asks_.erase(bestAsk);
        } else {
            break; // No more matches
        }
    }
}

double OrderBook::getBestBid() const {
    return bids_.empty() ? 0.0 : bids_.begin()->first;
}

double OrderBook::getBestAsk() const {
    return asks_.empty() ? 0.0 : asks_.begin()->first;
}

double OrderBook::getSpread() const {
    double bid = getBestBid();
    double ask = getBestAsk();
    return (bid > 0 && ask > 0) ? ask - bid : 0.0;
}

void OrderBook::printBook() const {
    std::cout << "Order Book for " << symbol_ << std::endl;
    std::cout << "Bids:" << std::endl;
    for (const auto& [price, level] : bids_) {
        std::cout << "  " << price << ": " << level->getTotalQuantity() << std::endl;
    }
    std::cout << "Asks:" << std::endl;
    for (const auto& [price, level] : asks_) {
        std::cout << "  " << price << ": " << level->getTotalQuantity() << std::endl;
    }
}