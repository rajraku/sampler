# Coding Standards

## C++ — General

### Header files (`.hh`)
- Contain **declarations only**: class definitions, member variable declarations, and function/method declarations.
- No method bodies in headers, except for **template methods**, which must be defined in the header so the compiler can instantiate them at each call site. Add a comment noting this constraint.
- Use `#ifndef` include guards (not `#pragma once`).

### Source files (`.cc`)
- Contain all method definitions that are not templates.
- Include the corresponding `.hh` as the first include.

---

### Declaration format in headers

**Return type on its own line**, method name on the next:

```cpp
// Good
bool
insertEvent(const SensorData& data);

// Bad
bool insertEvent(const SensorData& data);
```

**Every method and member variable has a one-line summary comment** directly above it:

```cpp
// Append one row to sensor_events. Returns true on success.
bool
insertEvent(const SensorData& data);

// Pool of ready-to-use connections.
std::queue<std::unique_ptr<pqxx::connection>> pool;
```

**One blank line between each declaration** (methods and variables alike):

```cpp
class Foo {
public:
    // Construct with connection string.
    Foo(const std::string& url);

    // Returns true on success.
    bool
    connect();

    // Publish message to channel.
    bool
    publish(const std::string& channel, const std::string& message);

private:
    // Underlying connection.
    std::unique_ptr<Connection> conn;

    // Number of reconnect attempts made so far.
    int retry_count;
};
```

Template methods follow the same comment and return-type rules; their body stays in the header:

```cpp
// Run fn(conn), retrying once with a fresh connection on failure.
// Template must stay in header — instantiated at each call site.
template<typename Fn>
bool
execute_with_retry(Fn&& fn) {
    // ...
}
```

---

### Naming

| Element | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `RedisClient`, `SSEManager` |
| Methods / functions | `snake_case` | `send_sse`, `acquire_connection` |
| Private member variables | `snake_case` — no trailing underscore | `pool_size`, `queue_mutex` |
| Local variables | `snake_case` | `conn_id`, `batch` |
| Constants / enums | `UPPER_SNAKE_CASE` | `MAX_RETRIES` |
| File names | `snake_case` | `postgres_client.cc`, `sse_manager.hh` |

> **No trailing underscores** on private members. Plain snake_case applies to all variables regardless of scope.

---

### Class layout
Order sections consistently:
1. `public:` — constructor, destructor, public methods
2. `private:` — private methods, then private member variables

```cpp
class Example {
public:
    // Construct with name.
    Example(std::string name);

    ~Example();

    // Do the primary work.
    void
    do_work();

private:
    // Internal helper.
    void
    helper();

    // Number of work items processed.
    int count;

    // Identifying name.
    std::string name;
};
```

---

### HTTP handlers

Every HTTP handler follows the same structure:

- One `.hh` file with the class declaration and the template `handle()` body.
- One `.cc` file with the constructor and any non-template helper methods.
- The handler is registered in `AppState` in `main.cc` and constructed in `main()`.

**Include `handler_utils.hh`** — never repeat the response initialisation boilerplate inline:

```cpp
#include "handler_utils.hh"

template<class Body, class Allocator>
http::response<http::string_body>
handle(const http::request<Body, http::basic_fields<Allocator>>& req) {

    // Guard the expected HTTP verb — returns 405 automatically if wrong.
    if (auto err = check_method(req, http::verb::post)) return *err;

    // Create a response with version, keep-alive, and server headers pre-set.
    auto res = make_json_response(req);

    // ... handler logic ...

    // Build a complete JSON error response in one call.
    return make_error(req, http::status::bad_request, "Invalid JSON", e.what());
}
```

Available helpers in `handler_utils.hh`:

| Helper | Purpose |
|--------|---------|
| `make_response(req, content_type)` | Base response with common headers |
| `make_json_response(req)` | Shorthand for `application/json` content-type |
| `make_error(req, status, msg[, details])` | Complete JSON error response, ready to return |
| `check_method(req, verb)` | Returns `std::optional<response>` (405) if method mismatches |

---

### Misc
- Use `std::` explicitly; avoid `using namespace std` in headers.
- Prefer `std::make_unique` / `std::make_shared` over raw `new`.
- Atomic members use `.load()` / `.store()` explicitly.
- Each `.cc` file is compiled as an independent translation unit — do not `#include` `.cc` files.
