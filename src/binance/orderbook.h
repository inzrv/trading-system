#pragma once

#include "common/orderbook.h"
#include "common/book_update.h"
#include "common/level.h"

#include <map>

namespace binance
{

class Orderbook : public IOrderbook
{
public:
    void apply(SequencedBookUpdate update) override;
    void initialize(Snapshot snapshot) override;
    void reset() override;
    L1 l1() const override;

private:
    void set(const std::vector<Level>& bids, const std::vector<Level>& asks);

private:
    std::map<double, double, std::greater<double>> m_bids;
    std::map<double, double, std::less<double>> m_asks;
};

} // namespace binance
