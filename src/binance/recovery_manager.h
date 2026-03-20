#pragma once

#include "common/recovery_manager.h"
#include "common/gateway.h"
#include "common/decoder.h"
#include "common/sequencer.h"
#include "common/orderbook.h"

namespace binance
{

class RecoveryManager : public IRecoveryManager
{
public:
    RecoveryManager(IGateway& gateway, IDecoder& decoder, ISequencer& sequencer, IOrderbook& orderbook);
    std::expected<void, RecoveringError> begin_initialize() override;
    std::expected<void, RecoveringError> try_recover() override;
    void stop() override;
    bool is_recovering() const override;
    void buffer_update(SequencedBookUpdate update) override;
    void on_decode_error(DecodingError error) const override;
    void on_sequence_error(SequencingError error, SequencedBookUpdate update) const override;

private:
    std::optional<size_t> find_first_applicable_update(uint64_t last_update_id) const;
    bool apply_updates_from(size_t pos);

private:
    IGateway& m_gateway;
    IDecoder& m_decoder;
    ISequencer& m_sequencer;
    IOrderbook& m_orderbook;
    std::vector<SequencedBookUpdate> m_buffer;
    std::optional<Snapshot> m_snapshot;
    bool m_snapshot_requested{false};

    static constexpr std::chrono::seconds kGatewayStartTimeout{2};
};

} // namespace binance
