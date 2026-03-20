#include "orderbook.h"

namespace binance
{

void Orderbook::apply(SequencedBookUpdate update)
{
    set(update.bids, update.asks);
}

void Orderbook::initialize(Snapshot snapshot)
{
    set(snapshot.bids, snapshot.asks);
}

void Orderbook::set(const std::vector<Level>& bids, const std::vector<Level>& asks)
{
    for (const auto& bid : bids) {
        if (bid.quantity == 0) {
            m_bids.erase(bid.price);
        } else {
            m_bids[bid.price] = bid.quantity;
        }
    }

    for (const auto& ask : asks) {
        if (ask.quantity == 0) {
            m_asks.erase(ask.price);
        } else {
            m_asks[ask.price] = ask.quantity;
        }
    }
}

void Orderbook::reset()
{
    m_bids.clear();
    m_asks.clear();
}

L1 Orderbook::l1() const
{
    L1 l1;

    if (!m_bids.empty()) {
        const auto& [price, qty] = *m_bids.begin();
        l1.best_bid = Level{.price = price, .quantity = qty};
    }

    if (!m_asks.empty()) {
        const auto& [price, qty] = *m_asks.begin();
        l1.best_ask = Level{.price = price, .quantity = qty};
    }

    return l1;
}

} // namespace binance
