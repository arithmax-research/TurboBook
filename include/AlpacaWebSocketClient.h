#pragma once

#include "MarketDataFeed.h"
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <openssl/ssl.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;
using ptree = boost::property_tree::ptree;

class AlpacaWebSocketClient : public MarketDataFeed {
private:
    net::io_context ioc_;
    ssl::context ctx_;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> ws_;
    std::thread io_thread_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> stopping_{false};
    std::atomic<bool> authenticated_{false};
    std::string symbol_;
    std::string apiKey_;
    std::string apiSecret_;
    std::shared_ptr<OrderBook> orderBook_;
    
    // Connection parameters
    std::string host_ = "stream.data.alpaca.markets";
    std::string port_ = "443";
    std::string target_ = "/v2/iex";
    
    // Retry mechanism
    std::chrono::seconds retry_delay_{3};

    void run();
    void connectWebSocket(const std::string& host, const std::string& port, const std::string& target);
    void authenticate();
    void subscribe(const std::string& symbol);
    void readLoop();
    void processMessage(const ptree& data);
    void handleDisconnection();
    void scheduleReconnect();
    bool setupSSL();

public:
    AlpacaWebSocketClient(const std::string& apiKey, const std::string& apiSecret);
    ~AlpacaWebSocketClient();

    void connect(const std::string& symbol) override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }
    void setOrderBook(std::shared_ptr<OrderBook> book) override { orderBook_ = book; }
};