// From André Seznec's original implementation.
// https://team.inria.fr/alf/members/andre-seznec/branch-prediction-research/

#include "cpu/pred/loop_predictor.hh"

#include "base/intmath.hh"
#include "base/logging.hh"
#include "base/random.hh"
#include "base/trace.hh"

LoopPredictor::LoopPredictor(const LoopPredictorParams *params)
  : BPredUnit(params),
    logSizeLoopPred(params->logSizeLoopPred),
    loopTableAgeBits(params->loopTableAgeBits),
    loopTableConfidenceBits(params->loopTableConfidenceBits),
    loopTableTagBits(params->loopTableTagBits),
    loopTableIterBits(params->loopTableIterBits),
    logLoopTableAssoc(params->logLoopTableAssoc),
    confidenceThreshold((1 << loopTableConfidenceBits) - 1),
    loopTagMask((1 << loopTableTagBits) - 1),
    loopNumIterMask((1 << loopTableIterBits) - 1),
    loopUseCounter(0),
    withLoopBits(params->withLoopBits),
    basePredictor(params->basePredictor)
{
    assert(basePredictor);

    assert(loopTableTagBits <= 16);
    assert(loopTableIterBits <= 16);

    assert(logSizeLoopPred >= logLoopTableAssoc);

    ltable = new LoopEntry[ULL(1) << logSizeLoopPred];
}

void
LoopPredictor::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    basePredictor->uncondBranch(tid, br_pc, bi->baseBranchInfo);
}

int
LoopPredictor::lindex(Addr pc_in) const
{
    Addr mask = (ULL(1) << (logSizeLoopPred - logLoopTableAssoc)) - 1;
    return (((pc_in >> instShiftAmt) & mask) << logLoopTableAssoc);
}


bool
LoopPredictor::getLoop(Addr pc, LoopPredictorBranchInfo* bi) const
{
    bi->loopHit = -1;
    bi->loopPredValid = false;
    bi->loopIndex = lindex(pc);
    unsigned pcShift = instShiftAmt + logSizeLoopPred - logLoopTableAssoc;
    bi->loopTag = ((pc) >> pcShift) & loopTagMask;

    for (int i = 0; i < (1 << logLoopTableAssoc); i++) {
        if (ltable[bi->loopIndex + i].tag == bi->loopTag) {
            bi->loopHit = i;
            bi->loopPredValid =
                ltable[bi->loopIndex + i].confidence == confidenceThreshold;
            bi->currentIter = ltable[bi->loopIndex + i].currentIterSpec;
            if (ltable[bi->loopIndex + i].currentIterSpec + 1 ==
                ltable[bi->loopIndex + i].numIter) {
                return !(ltable[bi->loopIndex + i].dir);
            }else {
                return (ltable[bi->loopIndex + i].dir);
            }
        }
    }
    return false;
}


void
LoopPredictor::specLoopUpdate(Addr pc, bool taken, LoopPredictorBranchInfo* bi)
{
    if (bi->loopHit>=0) {
        int index = bi->loopHit + bi->loopIndex;
        if (taken != ltable[index].dir) {
            ltable[index].currentIterSpec = 0;
        } else {
            ltable[index].currentIterSpec =
                (ltable[index].currentIterSpec + 1) & loopNumIterMask;
        }
    }
}

void
LoopPredictor::loopUpdate(Addr pc, bool taken, LoopPredictorBranchInfo* bi)
{
    int idx = bi->loopIndex + bi->loopHit;
    if (bi->loopHit >= 0) {
        //already a hit
        if (bi->loopPredValid) {
            if (taken != bi->loopPred) {
                // free the entry
                ltable[idx].numIter = 0;
                ltable[idx].age = 0;
                ltable[idx].confidence = 0;
                ltable[idx].currentIter = 0;
                return;
            } else if (bi->loopPred != bi->basePrediction) {
                unsignedCtrUpdate(ltable[idx].age, true, loopTableAgeBits);
            }
        }

        ltable[idx].currentIter =
            (ltable[idx].currentIter + 1) & loopNumIterMask;
        if (ltable[idx].currentIter > ltable[idx].numIter) {
            ltable[idx].confidence = 0;
            if (ltable[idx].numIter != 0) {
                // free the entry
                ltable[idx].numIter = 0;
                ltable[idx].age = 0;
                ltable[idx].confidence = 0;
            }
        }

        if (taken != ltable[idx].dir) {
            if (ltable[idx].currentIter == ltable[idx].numIter) {
                unsignedCtrUpdate(ltable[idx].confidence, true,
                                  loopTableConfidenceBits);
                //just do not predict when the loop count is 1 or 2
                if (ltable[idx].numIter < 3) {
                    // free the entry
                    ltable[idx].dir = taken;
                    ltable[idx].numIter = 0;
                    ltable[idx].age = 0;
                    ltable[idx].confidence = 0;
                }
            } else {
                if (ltable[idx].numIter == 0) {
                    // first complete nest;
                    ltable[idx].confidence = 0;
                    ltable[idx].numIter = ltable[idx].currentIter;
                } else {
                    //not the same number of iterations as last time: free the
                    //entry
                    ltable[idx].numIter = 0;
                    ltable[idx].age = 0;
                    ltable[idx].confidence = 0;
                }
            }
            ltable[idx].currentIter = 0;
        }

    } else if (taken) {
        //try to allocate an entry on taken branch
        int nrand = random_mt.random<int>();
        for (int i = 0; i < (1 << logLoopTableAssoc); i++) {
            int loop_hit = (nrand + i) & ((1 << logLoopTableAssoc) - 1);
            idx = bi->loopIndex + loop_hit;
            if (ltable[idx].age == 0) {
                ltable[idx].dir = !taken;
                ltable[idx].tag = bi->loopTag;
                ltable[idx].numIter = 0;
                ltable[idx].age = (1 << loopTableAgeBits) - 1;
                ltable[idx].confidence = 0;
                ltable[idx].currentIter = 1;
                break;

            }
            else
                ltable[idx].age--;
        }
    }
}

