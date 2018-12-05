
#include "cpu/pred/local_statistical_corrector.hh"

LStatisticalCorrector::LStatisticalCorrector(
    const LStatisticalCorrectorParams *params)
  : BPredUnit(params),
    basePredictor(params->basePredictor),
    numHistories(params->numHistories),
    numTables(params->numTables),
    entryBits(params->entryBits),
    logSize(params->logSize),
    localHistoryLengths(params->localHistoryLengths)
{
    assert(basePredictor);

    updateThreshold = 16 << 5;

    localFetchHistory.resize(numHistories);
    localRetireHistory.resize(numHistories);
    confidenceTable.resize(2 * numHistories);

    table = new int8_t*[numTables];
    for (int i = 0; i < numTables; i++) {
        table[i] = new int8_t[1 << logSize];
    }

    for (int i = 0; i < numTables; i++)
      for (int j = 0; j < (1 << logSize); j++)
             table[i][j] = ((j & 1) == 0) ? -4 : 3;

}

void
LStatisticalCorrector::uncondBranch(ThreadID tid, Addr br_pc,
    void* &bp_history)
{
    LSCBranchInfo* bi = (LSCBranchInfo*) bp_history;
    basePredictor->uncondBranch(tid, br_pc, bi->baseBranchInfo);
}

bool
LStatisticalCorrector::lookup(ThreadID tid, Addr branch_addr,
    void* &bp_history)
{
    LSCBranchInfo* bi = new LSCBranchInfo();
    bp_history = (void*) bi;

    bool prediction = basePredictor->lookup(tid, branch_addr,
        bi->baseBranchInfo);
    bi->basePrediction = prediction;

    if (basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo)) {

        int confidence = numTables *
            basePredictor->confidence(tid, bi->baseBranchInfo);

        int localHistory = localFetchHistory[index(branch_addr)];

        int idx = ((branch_addr << 1) + prediction) & ((1 << logSize) - 1);
        confidence += 1 + 2 * table[0][idx];
        for (int i = 0; i < numTables; i++) {
            if (localHistoryLengths[i] < 32) {
                idx = (localHistory & ((1 << localHistoryLengths[i]) - 1))
                    ^ (branch_addr >> i);
            }
            else {
                 idx = localHistory ^ (branch_addr >> i);
            }

            int L = 32;
            int A = 4 + i;
            while (L > logSize - 1) {
                idx ^= (idx >> A);
                L -= A;
            }

            idx = ((idx << 1) + prediction) & ((1 << logSize) - 1);
            confidence += 1 + 2 * table[i][idx];
        }

        if (confidenceTable[(index(branch_addr) << 1) + prediction] >= 0 &&
            prediction != (confidence >= 0)) {
            prediction = !prediction;
            localStatisticalCorrectorCorrected++;
            bi->corrected = true;
        }

    }

    // Speculative update
    localFetchHistory[index(branch_addr)] <<= 1;
    localFetchHistory[index(branch_addr)] += prediction;

    return prediction;
}

void
LStatisticalCorrector::btbUpdate(ThreadID tid, Addr branch_addr,
    void* &bp_history)
{
    LSCBranchInfo* bi = (LSCBranchInfo*) bp_history;
    basePredictor->btbUpdate(tid, branch_addr, bi->baseBranchInfo);
}

unsigned
LStatisticalCorrector::index(Addr branch_addr)
{
    return branch_addr & (numHistories - 1);
}

// Up-down saturating counter
void
LStatisticalCorrector::ctrUpdate(int8_t & ctr, bool taken, int nbits)
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
LStatisticalCorrector::update(ThreadID tid, Addr branch_addr, bool taken,
    void *bp_history, bool squashed)
{
    LSCBranchInfo* bi = (LSCBranchInfo*) bp_history;

    if (basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo)) {

        int confidence = numTables *
            basePredictor->confidence(tid, bi->baseBranchInfo);

        int localHistory = localRetireHistory[index(branch_addr)];

        std::vector<int> indices(numTables);
        int idx = ((branch_addr << 1) + bi->basePrediction) &
            ((1 << logSize) - 1);
        confidence += 1 + 2 * table[0][idx];
        indices[0] = idx;
        for (int i = 0; i < numTables; i++) {
            if (localHistoryLengths[i] < 32) {
                idx = (localHistory & ((1 << localHistoryLengths[i]) - 1))
                    ^ (branch_addr >> i);
            }
            else {
                 idx = localHistory ^ (branch_addr >> i);
            }

            int L = 32;
            int A = 4 + i;
            while (L > logSize - 1) {
                idx ^= (idx >> A);
                L -= A;
            }

            idx = ((idx << 1) + bi->basePrediction) & ((1 << logSize) - 1);
            confidence += 1 + 2 * table[i][idx];
            indices[i] = idx;
        }

        bool thisPrediction = (confidence >= 0);
        if (bi->basePrediction != thisPrediction) {
            if (thisPrediction == taken) {
                if (confidenceTable[(index(branch_addr) << 1) +
                    bi->basePrediction] > -64) {
                    confidenceTable[(index(branch_addr) << 1) +
                        bi->basePrediction]--;
                }
            }
            else {
                if (confidenceTable[(index(branch_addr) << 1) +
                    bi->basePrediction] < 63) {
                    confidenceTable[(index(branch_addr) << 1) +
                        bi->basePrediction]++;
                }
            }
        }

        if ((thisPrediction != taken) ||
            (abs(confidence) < (updateThreshold >> 5))) {

            if (thisPrediction != taken) updateThreshold++;
            else updateThreshold--;

            for (int i = 0; i < numTables; i++) {
                ctrUpdate (table[i][indices[i]], taken, entryBits);
            }
        }

    }

    if (bi->corrected && bi->basePrediction != taken)
        localStatisticalCorrectorCorrectedCorrectly++;

    basePredictor->update(tid, branch_addr, taken, bi->baseBranchInfo,
        squashed);

    localRetireHistory[index(branch_addr)] <<= 1;
    localRetireHistory[index(branch_addr)] += taken;

    if (!squashed) {
        delete bi;
    }
}

void
LStatisticalCorrector::squash(ThreadID tid, void *bp_history)
{
    LSCBranchInfo* bi = (LSCBranchInfo*) bp_history;
    basePredictor->squash(tid, bi->baseBranchInfo);

    delete bi;
}

unsigned
LStatisticalCorrector::getGHR(ThreadID tid, void *bp_history) const
{
    LSCBranchInfo* bi = (LSCBranchInfo*) bp_history;
    return basePredictor->getGHR(tid, bi->baseBranchInfo);
}

void
LStatisticalCorrector::regStats()
{
    BPredUnit::regStats();

    localStatisticalCorrectorCorrected
        .name(name() + ".localStatisticalCorrectorCorrected")
        .desc("Number of LSC corrections.");

    localStatisticalCorrectorCorrectedCorrectly
        .name(name() + ".localStatisticalCorrectorCorrectedCorrectly")
        .desc("Number of correct LSC corrections.");
}

LStatisticalCorrector*
LStatisticalCorrectorParams::create()
{
    return new LStatisticalCorrector(this);
}
