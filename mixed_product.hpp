#ifndef ITER_MIXED_PRODUCT_HPP_
#define ITER_MIXED_PRODUCT_HPP_

#include "internal/iterbase.hpp"

#include <iterator>
#include <tuple>
#include <utility>
#include <array>

/* If the set's powers are co-prime, we can increment all iterators in
 * the tuple given correct Cartesian project. The algorithm adds
 * additional fake elements to the sets and iterates over them.
 *
 * Example:
 * {1, 2}
 * {1, 2, 3, 4}
 * adding fake elements so the powers are co-prime:
 * {1, 2}
 * {1, 2, 3, 4, [5]}
 *
 * mixed product:
 * (1, 1)
 * (2, 2)
 * (1, 3)
 * (2, 4)
 * (1, [5]) <- fake (skipping)
 * (2, 1)
 * (1, 2)
 * (2, 3)
 * (1, 4)
 * */

namespace iter {
  namespace impl {
    template <typename... Containers>
    class MixedProductor;

    template <typename Container, typename... RestContainers>
    class MixedProductor<Container, RestContainers...>;

    template <>
    class MixedProductor<>;
  }

  template <typename... Containers>
  impl::MixedProductor<Containers...> mixed_product(Containers&&...);
}

// specialization for at least 1 template argument
template <typename Container, typename... RestContainers>
class iter::impl::MixedProductor<Container, RestContainers...> {

  using bigint_t = typename MixedProductor<RestContainers...>::bigint_t;

  friend MixedProductor iter::mixed_product<Container, RestContainers...>(
      Container&&, RestContainers&&...);

  template <typename... RC>
  friend class MixedProductor;

  using ProdIterDeref =
      std::tuple<iterator_deref<Container>, iterator_deref<RestContainers>...>;

 private:
  Container container;
  MixedProductor<RestContainers...> rest_products;
  MixedProductor(Container&& in_container, RestContainers&&... rest)
      : container(std::forward<Container>(in_container)),
        rest_products{std::forward<RestContainers>(rest)...} {}

  // calculate fake size for current container and store total fake size
  void set_size(bigint_t size) {
    if (size == 0)
        return;
    this->size_ = size;
    this->fakesize_ = size;
    if (this->total_fakesize() != 0)
    {
      while (gcd(this->fakesize_, this->total_fakesize()) != 1)
        ++(this->fakesize_);
      this->total_fakesize(this->fakesize_ * this->total_fakesize());
    }
    else
    {
      this->total_fakesize(this->fakesize_);
    }
  }

  static bigint_t gcd(bigint_t a, bigint_t b) {
    return (!b ? a : gcd(b, a%b));
  }

  bigint_t size_{0};
  bigint_t fakesize_{0};

 public:
  MixedProductor(MixedProductor&&) = default;

  bigint_t total_fakesize(bigint_t fsz = 0) {
    return rest_products.total_fakesize(fsz);
  }

  class Iterator
      : public std::iterator<std::input_iterator_tag, ProdIterDeref> {
   private:
    using RestIter = typename MixedProductor<RestContainers...>::Iterator;

    iterator_type<Container> iter;
    iterator_type<Container> begin;
    iterator_type<Container> end;
    RestIter rest_iter;
    MixedProductor<Container, RestContainers...>* owner;
    bigint_t counter;

    bool is_fake() const {
      if (counter < owner->size_ || owner->fakesize_ == 0)
        return false;
      return counter < owner->fakesize_ ? true : false;
    }

   public:

    bigint_t global_counter() const {
      return this->rest_iter.global_counter();
    }

    constexpr static const bool is_base_iter = false;
    Iterator(const iterator_type<Container>& it, RestIter&& rest,
      const iterator_type<Container>& end, MixedProductor<Container, RestContainers...>* owner)
        : iter{it}, begin{it}, end{end}, rest_iter{rest}, owner{owner}, counter{0} {}
    friend class MixedProductor<Container, RestContainers...>;

    // returns true if there is any iterator in fake state
    bool do_increment() {
      if (!this->is_fake())
        ++this->iter;
      ++this->counter;
      if (this->iter == this->end)
      {
        if (this->counter > 0 && this->owner->fakesize_ == 0)
          this->owner->set_size(this->counter);
        if (!this->is_fake())
        {
          this->iter = this->begin;
          this->counter = 0;
          return this->rest_iter.do_increment();
        }
        this->rest_iter.do_increment();
        return true;
      }
      return this->rest_iter.do_increment();
    }

    Iterator& operator++() {
      bool is_fake = false;
      do {
        is_fake = this->do_increment();
        if (this->all_sizes_discovered() 
            && this->global_counter() == this->owner->total_fakesize())
        {
          this->operator=(this->owner->end());
          break;
        }
      } while(is_fake);
      return *this;
    }

    Iterator operator++(int) {
      auto ret = *this;
      ++*this;
      return ret;
    }

    bool operator!=(const Iterator& other) const {
      return this->iter != other.iter
             && (RestIter::is_base_iter || this->rest_iter != other.rest_iter);
    }

    bool operator==(const Iterator& other) const {
      return !(*this != other);
    }

    ProdIterDeref operator*() {
      return std::tuple_cat(
          std::tuple<iterator_deref<Container>>{*this->iter}, *this->rest_iter);
    }

    ArrowProxy<ProdIterDeref> operator->() {
      return {**this};
    }

    bool all_sizes_discovered() const {
      return owner->fakesize_ != 0 && rest_iter.all_sizes_discovered();
    }
  };
  friend Iterator;

  Iterator begin() {
    return {std::begin(this->container), std::begin(this->rest_products),
        std::end(this->container), this};
  }

  Iterator end() {
    return {std::end(this->container), std::end(this->rest_products),
        std::end(this->container), this};
  }

};

template <>
class iter::impl::MixedProductor<> {
 public:
  using bigint_t = size_t;
  MixedProductor() = default;
  MixedProductor(MixedProductor&&) = default;

  class Iterator : public std::iterator<std::input_iterator_tag, std::tuple<>> {
   public:
    constexpr static const bool is_base_iter = true;

    bool is_fake() const {
      return false;
    }

    bool all_sizes_discovered() const {
      return true;
    }

    // see note in zip about base case operator!=
    bool operator!=(const Iterator&) const {
      return false;
    }

    bool operator==(const Iterator& other) const {
      return !(*this != other);
    }

    std::tuple<> operator*() const {
      return {};
    }

    bigint_t global_counter() const {
      return counter;
    }

    bool do_increment() {
      ++counter;
      return false;
    }

  private:
    bigint_t counter{0};
  };

  Iterator begin() {
    return {};
  }

  Iterator end() {
    return {};
  }

  bigint_t total_fakesize(const bigint_t fsz = 0) {
    if (fsz != 0)
      total_fakesize_ = fsz;
    return total_fakesize_;
  }

private:
  bigint_t total_fakesize_{0};
};

template <typename... Containers>
iter::impl::MixedProductor<Containers...> iter::mixed_product(Containers&&... containers) {
  return {std::forward<Containers>(containers)...};
}

namespace iter {
  constexpr std::array<std::tuple<>, 1> mixed_product() {
    return {{}};
  }
}

#endif