bool
LoopPredictor::lookup(ThreadID tid, Addr branch_pc, void* &b)
{
    LoopPredictorBranchInfo *bi = new LoopPredictorBranchInfo();
    b = (void*)(bi);

    bool pred_taken = basePredictor->lookup(
        tid, branch_pc, bi->baseBranchInfo);

    bi->loopPred = getLoop(branch_pc, bi);	// loop prediction
    if ((loopUseCounter >= 0) && bi->loopPredValid) {
        pred_taken = bi->loopPred;
        bi->provider = true;
    }

    specLoopUpdate(branch_pc, pred_taken, bi);
    return pred_taken;
}


void
LoopPredictor::update(
    ThreadID tid, Addr branch_pc, bool taken, void* b, bool squashed)
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) b;
    basePredictor->update(tid, branch_pc, taken, bi->baseBranchInfo, squashed);

    loopUpdate(branch_pc, taken, bi);

    if (bi->loopPredValid) {
        if (bi->basePrediction != bi->loopPred) {
            ctrUpdate(loopUseCounter,
                      (bi->loopPred == taken),
                      withLoopBits);
        }
        if (bi->provider) {
            if (bi->loopPred == taken)
                loopPredictorCorrect++;
            else
                loopPredictorWrong++;
        }
    }

    if (!squashed) {
        delete bi;
    }
}

void
LoopPredictor::btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history)
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    basePredictor->btbUpdate(tid, branch_addr, bi->baseBranchInfo);
}

void
LoopPredictor::ctrUpdate(int8_t & ctr, bool taken, int nbits)
{
    assert(nbits <= sizeof(int8_t) << 3);
    if (taken) {
        if (ctr < ((1 << (nbits - 1)) - 1))
            ctr++;
    } else {
        if (ctr > -(1 << (nbits - 1)))
            ctr--;
    }
}

// Up-down unsigned saturating counter
void
LoopPredictor::unsignedCtrUpdate(uint8_t & ctr, bool up, unsigned nbits)
{
    assert(nbits <= sizeof(uint8_t) << 3);
    if (up) {
        if (ctr < ((1 << nbits) - 1))
            ctr++;
    } else {
        if (ctr)
            ctr--;
    }
}

void
LoopPredictor::squash(ThreadID tid, void *bp_history)
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    basePredictor->squash(tid, bi->baseBranchInfo);

    if (bi->loopHit >= 0) {
        int idx = bi->loopIndex + bi->loopHit;
        ltable[idx].currentIterSpec = bi->currentIter;
    }

    delete bi;
}

unsigned
LoopPredictor::getGHR(ThreadID tid, void *bp_history) const
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    return basePredictor->getGHR(tid, bi->baseBranchInfo);
}

bool
LoopPredictor::enableStatisticalCorrector(ThreadID tid,
    const void* bp_history) const
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    return basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo);
}

int
LoopPredictor::confidence(ThreadID tid,
    const void* bp_history) const
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    return basePredictor->confidence(tid, bi->baseBranchInfo);
}

int
LoopPredictor::statHash(ThreadID tid, int i, const void* bp_history) const
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    return basePredictor->statHash(tid, i, bi->baseBranchInfo);
}

int
LoopPredictor::position(ThreadID tid, const void* bp_history) const
{
    LoopPredictorBranchInfo* bi = (LoopPredictorBranchInfo*) bp_history;
    return basePredictor->position(tid, bi->baseBranchInfo);
}

void
LoopPredictor::regStats()
{
    BPredUnit::regStats();

    loopPredictorCorrect
        .name(name() + ".loopPredictorCorrect")
        .desc("Number of times the loop predictor is the provider and "
              "the prediction is correct");

    loopPredictorWrong
        .name(name() + ".loopPredictorWrong")
        .desc("Number of times the loop predictor is the provier and "
              "the prediction is wrong");
}

LoopPredictor*
LoopPredictorParams::create()
{
    return new LoopPredictor(this);
}
