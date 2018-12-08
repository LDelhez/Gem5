from m5.objects import *

def tage5_(bp, n):
    tagTableTagWidths = [0, 8, 8, 9, 9]
    logTagTableSizes = [n-3, n-6, n-6, n-6, n-6]
    tagTableCounterBits = 3
    tagTableUBits = 2

    bp.nHistoryTables = 4
    bp.minHist = 5
    bp.maxHist = 130
    bp.tagTableTagWidths = tagTableTagWidths
    bp.logTagTableSizes = logTagTableSizes
    bp.tagTableCounterBits = tagTableCounterBits
    bp.tagTableUBits = tagTableUBits

    storage = sum([2**s * (t + tagTableCounterBits + tagTableUBits)
        for (t,s) in zip(tagTableTagWidths[1:],logTagTableSizes[1:])])
    storage += 1024 * 10 * 2**(n-16)

    return (bp, int(storage))

def tage8_(bp, n):
    tagTableTagWidths = [0, 9, 9, 10, 10, 11, 11, 12]
    logTagTableSizes = [n-3, n-7, n-7, n-7, n-7, n-7, n-7, n-7]
    tagTableCounterBits = 3
    tagTableUBits = 2

    bp.nHistoryTables = 7
    bp.minHist = 5
    bp.maxHist = 130
    bp.tagTableTagWidths = tagTableTagWidths
    bp.logTagTableSizes = logTagTableSizes
    bp.tagTableCounterBits = tagTableCounterBits
    bp.tagTableUBits = tagTableUBits

    storage = sum([2**s * (t + tagTableCounterBits + tagTableUBits)
        for (t,s) in zip(tagTableTagWidths[1:],logTagTableSizes[1:])])
    storage += 1024 * 10 * 2**(logTagTableSizes[0]-13)

    return (bp, int(storage))

def tage13_(bp, n):
    tagTableTagWidths = [0, 7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15]
    logTagTableSizes =
        [n-4, n-8, n-8, n-7, n-7, n-7, n-7, n-8, n-8, n-8, n-8, n-9, n-9]
    tagTableCounterBits = 3
    tagTableUBits = 2

    bp.nHistoryTables = 12
    bp.minHist = 4
    bp.maxHist = 640
    bp.tagTableTagWidths = tagTableTagWidths
    bp.logTagTableSizes = logTagTableSizes
    bp.tagTableCounterBits = tagTableCounterBits
    bp.tagTableUBits = tagTableUBits

    storage = sum([2**s * (t + tagTableCounterBits + tagTableUBits)
        for (t,s) in zip(tagTableTagWidths[1:],logTagTableSizes[1:])])
    storage += 1024 * 10 * 2**(logTagTableSizes[0]-13)

    return (bp, int(storage))

def tage16_(bp, n):
    n -= 2
    tagTableTagWidths =
        [0, 8, 8, 11, 11, 11, 11, 11, 13, 13, 13, 13, 13, 13, 14, 14]
    logTagTableSizes =
        [n-4, n-7, n-7, n-5, n-5, n-5, n-5, n-5, n-6, n-6, n-6, n-6,
            n-6, n-6, n-9, n-9]
    tagTableCounterBits = 3
    tagTableUBits = 1

    bp.nHistoryTables = 15
    bp.minHist = 3
    bp.maxHist = 1930
    bp.tagTableTagWidths = tagTableTagWidths
    bp.logTagTableSizes = logTagTableSizes
    bp.tagTableCounterBits = tagTableCounterBits
    bp.tagTableUBits = tagTableUBits

    storage = sum([2**s * (t + tagTableCounterBits + tagTableUBits)
        for (t,s) in zip(tagTableTagWidths[1:],logTagTableSizes[1:])])
    storage += 1024 * 10 * 2**(logTagTableSizes[0]-13)

    return (bp, int(storage))

def tage5(n):
    bp = TAGE()
    return tage5_(bp, n)

def tage8(n):
    bp = TAGE()
    return tage8_(bp, n)

def tage13(n):
    bp = TAGE()
    return tage13_(bp, n)

def tage16(n):
    bp = TAGE()
    return tage16_(bp, n)

