// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <chrono>

import hydrocarbon;
import tests;

static void test_timeout() {
    using namespace std::chrono_literals;

    hydro::RequestOptions opts;
    opts.connect.connectionTimeout = 500ms;

    // this ip should always hang
    const hydro::URL url = "http://10.255.255.1";
    auto ec = hydro::head(url, opts);
    tests::check_err("request with low connection timeout fails", ec, hydro::Error::Timeout);
}

int main() {
    test_timeout();

    tests::exit();
}