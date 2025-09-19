#pragma once

#include "OrderBook.h"
#include <vector>

struct ImbalanceInfo {
    double price;
    double bidVolume;
    double askVolume;
    double imbalanceRatio; // bidVolume / askVolume
};

class OrderBookAnalyzer {
private:
    const OrderBook& orderBook_;

public:
    explicit OrderBookAnalyzer(const OrderBook& book) : orderBook_(book) {}

    std::vector<ImbalanceInfo> detectImbalances(int levels = 5) const;
    double calculateVWAP(Side side, int levels = 5) const;
    double getOrderFlowImbalance() const;
};