def ltage_(bp, n, s):
    logSizeLoopPred = n-10
    loopTableIterBits = 14
    loopTableAgeBits = 8
    loopTableTagBits = 14
    loopTableConfidenceBits = 2

    bp.logSizeLoopPred = logSizeLoopPred
    bp.logLoopTableAssoc = 2

    s += 2**logSizeLoopPred * (loopTableIterBits * 2 + loopTableAgeBits +
        loopTableConfidenceBits + loopTableTagBits)

    return (bp, int(s))

def ltage5(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = tage5(n)
    (bp,s) = ltage_(bp,n,s)
    return (bp, int(s))

def ltage8(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = tage8(n)
    (bp,s) = ltage_(bp,n)
    return (bp, int(s))

def ltage13(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = tage13(n)
    (bp,s) = ltage_(bp,n,s)
    return (bp, int(s))

def ltage16(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = tage16(n)
    (bp,s) = ltage_(bp,n,s)
    return (bp, int(s))

def stage_(bp,n,s1):
    logSize = n-19+12
    numTables = 5
    tableEntryBits = 6

    bp.logSize = logSize
    bp.numTables = numTables
    bp.tableEntryBits = 6

    s1 += 2**logSize * tableEntryBits
    return (bp, int(s1))

def stage5(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = tage5(n)
    (bp,s) = stage_(bp,n,s)
    return (bp, int(s))

def stage8(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = tage8(n)
    (bp,s) = stage_(bp,n,s)
    return (bp, int(s))

def stage13(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = tage13(n)
    (bp,s) = stage_(bp,n,s)
    return (bp, int(s))

def stage16(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = tage16(n)
    (bp,s) = stage_(bp,n,s)
    return (bp, int(s))

def sltage5(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = stage5(n)
    return ltage_(bp,n,s)

def sltage8(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = stage8(n)
    return ltage_(bp,n,s)

def sltage13(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = stage13(n)
    return ltage_(bp,n,s)

def sltage16(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = stage16(n)
    return ltage_(bp,n,s)

def itage_(bp,n,s1):
    logSize = n-19+6
    bp.logSize = logSize

    s1 += 2**logSize * 20
    return (bp, int(s1))

def itage5(n):
    bp = IUM()
    (bp.basePredictor,s) = tage5(n)
    return itage_(bp,n,s)

def itage8(n):
    bp = IUM()
    (bp.basePredictor,s) = tage8(n)
    return itage_(bp,n,s)

def itage13(n):
    bp = IUM()
    (bp.basePredictor,s) = tage13(n)
    return itage_(bp,n,s)

def itage16(n):
    bp = IUM()
    (bp.basePredictor,s) = tage16(n)
    return itage_(bp,n,s)

def istage5(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = itage5(n)
    return stage_(bp,n,s)

def istage8(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = itage8(n)
    return stage_(bp,n,s)

def istage13(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = itage13(n)
    return stage_(bp,n,s)

def istage16(n):
    bp = StatisticalCorrector()
    (bp.basePredictor,s) = itage16(n)
    return stage_(bp,n,s)

def isltage5(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = istage5(n)
    return ltage_(bp,n,s)

def isltage8(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = istage8(n)
    return ltage_(bp,n,s)

def isltage13(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = istage13(n)
    return ltage_(bp,n,s)

def isltage16(n):
    bp = LoopPredictor()
    (bp.basePredictor,s) = istage16(n)
    return ltage_(bp,n,s)

def tagelsc_(bp,n,s):
    entryBits = 6
    logSize = n-19+10
    numTables = 5

    bp.entryBits = entryBits
    bp.logSize = logSize
    bp.numTables = numTables

    s += entryBits * numTables * (1 << logSize)
    return (bp,int(s))

def tagelsc5(n):
    bp = LStatisticalCorrector()
    (bp.basePredictor,s) = itage5(n)
    return tagelsc_(bp,n,s)

def tagelsc8(n):
    bp = LStatisticalCorrector()
    (bp.basePredictor,s) = itage8(n)
    return tagelsc_(bp,n,s)

def tagelsc13(n):
    bp = LStatisticalCorrector()
    (bp.basePredictor,s) = itage13(n)
    return tagelsc_(bp,n,s)

def tagelsc16(n):
    bp = LStatisticalCorrector()
    (bp.basePredictor,s) = itage16(n)
    return tagelsc_(bp,n,s)

def BP(opts):
    n = opts.mult
    return globals()[opts.pred](n)
