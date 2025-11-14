#pragma once

#include "OrderBook.h"
#include <vector>

struct ImbalanceInfo {
    double price;
    double bidVolume;
    double askVolume;
    double imbalanceRatio; // bidVolume / askVolume
};

struct DepthProfile {
    double price;
    double cumulativeBidVolume;
    double cumulativeAskVolume;
    double depthImbalance;
};

struct SpreadAnalysis {
    double absoluteSpread;
    double relativeSpread;  // spread / midPrice
    double effectiveSpread; // based on actual fills
    double realizedSpread;  // post-trade analysis
};

struct QuoteStability {
    double bidVolatility;     // How much bid prices change
    double askVolatility;     // How much ask prices change
    double quoteUpdateRate;   // Frequency of quote changes
    double quoteLife;         // Average time quotes persist
};

struct OrderFlowMetrics {
    double buyOrderRate;      // Orders per second on bid side
    double sellOrderRate;     // Orders per second on ask side
    double netOrderFlow;      // buyRate - sellRate
    double orderImbalance;    // normalized net flow
};

struct PriceMomentum {
    double shortTermMomentum;  // 5-level price change
    double mediumTermMomentum; // 10-level price change
    double priceAcceleration;  // Rate of momentum change
};

struct VolatilityMetrics {
    double realizedVolatility;    // Based on recent price changes
    double impliedVolatility;     // From bid-ask spread
    double orderBookVolatility;   // From order book depth changes
};

struct OptimalQuotes {
    double suggestedBid;
    double suggestedAsk;
    double confidenceScore;
    double expectedSpread;
};

class OrderBookAnalyzer {
private:
    const OrderBook& orderBook_;

public:
    explicit OrderBookAnalyzer(const OrderBook& book) : orderBook_(book) {}

    // Existing methods
    std::vector<ImbalanceInfo> detectImbalances(int levels = 5) const;
    double calculateVWAP(Side side, int levels = 5) const;
    double getOrderFlowImbalance() const;

    // New depth & liquidity analysis
    std::vector<DepthProfile> calculateDepthProfile(int levels = 10) const;
    double calculateBookSlope(Side side, int levels = 5) const;

    // Market microstructure signals
    SpreadAnalysis analyzeSpread() const;
    QuoteStability measureQuoteStability() const;

    // Order flow & momentum analysis
    OrderFlowMetrics calculateOrderFlowVelocity() const;
    double calculateCancellationRatio(Side side, int levels = 5) const;

    // Statistical price analysis
    PriceMomentum calculatePriceMomentum() const;
    VolatilityMetrics calculateVolatility() const;

    // Advanced market making signals
    double estimateMarketImpact(double orderSize, Side side) const;
    OptimalQuotes calculateOptimalQuotes(double inventory = 0.0, double riskTolerance = 0.5) const;
    double calculateLiquidityScore() const;
};