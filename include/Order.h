#pragma once

#include <cstdint>
#include <string>

enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };

struct Order {
    uint64_t id;
    uint64_t timestamp;
    Side side;
    OrderType type;
    double price;
    double quantity;
    std::string symbol;

    Order(uint64_t id, uint64_t ts, Side s, OrderType t, double p, double q, const std::string& sym)
        : id(id), timestamp(ts), side(s), type(t), price(p), quantity(q), symbol(sym) {}
};