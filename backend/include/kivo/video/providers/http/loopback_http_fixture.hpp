#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>
namespace kivo::video::providers::http {
struct FixtureRoute { std::string method; std::string path_pattern; int status{200};
    std::unordered_map<std::string,std::string> response_headers; std::string response_body;
    std::uint64_t body_size{0}; bool chunked_transfer{false}; bool close_connection{false};
    std::function<std::string(const std::unordered_map<std::string,std::string>&)> dynamic_handler; };
class LoopbackHttpFixture {
public:
    explicit LoopbackHttpFixture(std::uint16_t port=0);
    ~LoopbackHttpFixture();
    bool start(); void stop();
    void add_route(FixtureRoute route);
    std::string base_url() const;
    std::uint16_t port() const;
    void clear_routes();
    void set_write_delay_ms(int ms);
private: struct Impl; Impl* impl;
};
}  // namespace kivo::video::providers::http
