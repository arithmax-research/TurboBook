#pragma once

#include "Order.h"
#include <deque>
#include <memory>
#include <algorithm>

class PriceLevel {
public:
    double price;
    std::deque<std::shared_ptr<Order>> orders;

    PriceLevel(double p) : price(p) {}

    void addOrder(std::shared_ptr<Order> order) {
        orders.push_back(order);
    }

    void removeOrder(uint64_t orderId) {
        orders.erase(std::remove_if(orders.begin(), orders.end(),
            [orderId](const std::shared_ptr<Order>& o) { return o->id == orderId; }),
            orders.end());
    }

    double getTotalQuantity() const {
        double total = 0.0;
        for (const auto& order : orders) {
            total += order->quantity;
        }
        return total;
    }
};