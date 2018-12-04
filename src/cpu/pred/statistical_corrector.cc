
#include "cpu/pred/statistical_corrector.hh"

StatisticalCorrector::StatisticalCorrector(
    const StatisticalCorrectorParams *params)
  : BPredUnit(params),
    basePredictor(params->basePredictor),
    useThreshold(minThreshold),
    countThreshold(0)
{
    assert(basePredictor);

    table.resize((1 << logSize));
}

void
StatisticalCorrector::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
    SCBranchInfo* bi = (SCBranchInfo*) bp_history;
    basePredictor->uncondBranch(tid, br_pc, bi->baseBranchInfo);
}

bool
StatisticalCorrector::lookup(ThreadID tid, Addr branch_addr, void* &bp_history)
{
    SCBranchInfo* bi = new SCBranchInfo();
    bp_history = (void*) bi;

    bool prediction = basePredictor->lookup(tid, branch_addr,
        bi->baseBranchInfo);

    if (basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo)) {

        int confidence = basePredictor->confidence(tid, bi->baseBranchInfo);

        for (int i = 0; i < nTables; i++) {
            int idx = basePredictor->statHash(tid, i, bi->baseBranchInfo);

            idx = (idx << 1) + prediction;
            idx &= ((1 << logSize) - 1);

            confidence += 1 + 2 * table[idx];
        }

        bool correctedPrediction = confidence >= 0;
        if (abs(confidence) >= useThreshold &&
            correctedPrediction != prediction) {

            prediction = correctedPrediction;
            statisticalCorrectorCorrected++;
            bi->corrected = true;
        }
    }

    bi->prediction = prediction;

    return prediction;
}

void
StatisticalCorrector::btbUpdate(ThreadID tid, Addr branch_addr,
    void* &bp_history)
{
    SCBranchInfo* bi = (SCBranchInfo*) bp_history;
    basePredictor->btbUpdate(tid, branch_addr, bi->baseBranchInfo);
}

// Up-down saturating counter
void
StatisticalCorrector::ctrUpdate(int8_t & ctr, bool taken, int nbits)
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

void
StatisticalCorrector::updateThreshold(int confidence, bool correct)
{
    if (abs(confidence) < useThreshold - 4 ||
        abs(confidence) > useThreshold - 2)
        return;

   ctrUpdate(countThreshold, correct, 6);

   if (countThreshold == (1 << 5) - 1) {
       if (useThreshold > minThreshold) {
           countThreshold = 0;
           useThreshold -= 2;
       }
   }

   if (countThreshold == -(1 << 5)) {
       if (useThreshold < maxThreshold) {
           countThreshold = 0;
           useThreshold += 2;
       }
   }
}

void
StatisticalCorrector::update(ThreadID tid, Addr branch_addr, bool taken,
    void *bp_history, bool squashed)
{
    SCBranchInfo* bi = (SCBranchInfo*) bp_history;

    if (basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo)) {

        int confidence = basePredictor->confidence(tid, bi->baseBranchInfo);
        bool prediction = taken;

        std::vector<int> indices(nTables);
        for (int i = 0; i < nTables; i++) {
            int idx = basePredictor->statHash(tid, i, bi->baseBranchInfo);

            idx = (idx << 1) + prediction;
            idx &= ((1 << logSize) - 1);

            confidence += 1 + 2 * table[idx];
            indices[i] = idx;
        }

        bool correctedPrediction = confidence >= 0;
        if (abs(confidence) >= useThreshold &&
            prediction != correctedPrediction) {
            prediction = correctedPrediction;
        }

        if (taken != correctedPrediction)
            updateThreshold(confidence, taken == prediction);

        if (correctedPrediction != prediction ||
            abs(confidence) < (21 + 8*useThreshold)) {

            for (int i = 0; i < nTables; i++)
                ctrUpdate(table[indices[i]], taken, bitsPerEntry);
        }
    }

    if (bi->corrected && taken == bi->prediction) {
        statisticalCorrectorCorrectedCorrectly++;
    }

    basePredictor->update(tid, branch_addr, taken, bi->baseBranchInfo,
        squashed);

    if (!squashed) {
        delete bi;
    }
}

void
StatisticalCorrector::squash(ThreadID tid, void *bp_history)
{
    SCBranchInfo* bi = (SCBranchInfo*) bp_history;
    basePredictor->squash(tid, bi->baseBranchInfo);

    delete bi;
}

unsigned
StatisticalCorrector::getGHR(ThreadID tid, void *bp_history) const
{
    SCBranchInfo* bi = (SCBranchInfo*) bp_history;
    return basePredictor->getGHR(tid, bi->baseBranchInfo);
}

void
StatisticalCorrector::regStats()
{
    BPredUnit::regStats();

    statisticalCorrectorCorrected
        .name(name() + ".statisticalCorrectorCorrected")
        .desc("Number of SC corrections.");

    statisticalCorrectorCorrectedCorrectly
        .name(name() + ".statisticalCorrectorCorrectedCorrectly")
        .desc("Number of correct SC corrections.");
}

StatisticalCorrector*
StatisticalCorrectorParams::create()
{
    return new StatisticalCorrector(this);
}
