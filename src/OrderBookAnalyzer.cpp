#include "OrderBookAnalyzer.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <numeric>
#include <cmath>
#include <numeric>

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

std::vector<DepthProfile> OrderBookAnalyzer::calculateDepthProfile(int levels) const {
    std::vector<DepthProfile> profile;

    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    double cumulativeBidVol = 0.0;
    double cumulativeAskVol = 0.0;

    auto bidIt = bids.begin();
    auto askIt = asks.begin();

    for (int i = 0; i < levels && (bidIt != bids.end() || askIt != asks.end()); ++i) {
        double price = 0.0;
        double bidVol = 0.0;
        double askVol = 0.0;

        if (bidIt != bids.end()) {
            price = bidIt->first;
            bidVol = bidIt->second->getTotalQuantity();
            cumulativeBidVol += bidVol;
            ++bidIt;
        }

        if (askIt != asks.end()) {
            if (price == 0.0) price = askIt->first;
            askVol = askIt->second->getTotalQuantity();
            cumulativeAskVol += askVol;
            ++askIt;
        }

        double totalDepth = cumulativeBidVol + cumulativeAskVol;
        double depthImbalance = totalDepth > 0 ? (cumulativeBidVol - cumulativeAskVol) / totalDepth : 0.0;

        profile.push_back({price, cumulativeBidVol, cumulativeAskVol, depthImbalance});
    }

    return profile;
}

double OrderBookAnalyzer::calculateBookSlope(Side side, int levels) const {
    if (levels < 2) return 0.0;

    std::vector<std::pair<double, double>> priceVolumePairs;

    if (side == Side::BUY) {
        const auto& book = orderBook_.getBids();
        auto it = book.begin();
        for (int i = 0; i < levels && it != book.end(); ++i) {
            priceVolumePairs.emplace_back(it->first, it->second->getTotalQuantity());
            ++it;
        }
    } else {
        const auto& book = orderBook_.getAsks();
        auto it = book.begin();
        for (int i = 0; i < levels && it != book.end(); ++i) {
            priceVolumePairs.emplace_back(it->first, it->second->getTotalQuantity());
            ++it;
        }
    }

    if (priceVolumePairs.size() < 2) return 0.0;

    // Calculate linear regression slope
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
    int n = priceVolumePairs.size();

    for (int i = 0; i < n; ++i) {
        double x = i; // Position index
        double y = priceVolumePairs[i].second; // Volume
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }

    double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    return slope;
}

SpreadAnalysis OrderBookAnalyzer::analyzeSpread() const {
    double bestBid = orderBook_.getBestBid();
    double bestAsk = orderBook_.getBestAsk();

    if (bestBid <= 0 || bestAsk <= 0) {
        return {0.0, 0.0, 0.0, 0.0};
    }

    double absoluteSpread = bestAsk - bestBid;
    double midPrice = (bestBid + bestAsk) / 2.0;
    double relativeSpread = absoluteSpread / midPrice;

    // For effective spread, we'd need trade data - using relative spread as proxy
    double effectiveSpread = relativeSpread;

    // Realized spread would require post-trade analysis - using absolute spread as proxy
    double realizedSpread = absoluteSpread;

    return {absoluteSpread, relativeSpread, effectiveSpread, realizedSpread};
}

QuoteStability OrderBookAnalyzer::measureQuoteStability() const {
    // This would require historical data - for now return basic metrics
    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    double bidVolatility = 0.0;
    double askVolatility = 0.0;

    if (!bids.empty()) {
        double avgBidVolume = 0.0;
        for (const auto& [price, level] : bids) {
            avgBidVolume += level->getTotalQuantity();
        }
        avgBidVolume /= bids.size();

        double variance = 0.0;
        for (const auto& [price, level] : bids) {
            double diff = level->getTotalQuantity() - avgBidVolume;
            variance += diff * diff;
        }
        bidVolatility = sqrt(variance / bids.size());
    }

    if (!asks.empty()) {
        double avgAskVolume = 0.0;
        for (const auto& [price, level] : asks) {
            avgAskVolume += level->getTotalQuantity();
        }
        avgAskVolume /= asks.size();

        double variance = 0.0;
        for (const auto& [price, level] : asks) {
            double diff = level->getTotalQuantity() - avgAskVolume;
            variance += diff * diff;
        }
        askVolatility = sqrt(variance / asks.size());
    }

    // Placeholder values for update rate and quote life
    double quoteUpdateRate = 1.0; // Updates per second
    double quoteLife = 5.0; // Seconds quotes persist

    return {bidVolatility, askVolatility, quoteUpdateRate, quoteLife};
}

