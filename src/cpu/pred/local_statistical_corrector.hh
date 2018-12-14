// From André Seznec's original implementation.
// https://team.inria.fr/alf/members/andre-seznec/branch-prediction-research/

#ifndef __CPU_PRED_LOCAL_STATISTICAL_CORRECTOR_HH__
#define __CPU_PRED_LOCAL_STATISTICAL_CORRECTOR_HH__

#include "cpu/pred/bpred_unit.hh"
#include "params/LStatisticalCorrector.hh"

class LStatisticalCorrector : public BPredUnit
{
public:

    LStatisticalCorrector(const LStatisticalCorrectorParams* params);

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

    struct LSCBranchInfo
    {
        void* baseBranchInfo;
        bool basePrediction;
        bool corrected;

        LSCBranchInfo() : corrected(false) {}
    };

    struct FoldedHistory
    {
        unsigned comp;
        int compLength;
        int origLength;
        int outpoint;

        void init(int original_length, int compressed_length)
        {
            comp = 0;
            origLength = original_length;
            compLength = compressed_length;
            outpoint = original_length % compressed_length;
        }

        void update(uint8_t * h)
        {
            comp = (comp << 1) | h[0];
            comp ^= h[origLength] << outpoint;
            comp ^= (comp >> compLength);
            comp &= (ULL(1) << compLength) - 1;
        }
    };

    BPredUnit* basePredictor;

    const int numHistories;
    const int numTables;
    const int entryBits;
    const int logSize;
    std::vector<unsigned> localHistoryLengths;

    std::vector<long> localFetchHistory;
    std::vector<long> localRetireHistory;
    std::vector<int8_t> confidenceTable;

    int8_t **table;
    int updateThreshold;

    Stats::Scalar localStatisticalCorrectorCorrected;
    Stats::Scalar localStatisticalCorrectorCorrectedCorrectly;

    void ctrUpdate(int8_t & ctr, bool taken, int nbits);
    unsigned index(Addr branch_addr);
};

#endif // __CPU_PRED_LOCAL_STATISTICAL_CORRECTOR_HH__
