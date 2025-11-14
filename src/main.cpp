#include <iostream>

int main() {
    std::cout << "TurboBook - High-Performance Order Book Processing Framework" << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Available Applications:" << std::endl;
    std::cout << "----------------------" << std::endl;
    std::cout << "1. turbobook-binance       - Live crypto market data from Binance (BTCUSDT)" << std::endl;
    std::cout << "2. turbobook-alpaca        - Live stock market data from Alpaca (AAPL)" << std::endl;
    std::cout << "3. turbobook-multisymbol   - Multi-symbol market making analysis (BTC, ETH, BNB, ADA, SOL)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  ./turbobook-binance        # For single crypto data" << std::endl;
    std::cout << "  ./turbobook-alpaca         # For single stock data (requires ALPACA_API_KEY and ALPACA_API_SECRET)" << std::endl;
    std::cout << "  ./turbobook-multisymbol    # For multi-symbol market making analysis" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Each application runs independently and connects to real market data feeds." << std::endl;
    std::cout << "No simulated data is used in this version." << std::endl;
    
    return 0;
}