OrderFlowMetrics OrderBookAnalyzer::calculateOrderFlowVelocity() const {
    // This would require timestamped order data - for now using volume as proxy
    double bidVolume = 0.0;
    double askVolume = 0.0;

    for (const auto& [price, level] : orderBook_.getBids()) {
        bidVolume += level->getTotalQuantity();
    }

    for (const auto& [price, level] : orderBook_.getAsks()) {
        askVolume += level->getTotalQuantity();
    }

    // Assuming 1 second window for rate calculation
    double buyOrderRate = bidVolume / 1.0;
    double sellOrderRate = askVolume / 1.0;
    double netOrderFlow = buyOrderRate - sellOrderRate;
    double totalFlow = buyOrderRate + sellOrderRate;
    double orderImbalance = totalFlow > 0 ? netOrderFlow / totalFlow : 0.0;

    return {buyOrderRate, sellOrderRate, netOrderFlow, orderImbalance};
}

double OrderBookAnalyzer::calculateCancellationRatio(Side side, int levels) const {
    // This would require tracking order cancellations vs fills
    // For now, return a placeholder based on order book depth
    if (side == Side::BUY) {
        const auto& bids = orderBook_.getBids();
        if (bids.size() < levels) return 0.0;
        // Assume cancellation ratio based on depth distribution
        return 0.1; // 10% cancellation rate placeholder
    } else {
        const auto& asks = orderBook_.getAsks();
        if (asks.size() < levels) return 0.0;
        return 0.1; // 10% cancellation rate placeholder
    }
}

PriceMomentum OrderBookAnalyzer::calculatePriceMomentum() const {
    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    if (bids.empty() || asks.empty()) {
        return {0.0, 0.0, 0.0};
    }

    // Get price levels for momentum calculation
    std::vector<double> bidPrices, askPrices;
    auto bidIt = bids.begin();
    auto askIt = asks.begin();

    for (int i = 0; i < 10 && (bidIt != bids.end() || askIt != asks.end()); ++i) {
        if (bidIt != bids.end()) {
            bidPrices.push_back(bidIt->first);
            ++bidIt;
        }
        if (askIt != asks.end()) {
            askPrices.push_back(askIt->first);
            ++askIt;
        }
    }

    double shortTermMomentum = 0.0;
    double mediumTermMomentum = 0.0;
    double priceAcceleration = 0.0;

    if (bidPrices.size() >= 5) {
        shortTermMomentum = bidPrices[0] - bidPrices[4]; // Price change over 5 levels
    }

    if (bidPrices.size() >= 10) {
        mediumTermMomentum = bidPrices[0] - bidPrices[9]; // Price change over 10 levels
        priceAcceleration = mediumTermMomentum - shortTermMomentum; // Acceleration
    }

    return {shortTermMomentum, mediumTermMomentum, priceAcceleration};
}

VolatilityMetrics OrderBookAnalyzer::calculateVolatility() const {
    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    if (bids.empty() || asks.empty()) {
        return {0.0, 0.0, 0.0};
    }

    // Realized volatility from price differences
    std::vector<double> priceChanges;
    auto bidIt = bids.begin();
    double prevPrice = bidIt->first;
    ++bidIt;

    for (; bidIt != bids.end(); ++bidIt) {
        double currentPrice = bidIt->first;
        priceChanges.push_back(currentPrice - prevPrice);
        prevPrice = currentPrice;
    }

    double realizedVolatility = 0.0;
    if (!priceChanges.empty()) {
        double mean = std::accumulate(priceChanges.begin(), priceChanges.end(), 0.0) / priceChanges.size();
        double variance = 0.0;
        for (double change : priceChanges) {
            variance += (change - mean) * (change - mean);
        }
        realizedVolatility = sqrt(variance / priceChanges.size());
    }

    // Implied volatility from spread
    SpreadAnalysis spread = analyzeSpread();
    double impliedVolatility = spread.relativeSpread * 100.0; // Rough approximation

    // Order book volatility from volume changes
    double orderBookVolatility = 0.0;
    std::vector<double> volumes;
    for (const auto& [price, level] : bids) {
        volumes.push_back(level->getTotalQuantity());
    }
    for (const auto& [price, level] : asks) {
        volumes.push_back(level->getTotalQuantity());
    }

    if (!volumes.empty()) {
        double volMean = std::accumulate(volumes.begin(), volumes.end(), 0.0) / volumes.size();
        double volVariance = 0.0;
        for (double vol : volumes) {
            volVariance += (vol - volMean) * (vol - volMean);
        }
        orderBookVolatility = sqrt(volVariance / volumes.size());
    }

    return {realizedVolatility, impliedVolatility, orderBookVolatility};
}

