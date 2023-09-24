#pragma once

#include <folly/Range.h>

#include <stdexcept>
#include <utility>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <folly/Format.h>
#include <folly/Random.h>
#include <folly/logging/xlog.h>
#include <folly/stats/QuantileEstimator.h>
#pragma GCC diagnostic pop
#include <folly/logging/xlog.h>
#include<iostream>

// #include"Utils.cpp"

namespace util {


class CounterVisitor {
 public:
  enum CounterType {
    COUNT /* couters whose value can be exported directly */,
    RATE /* counters whose value should be exported by delta */
  };

  CounterVisitor() { init(); }

  /* implicit */ CounterVisitor(
      std::function<void(folly::StringPiece, double)> biFn)
      : biFn_(std::move(biFn)) {
    init();
  }

  /* implicit */ CounterVisitor(
      std::function<void(folly::StringPiece, double, CounterType)> triFn)
      : triFn_(std::move(triFn)) {
    init();
  }

  void operator()(folly::StringPiece name,
                  double count,
                  CounterType type) const {
    XDCHECK_NE(nullptr, triFn_);
    triFn_(name, count, type);
  }

  void operator()(folly::StringPiece name, double count) const {
    XDCHECK_NE(nullptr, biFn_);
    biFn_(name, count);
  }

  void operator=(std::function<void(folly::StringPiece, double)> biFn) {
    biFn_ = biFn;
    triFn_ = nullptr;
    init();
  }

  void operator=(
      std::function<void(folly::StringPiece, double, CounterType)> triFn) {
    triFn_ = triFn;
    biFn_ = nullptr;
    init();
  }

 private:
  // Initialize so that at most one of the functions is initialized.
  void init() {
    if (biFn_ && triFn_) {
      throw std::invalid_argument(
          "CounterVisitor can have at most one single function initialized.");
    }
    if (biFn_) {
      triFn_ = [this](folly::StringPiece name, double count, CounterType) {
        biFn_(name, count);
      };
    } else if (triFn_) {
      biFn_ = [this](folly::StringPiece name, double count) {
        triFn_(name, count, CounterType::COUNT);
      };
    } else {
      // Create noop functions.
      triFn_ = [](folly::StringPiece, double, CounterType) {};
      biFn_ = [](folly::StringPiece, double) {};
    }
  }

  // Function to collect all counters by value (COUNT).
  std::function<void(folly::StringPiece name, double count)> biFn_;
  // Function to collect counters by type.
  std::function<void(folly::StringPiece name, double count, CounterType type)>
      triFn_;
};


class PercentileStats {
 public:
  struct Estimates {
    uint64_t avg;
    uint64_t p0;
    uint64_t p5;
    uint64_t p10;
    uint64_t p25;
    uint64_t p50;
    uint64_t p75;
    uint64_t p90;
    uint64_t p95;
    uint64_t p99;
    uint64_t p999;
    uint64_t p9999;
    uint64_t p99999;
    uint64_t p999999;
    uint64_t p100;
  };

  // visit each latency estimate using the visitor.
  // @param visitor   the stat visitor
  // @param rst       the estimates to be visited
  // @param prefix    prefix for the stat name.
  static void visitQuantileEstimates(const CounterVisitor& visitor,
                                     const Estimates& rst,
                                     folly::StringPiece prefix);

  static void visitQuantileEstimates(
      const std::function<void(folly::StringPiece, double)>& visitor,
      const Estimates& rst,
      folly::StringPiece prefix) {
    visitQuantileEstimates(CounterVisitor(visitor), rst, prefix);
  }

  PercentileStats() : estimator_{std::chrono::seconds{kDefaultWindowSize}} {}
  PercentileStats(std::chrono::seconds windowSize) : estimator_{windowSize} {}

  // track latency by taking the value of duration directly.
  void trackValue(double value,
                  std::chrono::time_point<std::chrono::steady_clock> tp =
                      std::chrono::steady_clock::now()) {
    estimator_.addValue(value, tp);
  }

  // Return the estimates for stat. This is not cheap so do not
  // call frequently. The cost is roughly number of quantiles we
  // pass in multiplied by cost of estimating an individual quantile
  Estimates estimate();

  void visitQuantileEstimator(
      const std::function<void(folly::StringPiece, double)>& visitor,
      folly::StringPiece statPrefix) {
    visitQuantileEstimator(CounterVisitor{visitor}, statPrefix);
  }

  // visit each latency estimate using the visitor.
  // @param visitor   the stat visitor
  // @param rst       the estimates to be visited
  // @param prefix    prefix for the stat name.
  void visitQuantileEstimator(const CounterVisitor& visitor,
                              folly::StringPiece statPrefix) {
    auto rst = estimate();
    visitQuantileEstimates(visitor, rst, statPrefix);
  }

 private:
  static const std::array<double, 14> kQuantiles;
  static constexpr int kDefaultWindowSize = 1;

  folly::SlidingWindowQuantileEstimator<> estimator_;
};

class LatencyTracker {
 public:
  explicit LatencyTracker(PercentileStats& stats)
      : stats_(&stats), begin_(std::chrono::steady_clock::now()) {}
  LatencyTracker() {}
  ~LatencyTracker() {
    if (stats_) {
      auto tp = std::chrono::steady_clock::now();
      auto diffNanos =
          std::chrono::duration_cast<std::chrono::nanoseconds>(tp - begin_)
              .count();
      stats_->trackValue(static_cast<double>(diffNanos), tp);
    }
  }

  LatencyTracker(const LatencyTracker&) = delete;
  LatencyTracker& operator=(const LatencyTracker&) = delete;

  LatencyTracker(LatencyTracker&& rhs) noexcept
      : stats_(rhs.stats_), begin_(rhs.begin_) {
    rhs.stats_ = nullptr;
  }

  LatencyTracker& operator=(LatencyTracker&& rhs) noexcept {
    if (this != &rhs) {
      this->~LatencyTracker();
      new (this) LatencyTracker(std::move(rhs));
    }
    return *this;
  }

 private:
  PercentileStats* stats_{nullptr};
  std::chrono::time_point<std::chrono::steady_clock> begin_;
};
} // namespace util
