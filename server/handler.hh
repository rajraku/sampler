#ifndef HANDLER_HH
#define HANDLER_HH

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>

class Handler {
public:
    virtual ~Handler() = default;
    virtual void handle_request(http::request<http::string_body> const& req, std::string& response) = 0;
};

#endif // HANDLER_HH