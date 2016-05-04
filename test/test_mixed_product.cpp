#include <mixed_product.hpp>

#include "helpers.hpp"

#include <vector>
#include <string>
#include <iterator>

#include "catch.hpp"

using iter::mixed_product;
using Vec = const std::vector<int>;

TEST_CASE("mixed_product: basic test, two sequences", "[mixed_product]") {
  using TP = std::tuple<int, char>;
  using ResType = std::vector<TP>;

  Vec n1 = {0, 1};
  const std::string s{"abc"};

  auto p = mixed_product(n1, s);
  ResType v(std::begin(p), std::end(p));
  ResType vc = {
      TP{0, 'a'}, TP{1, 'b'}, TP{0, 'c'}, TP{1, 'a'}, TP{0, 'b'}, TP{1, 'c'}};

  REQUIRE(v == vc);
}

TEST_CASE("mixed_product: three sequences", "[mixed_product]") {
  using TP = std::tuple<int, char, int>;
  using ResType = const std::vector<TP>;

  Vec n1 = {0, 1};
  const std::string s{"ab"};
  Vec n2 = {2};

  auto p = mixed_product(n1, s, n2);
  ResType v(std::begin(p), std::end(p));

  ResType vc = {TP{0, 'a', 2}, TP{1, 'b', 2}, TP{1, 'a', 2}, TP{0, 'b', 2}};

  REQUIRE(v == vc);
}

TEST_CASE("mixed_product: empty when any iterable is empty", "[mixed_product]") {
  Vec n1 = {0, 1};
  Vec n2 = {0, 1, 2};
  Vec emp = {};

  SECTION("first iterable is empty") {
    auto p = mixed_product(emp, n1, n2);
    REQUIRE(std::begin(p) == std::end(p));
  }

  SECTION("middle iterable is empty") {
    auto p = mixed_product(n1, emp, n2);
    REQUIRE(std::begin(p) == std::end(p));
  }

  SECTION("last iterable is empty") {
    auto p = mixed_product(n1, n2, emp);
    REQUIRE(std::begin(p) == std::end(p));
  }
}

TEST_CASE("mixed_product: single iterable", "[mixed_product]") {
  const std::string s{"ab"};
  using TP = std::tuple<char>;
  using ResType = const std::vector<TP>;

  auto p = mixed_product(s);
  ResType v(std::begin(p), std::end(p));
  ResType vc = {TP{'a'}, TP{'b'}};

  REQUIRE(v == vc);
}

TEST_CASE("mixed_product: no arguments gives one empty tuple", "[mixed_product") {
  auto p = mixed_product();
  auto it = std::begin(p);
  REQUIRE(it != std::end(p));
  REQUIRE(*it == std::make_tuple());
  ++it;
  REQUIRE(it == std::end(p));
}

TEST_CASE("mixed_product: binds to lvalues and moves rvalues", "[mixed_product]") {
  itertest::BasicIterable<char> bi{'x', 'y'};
  itertest::BasicIterable<int> bi2{0, 1};

  SECTION("First ref'd, second moved") {
    mixed_product(bi, std::move(bi2));
    REQUIRE_FALSE(bi.was_moved_from());
    REQUIRE(bi2.was_moved_from());
  }

  SECTION("First moved, second ref'd") {
    mixed_product(std::move(bi), bi2);
    REQUIRE(bi.was_moved_from());
    REQUIRE_FALSE(bi2.was_moved_from());
  }
}

TEST_CASE("mixed_product: doesn't move or copy elements of iterable", "[mixed_product]") {
  constexpr itertest::SolidInt arr[] = {{1}, {0}, {2}};
  for (auto&& t : mixed_product(arr)) {
    (void)std::get<0>(t);
  }
}

TEST_CASE("mixed_product: iterator meets requirements", "mixed_[product]") {
  std::string s{"abc"};
  auto c = mixed_product(s, s);
  REQUIRE(itertest::IsIterator<decltype(std::begin(c))>::value);
}

template <typename... Ts>
using ImpT = decltype(mixed_product(std::declval<Ts>()...));
TEST_CASE("mixed_product: has correct ctor and assign ops", "[mied_product]") {
  using T = ImpT<std::string&, std::vector<double>, std::vector<std::string>>;
  REQUIRE(itertest::IsMoveConstructibleOnly<T>::value);
}

TEST_CASE("mixed_product: iterate through not co-prime", "[mixed_product]") {
  using TP = std::tuple<int, char>;
  using ResType = std::vector<TP>;

  Vec n1 = {0, 1};
  Vec n2 = {0, 1, 2, 3};
  auto p = mixed_product(n1, n2);
  ResType v(std::begin(p), std::end(p));
  ResType vc = {
      TP{0, 0}, TP{1, 1}, TP{0, 2}, TP{1, 3}, TP{1, 0}, TP{0, 1}, TP{1, 2}, TP{0, 3}};

  REQUIRE(v == vc);
}

