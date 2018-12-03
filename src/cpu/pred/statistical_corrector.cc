
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
    basePredictor->uncondBranch(tid, br_pc, bp_history);
}

bool
StatisticalCorrector::lookup(ThreadID tid, Addr branch_addr, void* &bp_history)
{
    bool prediction = basePredictor->lookup(tid, branch_addr, bp_history);

    if (basePredictor->enableStatisticalCorrector(tid, bp_history)) {

        int confidence = basePredictor->confidence(tid, bp_history);

        for (int i = 0; i < nTables; i++) {
            int idx = basePredictor->statHash(tid, i, bp_history);

            idx = (idx << 1) + prediction;
            idx &= ((1 << logSize) - 1);

            confidence += 1 + 2 * table[idx];
        }

        bool correctedPrediction = confidence >= 0;
        if (abs(confidence) >= useThreshold &&
            correctedPrediction != prediction) {

            prediction = correctedPrediction;
            statisticalCorrectorCorrected++;
        }

    }

    return prediction;
}

void
StatisticalCorrector::btbUpdate(ThreadID tid, Addr branch_addr,
    void* &bp_history)
{
    basePredictor->btbUpdate(tid, branch_addr, bp_history);
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

    if (basePredictor->enableStatisticalCorrector(tid, bp_history)) {

        int confidence = basePredictor->confidence(tid, bp_history);
        bool prediction = taken;

        std::vector<int> indices(nTables);
        for (int i = 0; i < nTables; i++) {
            int idx = basePredictor->statHash(tid, i, bp_history);

            idx = (idx << 1) + prediction;
            idx &= ((1 << logSize) - 1);

            confidence += 1 + 2 * table[idx];
            indices[i] = idx;
        }

        bool correctedPrediction = confidence >= 0;
        if (abs(confidence) >= useThreshold &&
            prediction != correctedPrediction) {

            statisticalCorrectorCorrectedCorrectly++;
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

    basePredictor->update(tid, branch_addr, taken, bp_history, squashed);
}

void
StatisticalCorrector::squash(ThreadID tid, void *bp_history)
{
    basePredictor->squash(tid, bp_history);
}

unsigned
StatisticalCorrector::getGHR(ThreadID tid, void *bp_history) const
{
    return basePredictor->getGHR(tid, bp_history);
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
