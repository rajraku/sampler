#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <set>

#include "config.hh"
#include "db/postgres_client.hh"
#include "pubsub/redis_client.hh"
#include "sse/sse_manager.hh"
#include "handlers/health_handler.hh"
#include "handlers/ingest_handler.hh"

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp = net::ip::tcp;

// Shared application state passed to every session
struct AppState {
    std::shared_ptr<PostgresClient> db;
    std::shared_ptr<RedisClient>    redis;
    std::shared_ptr<SSEManager>     sse;
    std::shared_ptr<IngestHandler>  ingest;
    std::shared_ptr<HealthHandler>  health;
};

class Session : public std::enable_shared_from_this<Session> {
    tcp::socket                      socket_;
    beast::flat_buffer               buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<AppState>        state_;

public:
    Session(tcp::socket socket, std::shared_ptr<AppState> state)
        : socket_(std::move(socket)), state_(std::move(state)) {}

    void run() { do_read(); }

private:
    void do_read() {
        auto self(shared_from_this());
        http::async_read(socket_, buffer_, req_,
            [this, self](beast::error_code ec, std::size_t) {
                if (!ec) route();
            });
    }

    void route() {
        std::string target = std::string(req_.target());
        std::string path   = target.substr(0, target.find('?'));

        // CORS preflight
        if (req_.method() == http::verb::options) {
            http::response<http::string_body> res{http::status::ok, req_.version()};
            res.set(http::field::access_control_allow_origin,  "*");
            res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            res.set(http::field::access_control_allow_headers, "Content-Type");
            res.prepare_payload();
            do_write(std::move(res));
        } else if (path == "/ingest") {
            do_write(state_->ingest->handle(req_));
        } else if (path == "/health") {
            do_write(state_->health->handle(req_));
        } else if (path == "/stream") {
            handle_sse(target);
        } else {
            http::response<http::string_body> res{http::status::not_found, req_.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Not found\n";
            res.prepare_payload();
            do_write(std::move(res));
        }
    }

    // SSE: write headers synchronously, then hand the socket to SSEManager.
    // The session ends here — SSEManager owns the socket from this point on.
    void handle_sse(const std::string& target) {
        auto sock = std::make_shared<tcp::socket>(std::move(socket_));

        const std::string headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n";

        beast::error_code ec;
        net::write(*sock, net::buffer(headers), ec);
        if (ec) return;

        // Parse optional ?sensors=id1,id2 filter
        std::set<std::string> filters;
        auto q = target.find("sensors=");
        if (q != std::string::npos) {
            std::istringstream ss(target.substr(q + 8));
            std::string s;
            while (std::getline(ss, s, ','))
                if (!s.empty()) filters.insert(s);
        }

        size_t conn_id = state_->sse->register_connection(sock, filters);

        // Push current DB state to this new subscriber
        if (!filters.empty()) {
            std::vector<std::string> fv(filters.begin(), filters.end());
            auto initial = state_->db->getLatest(fv);
            state_->sse->send_initial_state(conn_id, initial);
        }
    }

    void do_write(http::response<http::string_body> res) {
        auto self(shared_from_this());
        auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(socket_, *sp,
            [this, self, sp](beast::error_code ec, std::size_t) {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }
};

class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context&          ioc_;
    tcp::acceptor             acceptor_;
    std::shared_ptr<AppState> state_;

public:
    Listener(net::io_context& ioc, tcp::endpoint ep, std::shared_ptr<AppState> state)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), state_(std::move(state)) {
        beast::error_code ec;
        acceptor_.open(ep.protocol(), ec);
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        acceptor_.bind(ep, ec);
        if (ec) { std::cerr << "Bind: " << ec.message() << "\n"; return; }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(net::make_strand(ioc_),
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec)
                    std::make_shared<Session>(std::move(socket), state_)->run();
                do_accept();
            });
    }
};

int main() {
    try {
        auto cfg = ServerConfig::load();
        std::cout << "Starting server on port " << cfg.port << "\n";
        std::cout << "DB:    " << cfg.db_url    << "\n";
        std::cout << "Redis: " << cfg.redis_url << "\n";

        auto state    = std::make_shared<AppState>();
        state->db     = std::make_shared<PostgresClient>(cfg.db_url);
        state->redis  = std::make_shared<RedisClient>(cfg.redis_url);
        state->sse    = std::make_shared<SSEManager>(state->redis.get());
        state->ingest = std::make_shared<IngestHandler>(state->db.get(), state->redis.get());
        state->health = std::make_shared<HealthHandler>(state->db.get(), state->redis.get());

        net::io_context ioc{1};
        auto listener = std::make_shared<Listener>(
            ioc, tcp::endpoint{net::ip::make_address("0.0.0.0"), cfg.port}, state);

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
