#include "BinanceWebSocketClient.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <sstream>
#include <string>

using boost::property_tree::ptree;

BinanceWebSocketClient::BinanceWebSocketClient() 
    : ctx_(ssl::context::sslv23_client), ws_(nullptr) {
    setupSSL();
}

BinanceWebSocketClient::~BinanceWebSocketClient() {
    disconnect();
}

bool BinanceWebSocketClient::setupSSL() {
    try {
        ctx_.set_default_verify_paths();
        ctx_.set_verify_mode(ssl::verify_peer);
        ctx_.set_verify_callback([](bool, ssl::verify_context&) {
            // For production, you might want stricter verification
            return true;
        });
        return true;
    } catch (const std::exception& e) {
        std::cerr << "SSL setup failed: " << e.what() << std::endl;
        return false;
    }
}

void BinanceWebSocketClient::connect(const std::string& symbol) {
    symbol_ = symbol;
    std::string target = "/ws/" + symbol + "@depth20@1000ms";  // Changed to depth20 for more data

    try {
        connectWebSocket(host_, port_, target);
        connected_ = true;
        stopping_ = false;
        io_thread_ = std::thread(&BinanceWebSocketClient::run, this);
        std::cout << "Binance WebSocket connected successfully for " << symbol << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Binance connection failed: " << e.what() << std::endl;
        connected_ = false;
    }
}

void BinanceWebSocketClient::disconnect() {
    stopping_ = true;
    connected_ = false;
    
    if (ws_ && ws_->next_layer().next_layer().is_open()) {
        try {
            boost::system::error_code ec;
            ws_->close(websocket::close_code::normal, ec);
            // Don't treat close errors as fatal
        } catch (const std::exception& e) {
            // Ignore disconnect exceptions
        }
    }
    
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void BinanceWebSocketClient::connectWebSocket(const std::string& host, const std::string& port, const std::string& target) {
    tcp::resolver resolver{ioc_};
    auto const results = resolver.resolve(host, port);

    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc_, ctx_);

    // Set SSL SNI hostname - this is crucial for Binance
    if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    // Connect to the IP address
    auto ep = net::connect(ws_->next_layer().next_layer(), results.begin(), results.end());
    
    // Update the host string for the HTTP handshake  
    std::string host_string = host + ':' + port;
    
    // Perform SSL handshake
    ws_->next_layer().handshake(ssl::stream_base::client);
    
    // Set WebSocket options
    ws_->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    
    // Set a decorator to change the User-Agent of the handshake
    ws_->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(beast::http::field::user_agent, "TurboBook/1.0");
        }));
    
    // Perform WebSocket handshake
    ws_->handshake(host_string, target);
}

void BinanceWebSocketClient::run() {
    try {
        readLoop();
    } catch (const std::exception& e) {
        std::cerr << "Binance read error: " << e.what() << std::endl;
        connected_ = false;
        if (!stopping_) {
            handleDisconnection();
        }
    }
}

void BinanceWebSocketClient::readLoop() {
    beast::flat_buffer buffer;
    
    while (connected_ && !stopping_) {
        try {
            auto bytes_read = ws_->read(buffer);
            
            std::string message = beast::buffers_to_string(buffer.data());
            buffer.consume(bytes_read);

            try {
                std::stringstream ss(message);
                ptree data;
                read_json(ss, data);
                
                // Debug: Print first few messages to understand structure
                static int debug_count = 0;
                if (debug_count < 3) {
                    std::cout << "Binance message: " << message.substr(0, 200) << "..." << std::endl;
                    debug_count++;
                }
                
                // Check if it's the expected depth update
                if (data.count("bids") && data.count("asks")) {
                    processDepthUpdate(data);
                    
                    // Debug: Print when we process depth updates
                    static int depth_count = 0;
                    if (depth_count < 3) {
                        std::cout << "Processing Binance depth update #" << (depth_count + 1) << std::endl;
                        depth_count++;
                    }
                }
                
            } catch (const std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                std::cerr << "Raw message: " << message.substr(0, 200) << "..." << std::endl;
            }
            
        } catch (const beast::system_error& se) {
            if (se.code() != websocket::error::closed) {
                std::cerr << "WebSocket read error: " << se.what() << std::endl;
            }
            break;
        }
    }
}

void BinanceWebSocketClient::handleDisconnection() {
    connected_ = false;
    std::cout << "Binance connection lost, attempting to reconnect..." << std::endl;
    scheduleReconnect();
}

void BinanceWebSocketClient::scheduleReconnect() {
    if (stopping_) return;
    
    std::this_thread::sleep_for(retry_delay_);
    
    if (!stopping_) {
        try {
            connect(symbol_);
        } catch (const std::exception& e) {
            std::cerr << "Reconnection failed: " << e.what() << std::endl;
        }
    }
}

void BinanceWebSocketClient::processDepthUpdate(const ptree& data) {
    if (!orderBook_) {
        std::cout << "Warning: No order book set!" << std::endl;
        return;
    }

    std::cout << "Processing depth update for " << symbol_ << std::endl;

    // Clear previous depth data for this symbol to maintain accuracy
    // (In a production system, you'd want to handle incremental updates)
    
    try {
        int bids_processed = 0;
        int asks_processed = 0;
        
        // Process bids
        if (data.count("bids")) {
            for (const auto& bid_pair : data.get_child("bids")) {
                const ptree& bid = bid_pair.second;
                auto it = bid.begin();
                if (it == bid.end()) continue;
                
                double price = std::stod(it->second.get_value<std::string>());
                ++it;
                if (it == bid.end()) continue;
                
                double qty = std::stod(it->second.get_value<std::string>());
                
                if (qty > 0) {
                    // Use a simple incrementing order ID and user ID
                    static uint64_t orderId = 1;
                    auto order = std::make_shared<Order>(orderId++, 1, Side::BUY, OrderType::LIMIT, price, qty, symbol_);
                    orderBook_->addOrder(order);
                    bids_processed++;
                }
            }
        }

        // Process asks
        if (data.count("asks")) {
            for (const auto& ask_pair : data.get_child("asks")) {
                const ptree& ask = ask_pair.second;
                auto it = ask.begin();
                if (it == ask.end()) continue;
                
                double price = std::stod(it->second.get_value<std::string>());
                ++it;
                if (it == ask.end()) continue;
                
                double qty = std::stod(it->second.get_value<std::string>());
                
                if (qty > 0) {
                    // Use a simple incrementing order ID and user ID
                    static uint64_t orderId = 10000;
                    auto order = std::make_shared<Order>(orderId++, 2, Side::SELL, OrderType::LIMIT, price, qty, symbol_);
                    orderBook_->addOrder(order);
                    asks_processed++;
                }
            }
        }
        
        std::cout << "Processed " << bids_processed << " bids and " << asks_processed << " asks" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing depth update: " << e.what() << std::endl;
    }
}