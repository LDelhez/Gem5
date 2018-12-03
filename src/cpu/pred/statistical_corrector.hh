
#ifndef __CPU_PRED_STATISTICAL_CORRECTOR_HH__
#define __CPU_PRED_STATISTICAL_CORRECTOR_HH__

#include "cpu/pred/bpred_unit.hh"
#include "params/StatisticalCorrector.hh"

class StatisticalCorrector : public BPredUnit
{
public:

    StatisticalCorrector(const StatisticalCorrectorParams* params);

    // Base class methods.
    void uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history) override;
    bool lookup(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed) override;
    virtual void squash(ThreadID tid, void *bp_history) override;
    unsigned getGHR(ThreadID tid, void *bp_history) const override;

    virtual void regStats() override;

private:

    BPredUnit* basePredictor;

    const uint8_t logSize = 12;
    const uint8_t nTables = 5;
    const uint8_t bitsPerEntry = 6;
    const uint8_t minThreshold = 5;
    const uint8_t maxThreshold = 31;

    int8_t useThreshold;
    int8_t countThreshold;

    std::vector<int8_t> table;

    Stats::Scalar statisticalCorrectorCorrected;
    Stats::Scalar statisticalCorrectorCorrectedCorrectly;

    void updateThreshold(int confidence, bool correct);
    void ctrUpdate(int8_t &ctr, bool taken, int nbits);

};

#endif // __CPU_PRED_STATISTICAL_CORRECTOR_HH__