double OrderBookAnalyzer::estimateMarketImpact(double orderSize, Side side) const {
    // Simplified market impact model
    double totalDepth = 0.0;

    if (side == Side::BUY) {
        const auto& asks = orderBook_.getAsks();
        for (const auto& [price, level] : asks) {
            totalDepth += level->getTotalQuantity();
            if (totalDepth >= orderSize) break;
        }
    } else {
        const auto& bids = orderBook_.getBids();
        for (const auto& [price, level] : bids) {
            totalDepth += level->getTotalQuantity();
            if (totalDepth >= orderSize) break;
        }
    }

    // Impact proportional to order size relative to available depth
    double impactFactor = totalDepth > 0 ? std::min(orderSize / totalDepth, 1.0) : 1.0;
    return impactFactor * 0.001; // Rough 0.1% impact per unit of relative size
}

OptimalQuotes OrderBookAnalyzer::calculateOptimalQuotes(double inventory, double riskTolerance) const {
    double bestBid = orderBook_.getBestBid();
    double bestAsk = orderBook_.getBestAsk();

    if (bestBid <= 0 || bestAsk <= 0) {
        return {0.0, 0.0, 0.0, 0.0};
    }

    double midPrice = (bestBid + bestAsk) / 2.0;
    double baseSpread = orderBook_.getSpread();

    // Adjust spread based on inventory and risk
    double inventoryAdjustment = inventory * 0.0001; // Small adjustment per unit inventory
    double riskAdjustment = (1.0 - riskTolerance) * baseSpread * 0.5;

    double totalSpread = baseSpread + riskAdjustment;
    double halfSpread = totalSpread / 2.0;

    double suggestedBid = midPrice - halfSpread - inventoryAdjustment;
    double suggestedAsk = midPrice + halfSpread + inventoryAdjustment;

    // Confidence score based on order book depth
    double bidDepth = 0.0, askDepth = 0.0;
    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    if (!bids.empty()) bidDepth = bids.begin()->second->getTotalQuantity();
    if (!asks.empty()) askDepth = asks.begin()->second->getTotalQuantity();

    double confidenceScore = std::min(bidDepth, askDepth) / 1000.0; // Normalized confidence
    confidenceScore = std::max(0.0, std::min(1.0, confidenceScore));

    return {suggestedBid, suggestedAsk, confidenceScore, totalSpread};
}

double OrderBookAnalyzer::calculateLiquidityScore() const {
    const auto& bids = orderBook_.getBids();
    const auto& asks = orderBook_.getAsks();

    if (bids.empty() || asks.empty()) return 0.0;

    double totalBidVolume = 0.0;
    double totalAskVolume = 0.0;

    for (const auto& [price, level] : bids) {
        totalBidVolume += level->getTotalQuantity();
    }

    for (const auto& [price, level] : asks) {
        totalAskVolume += level->getTotalQuantity();
    }

    double spread = orderBook_.getSpread();
    double midPrice = (orderBook_.getBestBid() + orderBook_.getBestAsk()) / 2.0;
    double relativeSpread = spread / midPrice;

    // Liquidity score combines volume and spread
    double volumeScore = std::min(totalBidVolume, totalAskVolume) / 10000.0; // Normalize
    double spreadScore = 1.0 - std::min(relativeSpread * 100.0, 1.0); // Invert spread (lower spread = higher score)

    return (volumeScore + spreadScore) / 2.0; // Average of both scores
}