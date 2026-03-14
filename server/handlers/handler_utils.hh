#ifndef SERVER_HANDLERS_HANDLER_UTILS_HH
#define SERVER_HANDLERS_HANDLER_UTILS_HH

#include "../../common/json_utils.hh"
#include <boost/beast/http.hpp>
#include <optional>
#include <string>

namespace http = boost::beast::http;

// Create a response with version, keep-alive, and standard server headers pre-set.
template<class Body, class Allocator>
http::response<http::string_body>
make_response(const http::request<Body, http::basic_fields<Allocator>>& req,
              const std::string& content_type) {
    http::response<http::string_body> res;
    res.version(req.version());
    res.keep_alive(req.keep_alive());
    res.set(http::field::server, "IoT-Server");
    res.set(http::field::content_type, content_type);
    return res;
}

// Convenience wrapper that sets content-type to application/json.
template<class Body, class Allocator>
http::response<http::string_body>
make_json_response(const http::request<Body, http::basic_fields<Allocator>>& req) {
    return make_response(req, "application/json");
}

// Return a complete JSON error response with the given HTTP status.
// Omit details by passing an empty string.
template<class Body, class Allocator>
http::response<http::string_body>
make_error(const http::request<Body, http::basic_fields<Allocator>>& req,
           http::status status,
           const std::string& message,
           const std::string& details = "") {
    auto res = make_json_response(req);
    res.result(status);
    json body = {{"error", message}};
    if (!details.empty()) body["details"] = details;
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

// Return 405 if the request method does not match allowed_verb; nullopt if it does.
template<class Body, class Allocator>
std::optional<http::response<http::string_body>>
check_method(const http::request<Body, http::basic_fields<Allocator>>& req,
             http::verb allowed_verb) {
    if (req.method() == allowed_verb) return std::nullopt;
    return make_error(req, http::status::method_not_allowed, "Method not allowed");
}

#endif // SERVER_HANDLERS_HANDLER_UTILS_HH
