// From André Seznec's original implementation.
// https://team.inria.fr/alf/members/andre-seznec/branch-prediction-research/

#include "cpu/pred/ium.hh"

IUM::IUM(const IUMParams* params)
  : BPredUnit(params),
    basePredictor(params->basePredictor),
    logSize(params->logSize),
    retirePtr(0),
    fetchPtr(0)
{
    assert(basePredictor);

    table.resize(1 << logSize);
}

void
IUM::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    basePredictor->uncondBranch(tid, br_pc, bi->baseBranchInfo);
}

bool
IUM::lookup(ThreadID tid, Addr branch_addr, void* &bp_history)
{
    IUMBranchInfo* bi = new IUMBranchInfo();
    bp_history = (void*) bi;

    bool prediction = basePredictor->lookup(tid, branch_addr,
        bi->baseBranchInfo);

    // Prediction
    int tag = basePredictor->position(tid, bi->baseBranchInfo);
    int last = (retirePtr > fetchPtr + 8) ? fetchPtr + 8 : retirePtr;

    for (int i = fetchPtr; i < last; i++) {
        if (table[i & ((1 << logSize) - 1)].tag == tag) {
            iumHit++;
            prediction = table[i & ((1 << logSize) - 1)].prediction;
            bi->iumHit = true;
            bi->prediction = prediction;
            break;
        }
    }

    // Speculative update
    fetchPtr--;
    table[fetchPtr & ((1 << logSize) - 1)].tag = tag;
    table[fetchPtr & ((1 << logSize) - 1)].prediction = prediction;
    return prediction;
}

void
IUM::btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history)
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    basePredictor->btbUpdate(tid, branch_addr, bi->baseBranchInfo);
}

void
IUM::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
            bool squashed)
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    basePredictor->update(tid, branch_addr, taken, bi->baseBranchInfo,
        squashed);

    if (bi->iumHit) {
        if (taken == bi->prediction)
            iumCorrect++;
        else
            iumIncorrect++;
    }

    retirePtr--;
    if (!squashed) {
        delete bi;
    }
}

void
IUM::squash(ThreadID tid, void *bp_history)
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    basePredictor->squash(tid, bi->baseBranchInfo);

    delete bi;
}

unsigned
IUM::getGHR(ThreadID tid, void *bp_history) const
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    return basePredictor->getGHR(tid, bi->baseBranchInfo);
}

void
IUM::regStats()
{
    BPredUnit::regStats();

    iumHit
        .name(name() + ".iumHit")
        .desc("Number of tag hits in the IUM.");

    iumCorrect
        .name(name() + ".iumCorrect")
        .desc("Number of correct IUM predictions.");

    iumIncorrect
        .name(name() + ".iumIncorrect")
        .desc("Number of incorrect IUM predictions.");
}

IUM*
IUMParams::create()
{
    return new IUM(this);
}

bool
IUM::enableStatisticalCorrector(ThreadID tid, const void* bp_history) const
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    return basePredictor->enableStatisticalCorrector(tid, bi->baseBranchInfo);
}

int
IUM::confidence(ThreadID tid, const void* bp_history) const
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    return basePredictor->confidence(tid, bi->baseBranchInfo);
}

int
IUM::statHash(ThreadID tid, int i, const void* bp_history) const
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    return basePredictor->statHash(tid, i, bi->baseBranchInfo);
}

int
IUM::position(ThreadID tid, const void* bp_history) const
{
    IUMBranchInfo* bi = (IUMBranchInfo*) bp_history;
    return basePredictor->position(tid, bi->baseBranchInfo);
}
