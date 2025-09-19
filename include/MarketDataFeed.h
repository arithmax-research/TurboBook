#pragma once

#include "OrderBook.h"
#include <string>
#include <memory>

class MarketDataFeed {
public:
    virtual ~MarketDataFeed() = default;
    virtual void connect(const std::string& symbol) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual void setOrderBook(std::shared_ptr<OrderBook> book) = 0;
};