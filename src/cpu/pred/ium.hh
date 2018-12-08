
#ifndef __CPU_PRED_IUM_HH__
#define __CPU_PRED_IUM_HH__

#include "cpu/pred/bpred_unit.hh"
#include "params/IUM.hh"

class IUM : public BPredUnit
{
public:

    IUM(const IUMParams* params);

    // Base class methods.
    void uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history) override;
    bool lookup(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed) override;
    virtual void squash(ThreadID tid, void *bp_history) override;
    virtual unsigned getGHR(ThreadID tid, void *bp_history) const override;

    virtual bool enableStatisticalCorrector(ThreadID tid,
        const void* bp_history) const override;
    virtual int confidence(ThreadID tid,
        const void* bp_history) const override;
    virtual int statHash(ThreadID tid, int i, const void* bp_history)
        const override;

    virtual int position(ThreadID tid, const void* bp_history) const override;

    virtual void regStats() override;

private:

    struct IUMBranchInfo
    {
        // For stats
        bool prediction;
        bool iumHit;

        void* baseBranchInfo;

        IUMBranchInfo() : iumHit(false) {}
    };

    struct SpeculativeEntry
    {
        int tag;
        bool prediction;
    };

    BPredUnit* basePredictor;

    std::vector<SpeculativeEntry> table;

    const int logSize;

    int retirePtr;
    int fetchPtr;

    Stats::Scalar iumHit;
    Stats::Scalar iumCorrect;
    Stats::Scalar iumIncorrect;
};


#endif // __CPU_PRED_IUM_HH__
