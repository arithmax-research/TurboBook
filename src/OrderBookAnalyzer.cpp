#include "OrderBookAnalyzer.h"
#include <algorithm>
#include <limits>

std::vector<ImbalanceInfo> OrderBookAnalyzer::detectImbalances(int levels) const {
    std::vector<ImbalanceInfo> imbalances;

    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    auto bidIt = bids.begin();
    auto askIt = asks.begin();

    for (int i = 0; i < levels && (bidIt != bids.end() || askIt != asks.end()); ++i) {
        double price = 0.0;
        double bidVol = 0.0;
        double askVol = 0.0;

        if (bidIt != bids.end()) {
            price = bidIt->first;
            bidVol = bidIt->second->getTotalQuantity();
            ++bidIt;
        }

        if (askIt != asks.end()) {
            if (price == 0.0) price = askIt->first;
            askVol = askIt->second->getTotalQuantity();
            ++askIt;
        }

        double ratio = (askVol > 0) ? bidVol / askVol : (bidVol > 0 ? std::numeric_limits<double>::infinity() : 0.0);
        imbalances.push_back({price, bidVol, askVol, ratio});
    }

    return imbalances;
}

double OrderBookAnalyzer::calculateVWAP(Side side, int levels) const {
    double totalVolume = 0.0;
    double totalValue = 0.0;

    if (side == Side::BUY) {
        const auto& book = orderBook_.getBids();
        auto it = book.begin();
        for (int i = 0; i < levels && it != book.end(); ++i) {
            double price = it->first;
            double volume = it->second->getTotalQuantity();
            totalValue += price * volume;
            totalVolume += volume;
            ++it;
        }
    } else {
        const auto& book = orderBook_.getAsks();
        auto it = book.begin();
        for (int i = 0; i < levels && it != book.end(); ++i) {
            double price = it->first;
            double volume = it->second->getTotalQuantity();
            totalValue += price * volume;
            totalVolume += volume;
            ++it;
        }
    }

    return (totalVolume > 0) ? totalValue / totalVolume : 0.0;
}

double OrderBookAnalyzer::getOrderFlowImbalance() const {
    double bidVolume = 0.0;
    double askVolume = 0.0;

    for (const auto& [price, level] : orderBook_.getBids()) {
        bidVolume += level->getTotalQuantity();
    }

    for (const auto& [price, level] : orderBook_.getAsks()) {
        askVolume += level->getTotalQuantity();
    }

    return (askVolume > 0) ? bidVolume / askVolume : (bidVolume > 0 ? std::numeric_limits<double>::infinity() : 0.0);
}