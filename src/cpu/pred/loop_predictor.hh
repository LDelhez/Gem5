// From André Seznec's original implementation.
// https://team.inria.fr/alf/members/andre-seznec/branch-prediction-research/

#ifndef __CPU_PRED_LOOP_PREDICTOR
#define __CPU_PRED_LOOP_PREDICTOR


#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/LoopPredictor.hh"

class LoopPredictor: public BPredUnit
{
  public:
    LoopPredictor(const LoopPredictorParams *params);

    // Base class methods.
    void uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history) override;
    bool lookup(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed) override;
    virtual void squash(ThreadID tid, void *bp_history) override;
    unsigned getGHR(ThreadID tid, void *bp_history) const override;

    virtual bool enableStatisticalCorrector(ThreadID tid,
        const void* bp_history) const override;
    virtual int confidence(ThreadID tid,
        const void* bp_history) const override;
    virtual int statHash(ThreadID tid, int i, const void* bp_history)
        const override;

    virtual int position(ThreadID tid, const void* bp_history) const override;

    virtual void regStats() override;

  private:
    struct LoopEntry
    {
        uint16_t numIter;
        uint16_t currentIter;
        uint16_t currentIterSpec;
        uint8_t confidence;
        uint16_t tag;
        uint8_t age;
        bool dir;

        LoopEntry() : numIter(0), currentIter(0), currentIterSpec(0),
                      confidence(0), tag(0), age(0), dir(0) { }
    };

    struct LoopPredictorBranchInfo
    {
        uint16_t loopTag;
        uint16_t currentIter;

        bool loopPred;
        bool loopPredValid;
        int  loopIndex;
        int loopHit;

        bool basePrediction;
        bool provider;
        void* baseBranchInfo;

        LoopPredictorBranchInfo()
            : loopTag(0), currentIter(0),
              loopPred(false),
              loopPredValid(false), loopIndex(0), loopHit(-1), provider(false)
        {}
    };

    int lindex(Addr pc_in) const;
    bool getLoop(Addr pc, LoopPredictorBranchInfo* bi) const;
    void loopUpdate(Addr pc, bool Taken, LoopPredictorBranchInfo* bi);
    void specLoopUpdate(Addr pc, bool taken, LoopPredictorBranchInfo* bi);
    void ctrUpdate(int8_t & ctr, bool taken, int nbits);
    void unsignedCtrUpdate(uint8_t & ctr, bool up, unsigned nbits);


    const unsigned logSizeLoopPred;
    const unsigned loopTableAgeBits;
    const unsigned loopTableConfidenceBits;
    const unsigned loopTableTagBits;
    const unsigned loopTableIterBits;
    const unsigned logLoopTableAssoc;
    const uint8_t confidenceThreshold;
    const uint16_t loopTagMask;
    const uint16_t loopNumIterMask;

    LoopEntry *ltable;

    int8_t loopUseCounter;
    unsigned withLoopBits;

    BPredUnit* basePredictor;

    // stats
    Stats::Scalar loopPredictorCorrect;
    Stats::Scalar loopPredictorWrong;

};

#endif // __CPU_PRED_LOOP_PREDICTOR
