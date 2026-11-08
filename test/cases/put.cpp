// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>
#include <format>

import hydrocarbon;
import tests;
import jacinth;

static void test_put() {
    const hydro::URL url = "https://httpbin.org/put";

    hydro::RequestOptions opts;
    opts.body = "PUT Request body, should be returned verbatim.";

    opts.headers.add("my-custom-header-1", "my-custom-value-1");

    auto ec = hydro::put(url, opts);
    tests::check_err("PUT request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    const auto doc = jacinth::doc::read(ec->body);
    tests::check_match("returned method", doc.get<std::string>("method"), "PUT");
    tests::check_match("PUT body", doc.get<std::string>("data"), opts.body);
    tests::check_match("returned URL", doc.get<std::string>("url"), std::format("{}", url));

    tests::check_match("custom header", doc.get("headers").get<std::string>("My-Custom-Header-1"), "my-custom-value-1");
    tests::check_match("content length", doc.get("headers").get<std::string>("Content-Length"), std::to_string(opts.body.size()));
}

int main() {
    test_put();

    tests::exit();
}