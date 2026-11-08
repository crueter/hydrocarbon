// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>

import hydrocarbon;
import tests;

static void test_head() {
    const hydro::URL url = "https://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::head(url);
    tests::check_err("HEAD request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    tests::check_match("returned status code", ec->status_code, hydro::Status::Ok);
    tests::check_match("body size", ec->body.size(), std::size_t{0});
}

int main() {
    test_head();

    tests::exit();
}