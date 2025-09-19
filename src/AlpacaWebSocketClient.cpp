#include "AlpacaWebSocketClient.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <sstream>
#include <string>

using boost::property_tree::ptree;

AlpacaWebSocketClient::AlpacaWebSocketClient(const std::string& apiKey, const std::string& apiSecret)
    : ctx_(ssl::context::sslv23_client), ws_(nullptr), apiKey_(apiKey), apiSecret_(apiSecret) {
    setupSSL();
}

AlpacaWebSocketClient::~AlpacaWebSocketClient() {
    disconnect();
}

bool AlpacaWebSocketClient::setupSSL() {
    try {
        ctx_.set_default_verify_paths();
        ctx_.set_verify_mode(ssl::verify_peer);
        ctx_.set_verify_callback([](bool, ssl::verify_context&) {
            // For production, you might want stricter verification
            return true;
        });
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Alpaca SSL setup failed: " << e.what() << std::endl;
        return false;
    }
}

void AlpacaWebSocketClient::connect(const std::string& symbol) {
    symbol_ = symbol;

    try {
        connectWebSocket(host_, port_, target_);
        authenticate();
        subscribe(symbol);
        connected_ = true;
        stopping_ = false;
        io_thread_ = std::thread(&AlpacaWebSocketClient::run, this);
        std::cout << "Alpaca WebSocket connected successfully for " << symbol << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Alpaca connection failed: " << e.what() << std::endl;
        connected_ = false;
    }
}

void AlpacaWebSocketClient::disconnect() {
    stopping_ = true;
    connected_ = false;
    authenticated_ = false;
    
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

void AlpacaWebSocketClient::connectWebSocket(const std::string& host, const std::string& port, const std::string& target) {
    tcp::resolver resolver{ioc_};
    auto const results = resolver.resolve(host, port);

    ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc_, ctx_);

    // Set SSL SNI hostname
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

void AlpacaWebSocketClient::authenticate() {
    ptree auth_msg;
    auth_msg.put("action", "auth");
    ptree data;
    data.put("key", apiKey_);
    data.put("secret", apiSecret_);
    auth_msg.add_child("data", data);

    std::stringstream ss;
    boost::property_tree::write_json(ss, auth_msg);
    std::string auth_str = ss.str();
    
    ws_->write(net::buffer(auth_str));
    
    // Read authentication response
    beast::flat_buffer buffer;
    ws_->read(buffer);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    
    std::cout << "Auth response: " << response << std::endl;
    
    try {
        std::stringstream resp_ss(response);
        ptree resp_data;
        read_json(resp_ss, resp_data);
        
        // Check if it's an array response
        if (resp_data.count("") && resp_data.get_child("").size() > 0) {
            // Get the first element of the array
            auto first_element = resp_data.get_child("").begin()->second;
            if (first_element.count("T") && first_element.get<std::string>("T") == "success") {
                authenticated_ = true;
                std::cout << "Alpaca authentication successful" << std::endl;
            } else {
                std::cerr << "Alpaca authentication failed" << std::endl;
            }
        } else if (resp_data.count("T") && resp_data.get<std::string>("T") == "success") {
            authenticated_ = true;
            std::cout << "Alpaca authentication successful" << std::endl;
        } else if (resp_data.count("msg") && resp_data.get<std::string>("msg") == "connected") {
            authenticated_ = true;
            std::cout << "Alpaca authentication successful" << std::endl;
        } else {
            std::cerr << "Alpaca authentication failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing auth response: " << e.what() << std::endl;
    }
}

void AlpacaWebSocketClient::subscribe(const std::string& symbol) {
    if (!authenticated_) {
        std::cerr << "Cannot subscribe: not authenticated" << std::endl;
        return;
    }
    
    ptree sub_msg;
    sub_msg.put("action", "subscribe");
    ptree data;
    ptree quotes;
    quotes.push_back(std::make_pair("", ptree(symbol)));
    data.add_child("quotes", quotes);
    sub_msg.add_child("data", data);

    std::stringstream ss;
    boost::property_tree::write_json(ss, sub_msg);
    std::string sub_str = ss.str();
    
    ws_->write(net::buffer(sub_str));
    std::cout << "Subscribed to " << symbol << " quotes" << std::endl;
}

void AlpacaWebSocketClient::run() {
    try {
        readLoop();
    } catch (const std::exception& e) {
        std::cerr << "Alpaca read error: " << e.what() << std::endl;
        connected_ = false;
        if (!stopping_) {
            handleDisconnection();
        }
    }
}

void AlpacaWebSocketClient::readLoop() {
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
                processMessage(data);
                
            } catch (const std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                std::cerr << "Raw message: " << message << std::endl;
            }
            
        } catch (const beast::system_error& se) {
            if (se.code() != websocket::error::closed) {
                std::cerr << "Alpaca WebSocket read error: " << se.what() << std::endl;
            }
            break;
        }
    }
}

void AlpacaWebSocketClient::handleDisconnection() {
    connected_ = false;
    authenticated_ = false;
    std::cout << "Alpaca connection lost, attempting to reconnect..." << std::endl;
    scheduleReconnect();
}

void AlpacaWebSocketClient::scheduleReconnect() {
    if (stopping_) return;
    
    std::this_thread::sleep_for(retry_delay_);
    
    if (!stopping_) {
        try {
            connect(symbol_);
        } catch (const std::exception& e) {
            std::cerr << "Alpaca reconnection failed: " << e.what() << std::endl;
        }
    }
}

void AlpacaWebSocketClient::processMessage(const ptree& data) {
    try {
        // Check message type
        if (data.count("T")) {
            std::string msg_type = data.get<std::string>("T");
            
            if (msg_type == "success") {
                std::cout << "Alpaca: " << data.get<std::string>("msg", "Success") << std::endl;
            } else if (msg_type == "error") {
                std::cerr << "Alpaca error: " << data.get<std::string>("msg", "Unknown error") << std::endl;
            } else if (msg_type == "subscription") {
                std::cout << "Alpaca subscription confirmation received" << std::endl;
            } else if (msg_type == "q") {  // Quote message
                if (orderBook_ && data.count("S")) {
                    std::string symbol = data.get<std::string>("S");
                    if (symbol == symbol_) {
                        // Process bid
                        if (data.count("bp") && data.count("bs")) {
                            double bid_price = data.get<double>("bp");
                            double bid_size = data.get<double>("bs");
                            if (bid_price > 0 && bid_size > 0) {
                                auto bid_order = std::make_shared<Order>(0, 0, Side::BUY, OrderType::LIMIT, bid_price, bid_size, symbol_);
                                orderBook_->addOrder(bid_order);
                            }
                        }
                        
                        // Process ask
                        if (data.count("ap") && data.count("as")) {
                            double ask_price = data.get<double>("ap");
                            double ask_size = data.get<double>("as");
                            if (ask_price > 0 && ask_size > 0) {
                                auto ask_order = std::make_shared<Order>(0, 0, Side::SELL, OrderType::LIMIT, ask_price, ask_size, symbol_);
                                orderBook_->addOrder(ask_order);
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing Alpaca message: " << e.what() << std::endl;
    }
}