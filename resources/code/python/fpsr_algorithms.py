# SPDX-License-Identifier: Apache-2.0 — See LICENSE for full terms
# Created by Patrick Woo, 2025.
# This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
# https://github.com/patwooky/fpsr

'''
file: fpsr_algorithms.py
brief: Python implementation of FPS-R algorithms: 
    Stacked Modulo (SM), Toggled Modulo (TM), Quantised Switching (QS), and Bitwise Decode (BD).
details: 
    FPS-R (Frame-Persistent Stateless Randomisation) is a set of algorithms that
    generate frame-persistent and stateless random values. 
    This file contains four stateless, frame-persistent randomization algorithms.
    It uses a custom portable_rand() function to ensure deterministic and consistent results across any platform.
'''

import math

# -----------------------------------------------------------------------------
# Deterministic helpers and PRNG matching the C reference implementation
# -----------------------------------------------------------------------------
# Why these helpers?
# - Python's % and // already use floor semantics for negatives, which we mirror in C.
#   We still define explicit helpers so both languages call the same logical steps.
# - All frame/seed/duration math remains in the integer domain (Python int is arbitrary
#   precision). Where the C code relies on uint64_t wraparound, we emulate it with masks.
# - All fractional math that converts to/from integers uses Python's float (IEEE-754
#   double) and math.floor to match C 'double' behavior, ensuring bit-for-bit parity.

# Bit-width used for chunked bit operations. It must remain 64 for deterministic
# compatibility with SplitMix64 and the 64-bit masking below.
# This value is not meant to be changed. DO NOT MODIFY.
_CHUNK_BITS = 64

# 64-bit mask for emulating uint64_t wraparound exactly like C.
_UINT64_MASK = (1 << _CHUNK_BITS) - 1

def _to_uint64(x: int) -> int:
    """Cast any Python int to an emulated uint64_t by masking to 64 bits.
    This reproduces C's well-defined unsigned wraparound and is essential for
    deterministic PRNG behavior across languages and platforms.
    """
    return x & _UINT64_MASK

# Floor-based modulo that matches Python's a % m for negative a (m>0).
# We expose it explicitly to mirror the C helper and document the determinism intent.
def i64_floor_mod(a: int, m: int) -> int:
    """Return a modulo m using floor semantics, identical to Python's % for m>0.
    C's % truncates toward zero, which diverges for negative a; by always using
    floor-mod here (and in C), we guarantee alignment logic matches exactly.
    """
    # Assumes m > 0 by contract.
    return a % m

# Align down to the nearest multiple of m using floor-based modulo.
# This mirrors C's i64_align_down and Python's 'a - (a % m)' even when a < 0.
def i64_align_down(a: int, m: int) -> int:
    """Align a down to a multiple of m using floor-mod semantics.
    Using this helper wherever alignment is needed ensures C/Python parity.
    """
    return a - i64_floor_mod(a, m)

# SplitMix64: portable 64-bit mixer with well-defined unsigned wraparound.
# Each arithmetic step is masked to uint64 to exactly mirror C's uint64_t behavior.
def _splitmix64(x: int) -> int:
    x = _to_uint64(x + 0x9E3779B97F4A7C15)
    x = _to_uint64((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9)
    x = _to_uint64((x ^ (x >> 27)) * 0x94D049BB133111EB)
    x = _to_uint64(x ^ (x >> 31))
    return x

# Portable, deterministic PRNG that returns a double in [0,1).
# Uses the top 53 bits of the 64-bit output to match IEEE-754 double mantissa size.
# Implemented identically in C and Python to yield bit-for-bit identical floats.
def portable_rand_u64(seed: int) -> float:
    # [PORT UPDATE] C-version passes a uint64_t. We emulate this by masking
    # the input seed *first* before passing to splitmix64.
    r = _splitmix64(_to_uint64(seed))
    return float((r >> 11)) * (1.0 / 9007199254740992.0)  # 2^53

# Back-compat wrapper with the old name/signature.
# Prefer passing integer seeds; if a float is provided (legacy), we floor it to
# remove ambiguity and to align with C's explicit use of floor() before casts.
def portable_rand(seed):
    """
    A simple, portable pseudo-random number generator.
    Generates a deterministic float between 0.0 and 1.0 from an integer-like seed.
    This wrapper forwards to a 64-bit deterministic PRNG that mirrors the C code
    (SplitMix64 + top-53-bit mapping), ensuring cross-language bit-for-bit parity.

    Args:
        seed (int|float): An integer-like seed. Floats are floored for determinism.

    Returns:
        float: A pseudo-random float between 0.0 and 1.0.
    """
    if isinstance(seed, float):
        seed = math.floor(seed)
    else:
        seed = int(seed)
    return portable_rand_u64(seed)

# --- Bitwise Rotation Helpers ---

def _circular_left_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular left shift (rotate left)."""
    # [PORT UPDATE] Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value << shift) | (value >> (_CHUNK_BITS - shift)))

def _circular_right_shift(value: int, shift: int) -> int:
    """Performs a _CHUNK_BITS-wide circular right shift (rotate right)."""
    # [PORT UPDATE] Ensure shift is within [0, CHUNK_BITS-1]
    # to match C's modulo behavior.
    shift %= _CHUNK_BITS
    if shift == 0: return _to_uint64(value)
    return _to_uint64((value >> shift) | (value << (_CHUNK_BITS - shift)))

# -----------------------------------------------------------------------------
# Wavetable Type & Sampler
# -----------------------------------------------------------------------------

class FPSR_Wavetable:
    """
    Lightweight container for power-of-2 cyclical wavetables.
    Pass None to functions to use the default 1024-entry unipolar sine table.
    """
    def __init__(self, samples: list):
        self.samples = samples
        self.size = len(samples)

_TWO_PI = 6.28318530718

# Python Sine Wavetable
# Auto-generated 1024-point sine lookup table
# Maps normalized phase [0.0, 1.0) to [0.0, 1.0] (unipolar)
_fpsr_sine_lut_1024 = [
    0.5000000000000000,
    0.5030679423245772,
    0.5061357691428600,
    0.5092033649529024,
    0.5122706142614561,
    0.5153374015883183,
    0.5184036114706794,
    0.5214691284674704,
    0.5245338371637090,
    0.5275976221748450,
    0.5306603681511043,
    0.5337219597818320,
    0.5367822817998337,
    0.5398412189857150,
    0.5428986561722200,
    0.5459544782485664,
    0.5490085701647803,
    0.5520608169360273,
    0.5551111036469415,
    0.5581593154559523,
    0.5612053375996081,
    0.5642490553968966,
    0.5672903542535631,
    0.5703291196664246,
    0.5733652372276808,
    0.5763985926292217,
    0.5794290716669307,
    0.5824565602449849,
    0.5854809443801506,
    0.5885021102060743,
    0.5915199439775705,
    0.5945343320749031,
    0.5975451610080641,
    0.6005523174210460,
    0.6035556880961093,
    0.6065551599580457,
    0.6095506200784349,
    0.6125419556798964,
    0.6155290541403355,
    0.6185118029971836,
    0.6214900899516319,
    0.6244638028728601,
    0.6274328298022573,
    0.6303970589576378,
    0.6333563787374492,
    0.6363106777249745,
    0.6392598446925265,
    0.6422037686056359,
    0.6451423386272311,
    0.6480754441218119,
    0.6510029746596140,
    0.6539248200207675,
    0.6568408701994457,
    0.6597510154080078,
    0.6626551460811314,
    0.6655531528799382,
    0.6684449266961100,
    0.6713303586559972,
    0.6742093401247173,
    0.6770817627102452,
    0.6799475182674941,
    0.6828064989023870,
    0.6856585969759188,
    0.6885037051082091,
    0.6913417161825449,
    0.6941725233494132,
    0.6969960200305241,
    0.6998120999228234,
    0.7026206570024949,
    0.7054215855289520,
    0.7082147800488185,
    0.7110001353998998,
    0.7137775467151410,
    0.7165469094265760,
    0.7193081192692639,
    0.7220610722852145,
    0.7248056648273032,
    0.7275417935631719,
    0.7302693554791200,
    0.7329882478839831,
    0.7356983684129988,
    0.7383996150316611,
    0.7410918860395613,
    0.7437750800742180,
    0.7464490961148920,
    0.7491138334863909,
    0.7517691918628588,
    0.7544150712715535,
    0.7570513720966108,
    0.7596779950827948,
    0.7622948413392345,
    0.7649018123431472,
    0.7674988099435486,
    0.7700857363649465,
    0.7726624942110232,
    0.7752289864683024,
    0.7777851165098011,
    0.7803307880986681,
    0.7828659053918066,
    0.7853903729434837,
    0.7879040957089227,
    0.7904069790478823,
    0.7928989287282194,
    0.7953798509294371,
    0.7978496522462166,
    0.8003082396919345,
    0.8027555207021628,
    0.8051914031381547,
    0.8076157952903134,
    0.8100286058816446,
    0.8124297440711932,
    0.8148191194574634,
    0.8171966420818227,
    0.8195622224318879,
    0.8219157714448957,
    0.8242572005110562,
    0.8265864214768883,
    0.8289033466485394,
    0.8312078887950859,
    0.8334999611518188,
    0.8357794774235092,
    0.8380463517876580,
    0.8403004988977265,
    0.8425418338863502,
    0.8447702723685334,
    0.8469857304448269,
    0.8491881247044863,
    0.8513773722286126,
    0.8535533905932737,
    0.8557160978726082,
    0.8578654126419093,
    0.8600012539806908,
    0.8621235414757334,
    0.8642321952241125,
    0.8663271358362064,
    0.8684082844386849,
    0.8704755626774796,
    0.8725288927207330,
    0.8745681972617296,
    0.8765933995218063,
    0.8786044232532423,
    0.8806011927421309,
    0.8825836328112295,
    0.8845516688227898,
    0.8865052266813684,
    0.8884442328366162,
    0.8903686142860472,
    0.8922782985777876,
    0.8941732138133032,
    0.8960532886501061,
    0.8979184523044417,
    0.8997686345539525,
    0.9016037657403224,
    0.9034237767718996,
    0.9052285991262974,
    0.9070181648529742,
    0.9087924065757919,
    0.9105512574955523,
    0.9122946513925126,
    0.9140225226288778,
    0.9157348061512727,
    0.9174314374931900,
    0.9191123527774190,
    0.9207774887184492,
    0.9224267826248536,
    0.9240601724016486,
    0.9256775965526326,
    0.9272789941827002,
    0.9288643050001361,
    0.9304334693188836,
    0.9319864280607933,
    0.9335231227578463,
    0.9350434955543556,
    0.9365474892091450,
    0.9380350470977032,
    0.9395061132143168,
    0.9409606321741775,
    0.9423985492154690,
    0.9438198102014270,
    0.9452243616223790,
    0.9466121505977576,
    0.9479831248780926,
    0.9493372328469769,
    0.9506744235230110,
    0.9519946465617217,
    0.9532978522574577,
    0.9545839915452612,
    0.9558530160027150,
    0.9571048778517653,
    0.9583395299605213,
    0.9595569258450289,
    0.9607570196710209,
    0.9619397662556434,
    0.9631051210691557,
    0.9642530402366079,
    0.9653834805394919,
    0.9664963994173694,
    0.9675917549694737,
    0.9686695059562875,
    0.9697296118010950,
    0.9707720325915103,
    0.9717967290809801,
    0.9728036626902606,
    0.9737927955088705,
    0.9747640902965183,
    0.9757175104845042,
    0.9766530201770969,
    0.9775705841528853,
    0.9784701678661045,
    0.9793517374479358,
    0.9802152597077829,
    0.9810607021345208,
    0.9818880328977200,
    0.9826972208488447,
    0.9834882355224260,
    0.9842610471372086,
    0.9850156265972720,
    0.9857519454931258,
    0.9864699761027800,
    0.9871696913927879,
    0.9878510650192642,
    0.9885140713288771,
    0.9891586853598138,
    0.9897848828427203,
    0.9903926402016152,
    0.9909819345547777,
    0.9915527437156082,
    0.9921050461934645,
    0.9926388211944706,
    0.9931540486222994,
    0.9936507090789293,
    0.9941287838653747,
    0.9945882549823906,
    0.9950291051311486,
    0.9954513177138899,
    0.9958548768345498,
    0.9962397672993550,
    0.9966059746173972,
    0.9969534850011781,
    0.9972822853671277,
    0.9975923633360984,
    0.9978837072338299,
    0.9981563060913889,
    0.9984101496455828,
    0.9986452283393451,
    0.9988615333220958,
    0.9990590564500745,
    0.9992377902866474,
    0.9993977281025862,
    0.9995388638763227,
    0.9996611922941747,
    0.9997647087505466,
    0.9998494093481021,
    0.9999152908979116,
    0.9999623509195723,
    0.9999905876413006,
    1.0000000000000000,
    0.9999905876413006,
    0.9999623509195723,
    0.9999152908979116,
    0.9998494093481021,
    0.9997647087505466,
    0.9996611922941747,
    0.9995388638763227,
    0.9993977281025862,
    0.9992377902866474,
    0.9990590564500745,
    0.9988615333220958,
    0.9986452283393451,
    0.9984101496455828,
    0.9981563060913889,
    0.9978837072338299,
    0.9975923633360985,
    0.9972822853671277,
    0.9969534850011781,
    0.9966059746173972,
    0.9962397672993550,
    0.9958548768345498,
    0.9954513177138899,
    0.9950291051311486,
    0.9945882549823906,
    0.9941287838653747,
    0.9936507090789293,
    0.9931540486222994,
    0.9926388211944706,
    0.9921050461934645,
    0.9915527437156082,
    0.9909819345547777,
    0.9903926402016152,
    0.9897848828427203,
    0.9891586853598138,
    0.9885140713288771,
    0.9878510650192642,
    0.9871696913927879,
    0.9864699761027801,
    0.9857519454931258,
    0.9850156265972720,
    0.9842610471372086,
    0.9834882355224260,
    0.9826972208488447,
    0.9818880328977200,
    0.9810607021345208,
    0.9802152597077829,
    0.9793517374479358,
    0.9784701678661045,
    0.9775705841528853,
    0.9766530201770969,
    0.9757175104845042,
    0.9747640902965183,
    0.9737927955088705,
    0.9728036626902608,
    0.9717967290809801,
    0.9707720325915103,
    0.9697296118010950,
    0.9686695059562875,
    0.9675917549694738,
    0.9664963994173694,
    0.9653834805394919,
    0.9642530402366079,
    0.9631051210691557,
    0.9619397662556434,
    0.9607570196710210,
    0.9595569258450289,
    0.9583395299605213,
    0.9571048778517653,
    0.9558530160027150,
    0.9545839915452612,
    0.9532978522574577,
    0.9519946465617217,
    0.9506744235230110,
    0.9493372328469769,
    0.9479831248780926,
    0.9466121505977576,
    0.9452243616223790,
    0.9438198102014270,
    0.9423985492154690,
    0.9409606321741775,
    0.9395061132143168,
    0.9380350470977032,
    0.9365474892091451,
    0.9350434955543557,
    0.9335231227578464,
    0.9319864280607935,
    0.9304334693188836,
    0.9288643050001361,
    0.9272789941827002,
    0.9256775965526326,
    0.9240601724016486,
    0.9224267826248536,
    0.9207774887184492,
    0.9191123527774191,
    0.9174314374931900,
    0.9157348061512727,
    0.9140225226288778,
    0.9122946513925125,
    0.9105512574955523,
    0.9087924065757919,
    0.9070181648529743,
    0.9052285991262974,
    0.9034237767718998,
    0.9016037657403224,
    0.8997686345539526,
    0.8979184523044418,
    0.8960532886501061,
    0.8941732138133032,
    0.8922782985777875,
    0.8903686142860473,
    0.8884442328366162,
    0.8865052266813686,
    0.8845516688227898,
    0.8825836328112295,
    0.8806011927421309,
    0.8786044232532424,
    0.8765933995218063,
    0.8745681972617296,
    0.8725288927207331,
    0.8704755626774795,
    0.8684082844386850,
    0.8663271358362064,
    0.8642321952241127,
    0.8621235414757334,
    0.8600012539806909,
    0.8578654126419094,
    0.8557160978726084,
    0.8535533905932737,
    0.8513773722286126,
    0.8491881247044865,
    0.8469857304448269,
    0.8447702723685335,
    0.8425418338863502,
    0.8403004988977266,
    0.8380463517876580,
    0.8357794774235092,
    0.8334999611518188,
    0.8312078887950860,
    0.8289033466485394,
    0.8265864214768883,
    0.8242572005110562,
    0.8219157714448957,
    0.8195622224318879,
    0.8171966420818227,
    0.8148191194574637,
    0.8124297440711932,
    0.8100286058816447,
    0.8076157952903135,
    0.8051914031381548,
    0.8027555207021628,
    0.8003082396919344,
    0.7978496522462167,
    0.7953798509294371,
    0.7928989287282195,
    0.7904069790478823,
    0.7879040957089227,
    0.7853903729434837,
    0.7828659053918068,
    0.7803307880986681,
    0.7777851165098011,
    0.7752289864683024,
    0.7726624942110232,
    0.7700857363649465,
    0.7674988099435486,
    0.7649018123431475,
    0.7622948413392345,
    0.7596779950827949,
    0.7570513720966109,
    0.7544150712715536,
    0.7517691918628588,
    0.7491138334863909,
    0.7464490961148921,
    0.7437750800742180,
    0.7410918860395614,
    0.7383996150316611,
    0.7356983684129990,
    0.7329882478839831,
    0.7302693554791201,
    0.7275417935631719,
    0.7248056648273035,
    0.7220610722852147,
    0.7193081192692637,
    0.7165469094265761,
    0.7137775467151410,
    0.7110001353998999,
    0.7082147800488185,
    0.7054215855289521,
    0.7026206570024950,
    0.6998120999228236,
    0.6969960200305241,
    0.6941725233494133,
    0.6913417161825449,
    0.6885037051082090,
    0.6856585969759188,
    0.6828064989023869,
    0.6799475182674941,
    0.6770817627102452,
    0.6742093401247173,
    0.6713303586559972,
    0.6684449266961101,
    0.6655531528799382,
    0.6626551460811316,
    0.6597510154080080,
    0.6568408701994457,
    0.6539248200207675,
    0.6510029746596140,
    0.6480754441218119,
    0.6451423386272312,
    0.6422037686056361,
    0.6392598446925266,
    0.6363106777249746,
    0.6333563787374492,
    0.6303970589576380,
    0.6274328298022573,
    0.6244638028728601,
    0.6214900899516320,
    0.6185118029971836,
    0.6155290541403357,
    0.6125419556798964,
    0.6095506200784351,
    0.6065551599580457,
    0.6035556880961094,
    0.6005523174210460,
    0.5975451610080643,
    0.5945343320749031,
    0.5915199439775705,
    0.5885021102060745,
    0.5854809443801506,
    0.5824565602449850,
    0.5794290716669307,
    0.5763985926292219,
    0.5733652372276810,
    0.5703291196664247,
    0.5672903542535631,
    0.5642490553968965,
    0.5612053375996082,
    0.5581593154559523,
    0.5551111036469416,
    0.5520608169360273,
    0.5490085701647804,
    0.5459544782485664,
    0.5428986561722201,
    0.5398412189857151,
    0.5367822817998339,
    0.5337219597818321,
    0.5306603681511043,
    0.5275976221748451,
    0.5245338371637089,
    0.5214691284674705,
    0.5184036114706794,
    0.5153374015883184,
    0.5122706142614561,
    0.5092033649529025,
    0.5061357691428600,
    0.5030679423245774,
    0.5000000000000001,
    0.4969320576754227,
    0.4938642308571401,
    0.4907966350470976,
    0.4877293857385440,
    0.4846625984116817,
    0.4815963885293207,
    0.4785308715325296,
    0.4754661628362911,
    0.4724023778251551,
    0.4693396318488959,
    0.4662780402181680,
    0.4632177182001663,
    0.4601587810142850,
    0.4571013438277801,
    0.4540455217514338,
    0.4509914298352197,
    0.4479391830639728,
    0.4448888963530585,
    0.4418406845440478,
    0.4387946624003919,
    0.4357509446031036,
    0.4327096457464370,
    0.4296708803335754,
    0.4266347627723192,
    0.4236014073707783,
    0.4205709283330694,
    0.4175434397550151,
    0.4145190556198495,
    0.4114978897939257,
    0.4084800560224296,
    0.4054656679250970,
    0.4024548389919358,
    0.3994476825789541,
    0.3964443119038907,
    0.3934448400419544,
    0.3904493799215651,
    0.3874580443201037,
    0.3844709458596645,
    0.3814881970028166,
    0.3785099100483681,
    0.3755361971271400,
    0.3725671701977428,
    0.3696029410423622,
    0.3666436212625509,
    0.3636893222750255,
    0.3607401553074736,
    0.3577962313943641,
    0.3548576613727690,
    0.3519245558781881,
    0.3489970253403861,
    0.3460751799792326,
    0.3431591298005544,
    0.3402489845919922,
    0.3373448539188685,
    0.3344468471200619,
    0.3315550733038899,
    0.3286696413440029,
    0.3257906598752827,
    0.3229182372897549,
    0.3200524817325059,
    0.3171935010976132,
    0.3143414030240813,
    0.3114962948917910,
    0.3086582838174552,
    0.3058274766505868,
    0.3030039799694760,
    0.3001879000771766,
    0.2973793429975051,
    0.2945784144710480,
    0.2917852199511816,
    0.2889998646001002,
    0.2862224532848591,
    0.2834530905734240,
    0.2806918807307364,
    0.2779389277147855,
    0.2751943351726966,
    0.2724582064368282,
    0.2697306445208800,
    0.2670117521160170,
    0.2643016315870012,
    0.2616003849683390,
    0.2589081139604387,
    0.2562249199257822,
    0.2535509038851080,
    0.2508861665136092,
    0.2482308081371413,
    0.2455849287284464,
    0.2429486279033892,
    0.2403220049172052,
    0.2377051586607656,
    0.2350981876568527,
    0.2325011900564515,
    0.2299142636350536,
    0.2273375057889769,
    0.2247710135316976,
    0.2222148834901990,
    0.2196692119013320,
    0.2171340946081934,
    0.2146096270565164,
    0.2120959042910773,
    0.2095930209521178,
    0.2071010712717806,
    0.2046201490705630,
    0.2021503477537834,
    0.1996917603080657,
    0.1972444792978373,
    0.1948085968618453,
    0.1923842047096866,
    0.1899713941183554,
    0.1875702559288069,
    0.1851808805425365,
    0.1828033579181774,
    0.1804377775681121,
    0.1780842285551044,
    0.1757427994889438,
    0.1734135785231117,
    0.1710966533514607,
    0.1687921112049141,
    0.1665000388481813,
    0.1642205225764908,
    0.1619536482123421,
    0.1596995011022735,
    0.1574581661136499,
    0.1552297276314666,
    0.1530142695551731,
    0.1508118752955136,
    0.1486226277713875,
    0.1464466094067263,
    0.1442839021273918,
    0.1421345873580908,
    0.1399987460193092,
    0.1378764585242666,
    0.1357678047758874,
    0.1336728641637936,
    0.1315917155613151,
    0.1295244373225206,
    0.1274711072792671,
    0.1254318027382705,
    0.1234066004781938,
    0.1213955767467579,
    0.1193988072578690,
    0.1174163671887705,
    0.1154483311772103,
    0.1134947733186317,
    0.1115557671633837,
    0.1096313857139528,
    0.1077217014222125,
    0.1058267861866971,
    0.1039467113498938,
    0.1020815476955583,
    0.1002313654460476,
    0.0983962342596775,
    0.0965762232281004,
    0.0947714008737027,
    0.0929818351470260,
    0.0912075934242081,
    0.0894487425044477,
    0.0877053486074875,
    0.0859774773711223,
    0.0842651938487274,
    0.0825685625068101,
    0.0808876472225811,
    0.0792225112815507,
    0.0775732173751464,
    0.0759398275983514,
    0.0743224034473676,
    0.0727210058172997,
    0.0711356949998640,
    0.0695665306811165,
    0.0680135719392068,
    0.0664768772421537,
    0.0649565044456443,
    0.0634525107908551,
    0.0619649529022966,
    0.0604938867856833,
    0.0590393678258225,
    0.0576014507845312,
    0.0561801897985730,
    0.0547756383776211,
    0.0533878494022424,
    0.0520168751219076,
    0.0506627671530231,
    0.0493255764769890,
    0.0480053534382784,
    0.0467021477425423,
    0.0454160084547388,
    0.0441469839972851,
    0.0428951221482348,
    0.0416604700394786,
    0.0404430741549712,
    0.0392429803289791,
    0.0380602337443567,
    0.0368948789308443,
    0.0357469597633923,
    0.0346165194605082,
    0.0335036005826305,
    0.0324082450305262,
    0.0313304940437126,
    0.0302703881989052,
    0.0292279674084896,
    0.0282032709190199,
    0.0271963373097394,
    0.0262072044911294,
    0.0252359097034817,
    0.0242824895154958,
    0.0233469798229032,
    0.0224294158471146,
    0.0215298321338956,
    0.0206482625520643,
    0.0197847402922172,
    0.0189392978654792,
    0.0181119671022801,
    0.0173027791511554,
    0.0165117644775739,
    0.0157389528627914,
    0.0149843734027280,
    0.0142480545068742,
    0.0135300238972199,
    0.0128303086072121,
    0.0121489349807358,
    0.0114859286711229,
    0.0108413146401862,
    0.0102151171572797,
    0.0096073597983848,
    0.0090180654452223,
    0.0084472562843919,
    0.0078949538065355,
    0.0073611788055294,
    0.0068459513777007,
    0.0063492909210708,
    0.0058712161346253,
    0.0054117450176095,
    0.0049708948688514,
    0.0045486822861100,
    0.0041451231654502,
    0.0037602327006450,
    0.0033940253826027,
    0.0030465149988220,
    0.0027177146328723,
    0.0024076366639015,
    0.0021162927661701,
    0.0018436939086110,
    0.0015898503544172,
    0.0013547716606549,
    0.0011384666779042,
    0.0009409435499254,
    0.0007622097133526,
    0.0006022718974138,
    0.0004611361236773,
    0.0003388077058253,
    0.0002352912494534,
    0.0001505906518979,
    0.0000847091020883,
    0.0000376490804277,
    0.0000094123586994,
    0.0000000000000000,
    0.0000094123586994,
    0.0000376490804277,
    0.0000847091020883,
    0.0001505906518979,
    0.0002352912494534,
    0.0003388077058252,
    0.0004611361236773,
    0.0006022718974138,
    0.0007622097133526,
    0.0009409435499254,
    0.0011384666779042,
    0.0013547716606549,
    0.0015898503544172,
    0.0018436939086110,
    0.0021162927661701,
    0.0024076366639015,
    0.0027177146328723,
    0.0030465149988220,
    0.0033940253826027,
    0.0037602327006450,
    0.0041451231654502,
    0.0045486822861100,
    0.0049708948688514,
    0.0054117450176095,
    0.0058712161346253,
    0.0063492909210708,
    0.0068459513777006,
    0.0073611788055294,
    0.0078949538065354,
    0.0084472562843918,
    0.0090180654452223,
    0.0096073597983848,
    0.0102151171572797,
    0.0108413146401861,
    0.0114859286711229,
    0.0121489349807357,
    0.0128303086072120,
    0.0135300238972199,
    0.0142480545068741,
    0.0149843734027280,
    0.0157389528627913,
    0.0165117644775739,
    0.0173027791511553,
    0.0181119671022800,
    0.0189392978654792,
    0.0197847402922171,
    0.0206482625520642,
    0.0215298321338955,
    0.0224294158471146,
    0.0233469798229031,
    0.0242824895154958,
    0.0252359097034816,
    0.0262072044911293,
    0.0271963373097394,
    0.0282032709190198,
    0.0292279674084895,
    0.0302703881989051,
    0.0313304940437125,
    0.0324082450305261,
    0.0335036005826305,
    0.0346165194605081,
    0.0357469597633922,
    0.0368948789308443,
    0.0380602337443567,
    0.0392429803289790,
    0.0404430741549711,
    0.0416604700394786,
    0.0428951221482347,
    0.0441469839972851,
    0.0454160084547388,
    0.0467021477425422,
    0.0480053534382783,
    0.0493255764769889,
    0.0506627671530230,
    0.0520168751219075,
    0.0533878494022423,
    0.0547756383776210,
    0.0561801897985729,
    0.0576014507845312,
    0.0590393678258225,
    0.0604938867856832,
    0.0619649529022965,
    0.0634525107908550,
    0.0649565044456443,
    0.0664768772421536,
    0.0680135719392067,
    0.0695665306811163,
    0.0711356949998639,
    0.0727210058172996,
    0.0743224034473675,
    0.0759398275983513,
    0.0775732173751464,
    0.0792225112815506,
    0.0808876472225810,
    0.0825685625068099,
    0.0842651938487273,
    0.0859774773711222,
    0.0877053486074874,
    0.0894487425044476,
    0.0912075934242080,
    0.0929818351470258,
    0.0947714008737026,
    0.0965762232281003,
    0.0983962342596774,
    0.1002313654460475,
    0.1020815476955582,
    0.1039467113498937,
    0.1058267861866969,
    0.1077217014222124,
    0.1096313857139526,
    0.1115557671633836,
    0.1134947733186316,
    0.1154483311772102,
    0.1174163671887704,
    0.1193988072578689,
    0.1213955767467577,
    0.1234066004781937,
    0.1254318027382702,
    0.1274711072792671,
    0.1295244373225204,
    0.1315917155613149,
    0.1336728641637934,
    0.1357678047758875,
    0.1378764585242665,
    0.1399987460193091,
    0.1421345873580905,
    0.1442839021273918,
    0.1464466094067262,
    0.1486226277713872,
    0.1508118752955137,
    0.1530142695551730,
    0.1552297276314664,
    0.1574581661136496,
    0.1596995011022735,
    0.1619536482123420,
    0.1642205225764907,
    0.1665000388481810,
    0.1687921112049141,
    0.1710966533514606,
    0.1734135785231115,
    0.1757427994889438,
    0.1780842285551043,
    0.1804377775681120,
    0.1828033579181770,
    0.1851808805425365,
    0.1875702559288068,
    0.1899713941183553,
    0.1923842047096863,
    0.1948085968618453,
    0.1972444792978372,
    0.1996917603080653,
    0.2021503477537834,
    0.2046201490705629,
    0.2071010712717805,
    0.2095930209521175,
    0.2120959042910774,
    0.2146096270565163,
    0.2171340946081932,
    0.2196692119013317,
    0.2222148834901989,
    0.2247710135316975,
    0.2273375057889766,
    0.2299142636350536,
    0.2325011900564514,
    0.2350981876568525,
    0.2377051586607653,
    0.2403220049172052,
    0.2429486279033891,
    0.2455849287284463,
    0.2482308081371409,
    0.2508861665136091,
    0.2535509038851079,
    0.2562249199257818,
    0.2589081139604387,
    0.2616003849683389,
    0.2643016315870010,
    0.2670117521160167,
    0.2697306445208800,
    0.2724582064368280,
    0.2751943351726965,
    0.2779389277147851,
    0.2806918807307361,
    0.2834530905734239,
    0.2862224532848587,
    0.2889998646001002,
    0.2917852199511813,
    0.2945784144710479,
    0.2973793429975048,
    0.3001879000771766,
    0.3030039799694759,
    0.3058274766505866,
    0.3086582838174548,
    0.3114962948917909,
    0.3143414030240811,
    0.3171935010976128,
    0.3200524817325060,
    0.3229182372897548,
    0.3257906598752826,
    0.3286696413440026,
    0.3315550733038900,
    0.3344468471200617,
    0.3373448539188683,
    0.3402489845919923,
    0.3431591298005542,
    0.3460751799792324,
    0.3489970253403857,
    0.3519245558781882,
    0.3548576613727688,
    0.3577962313943639,
    0.3607401553074732,
    0.3636893222750255,
    0.3666436212625507,
    0.3696029410423620,
    0.3725671701977428,
    0.3755361971271399,
    0.3785099100483679,
    0.3814881970028161,
    0.3844709458596645,
    0.3874580443201035,
    0.3904493799215649,
    0.3934448400419540,
    0.3964443119038907,
    0.3994476825789539,
    0.4024548389919356,
    0.4054656679250970,
    0.4084800560224295,
    0.4114978897939255,
    0.4145190556198491,
    0.4175434397550151,
    0.4205709283330692,
    0.4236014073707781,
    0.4266347627723188,
    0.4296708803335754,
    0.4327096457464368,
    0.4357509446031032,
    0.4387946624003920,
    0.4418406845440476,
    0.4448888963530583,
    0.4479391830639725,
    0.4509914298352197,
    0.4540455217514335,
    0.4571013438277798,
    0.4601587810142846,
    0.4632177182001663,
    0.4662780402181679,
    0.4693396318488955,
    0.4724023778251551,
    0.4754661628362910,
    0.4785308715325294,
    0.4815963885293203,
    0.4846625984116817,
    0.4877293857385438,
    0.4907966350470974,
    0.4938642308571397,
    0.4969320576754228
]

def fpsr_sample_wavetable(phase_normalized: float, wavetable=None) -> float:
    """
    Bitwise-modulo wavetable sampler with linear interpolation.
    Extracts values from a unipolar [0.0, 1.0] table given normalized phase [0.0, 1.0).
    Matches fpsr_sample_wavetable in the C reference implementation.
    """
    if isinstance(wavetable, FPSR_Wavetable):
        lut = wavetable.samples
    elif isinstance(wavetable, (list, tuple)):
        lut = wavetable
    else:
        lut = _fpsr_sine_lut_1024

    size = len(lut)
    mask = size - 1

    # Wrap phase into [0.0, 1.0)
    phase_normalized = phase_normalized - math.floor(phase_normalized)
    if phase_normalized < 0.0:
        phase_normalized += 1.0

    index_float = phase_normalized * float(size)
    index = int(index_float)
    frac = index_float - float(index)

    idx0 = index & mask
    idx1 = (index + 1) & mask

    return lut[idx0] * (1.0 - frac) + lut[idx1] * frac

# -----------------------------------------------------------------------------

"""
--------------------------
FPS-R: Stacked Modulo (SM)
--------------------------
"""

# [PORT UPDATE] Renamed to fpsr_sm_base to match C reference
def fpsr_sm_base(frame, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch=True):
    """
    Produces a pseudo-random value that persists across multiple frames, held for a calculated duration.
    The hold timing varies over time, driven by deterministic interference between reseeded modular rhythms.
    This method mimics structured hesitation and twitch-like behavior—creating motion that feels deliberate without relying on state or memory.

    Args:
        frame (int): The current frame or time input.
        minHold (int): The minimum duration (in frames) for a value to hold.
        maxHold (int): The maximum duration (in frames) for a value to hold.
        reseedInterval (int): The fixed interval at which a new hold duration is calculated.
        seedInner (int): An offset for the random duration calculation to create unique sequences.
        seedOuter (int): An offset for the final value calculation to create unique sequences.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw integer state value (as a float).
    """
    # --- 1. Calculate the random hold duration ---
    if reseedInterval < 1:
        reseedInterval = 1  # Prevent division by zero.

    # Use floor-based modulo to match C helper and Python semantics for negatives.
    # [PORT UPDATE] Cast seed to int to ensure portable_rand_u64 receives an int
    reseed_anchor = int(seedInner + frame) - i64_floor_mod(int(frame), int(reseedInterval))

    # Deterministic PRNG over 64-bit integer seed; result is double in [0,1).
    rand_for_duration = portable_rand_u64(reseed_anchor)

    # Compute duration with double intermediates then floor to int, mirroring C.
    holdDuration = math.floor(float(minHold) + rand_for_duration * float(maxHold - minHold))

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    # Align down using floor-mod semantics for negative inputs to ensure parity.
    held_integer_state = i64_align_down(int(seedOuter + frame), int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # [PORT UPDATE] Match the canonical C implementation.
        # The C version directly uses the 64-bit 'held_integer_state' as the
        # seed. The previous Python version multiplied this by 100,000.
        # This update removes that multiplication to align with the C reference.
        # portable_rand_u64 will handle casting to uint64 internally.
        fpsr_output = portable_rand_u64(held_integer_state)
    else:
        # Return the raw integer state as a float (matches C's cast).
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# Sample code to call the function
# Parameters
frame = 100  # Replace with the current frame value
minHoldFrames = 16  # probable minimum held period
maxHoldFrames = 24  # maximum held period before cycling
reseedFrames = 9    # inner mod cycle timing
offsetInner = -41   # offsets the inner frame
offsetOuter = 23    # offsets the outer frame
use_final_random = True # Set to False to bypass final randomization

# # Call the FPS-R:SM function
# randVal = fpsr_sm_base(frame, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random)
# # Another call to fpsr_sm for the previous frame
# randVal_previous = fpsr_sm_base(frame - 1, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, use_final_random)
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Stacked Modulo (SM) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}\n')

# end of fpsr_sm function


"""
---------------------------
FPS-R: Toggled Modulo (TM)
---------------------------
"""

# [PORT UPDATE] Renamed to fpsr_tm_base to match C reference
def fpsr_tm_base(frame, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch=True):
    """
    Generates a persistent value that holds for a rhythmically toggled duration.
    This function uses a deterministic switch to toggle the hold duration
    between two fixed periods. This creates a predictable, rhythmic, or mechanical
    "move-and-hold" pattern, as opposed to the organic randomness of SM.

    Args:
        frame (int): The current frame or time input.
        periodA (int): The first hold duration (in frames).
        periodB (int): The second hold duration (in frames).
        periodSwitch (int): The fixed interval at which the hold duration is toggled.
        seedInner (int): An offset for the toggle clock to de-sync it from the main clock.
        seedOuter (int): An offset for the main clock to create unique output sequences.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw integer state value (as a float).
    """
    # --- 1. Determine the hold duration by toggling between two periods ---
    if periodSwitch < 1:
        periodSwitch = 1  # Prevent division by zero.

    # The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    inner_clock_frame = int(seedInner + frame)
    
    # Use floor-based modulo for cross-language consistency with the C helper.
    r = i64_floor_mod(inner_clock_frame, int(periodSwitch))

    # Toggle threshold at exactly half the period using integer math (no FP rounding).
    holdDuration = periodA if (2 * r) < periodSwitch else periodB

    if holdDuration < 1:
        holdDuration = 1  # Prevent division by zero.

    # --- 2. Generate the stable integer "state" for the hold period ---
    outer_clock_frame = int(seedOuter + frame)
    held_integer_state = i64_align_down(outer_clock_frame, int(holdDuration))

    # --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    if finalRandSwitch:
        # [PORT UPDATE] Match the canonical C implementation.
        # The C version directly uses the 64-bit 'held_integer_state' as the
        # seed. The previous Python version multiplied this by 100,000.
        # This update removes that multiplication to align with the C reference.
        # portable_rand_u64 will handle casting to uint64 internally.
        fpsr_output = portable_rand_u64(held_integer_state)
    else:
        fpsr_output = float(held_integer_state)
    
    return fpsr_output

# Sample code to call the FPS-R:TM function
# Parameters
frame = 100  # Replace with the current frame value
period_A = 10  # The first hold duration
period_B = 25  # The second hold duration
switch_duration = 30  # The toggle happens every 30 frames
offset_inner = 15  # offsets the inner (toggle) clock
offset_outer = 0  # offsets the outer (hold) clock
use_final_random = True  # Set to False to bypass final randomization

# # Call the FPS-R:TM function
# randVal = fpsr_tm_base(frame, period_A, period_B, switch_duration, offset_inner, offset_outer, use_final_random)
# # Another call to fpsr_tm for the previous frame
# randVal_previous = fpsr_tm_base(frame - 1, period_A, period_B, switch_duration, offset_inner, offset_outer, use_final_random)
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Toggled Modulo (TM) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}\n')

# end of fpsr_tm function



"""
-------------------------------
FPS-R: Quantised Switching (QS)
-------------------------------
"""

def fpsr_qs_base(frame, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets,
                 streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch=True, wavetable=None):
    """
    Generates a flickering, quantised value by switching between two sine wave streams.
    For each stream, a new random quantisation level is chosen from within the [min, max] 
    range at a set interval. The function then switches between these two streams to create
    complex, glitch-like patterns.
    
    Args:
        frame (int): The current frame or time input.
        baseWaveFreq (float): The base frequency for the modulation wave of stream 1.
        stream2FreqMult (float): A multiplier for the second stream's frequency.
        quantLevelsMinMax (list[int]): A list of two integers for the min and max quantisation levels.
        streamsOffset (list[int]): A list of two integers to offset the frame for each stream.
        quantOffsets (list[int]): A list of two integers to offset the random quantisation selection.
        streamSwitchDur (int): The number of frames after which the streams switch.
        stream1QuantDur (int): The duration for which stream 1's random quantisation level is held.
        stream2QuantDur (int): The duration for which stream 2's random quantisation level is held.
        finalRandSwitch (bool): A flag to enable/disable the final randomisation step.
        wavetable (FPSR_Wavetable|list|None): Optional custom wavetable (None = default unipolar sine).

    Returns:
        float: If finalRandSwitch is True, a random value between 0.0 and 1.0. 
               If False, the raw stepped signal value, in the [0, 1] range.
    """
    # --- 1. Set default durations if not provided ---
    # Ensure durations are at least 1 frame to prevent division by zero.
    # We will keep the Python-side guards for safety.
    if baseWaveFreq == 0: baseWaveFreq = 0.01 # Avoid division by zero
    if streamSwitchDur < 1: streamSwitchDur = 1
    if stream1QuantDur < 1: stream1QuantDur = 1
    if stream2QuantDur < 1: stream2QuantDur = 1

    # --- 2. Calculate random quantisation levels for each stream ---
    quant_min = int(quantLevelsMinMax[0])
    quant_max = int(quantLevelsMinMax[1])
    quant_range = quant_max - quant_min + 1
    if quant_range < 1: quant_range = 1

    # --- Stream 1 Quant Level ---
    s1_quant_seed_aligned = i64_align_down(int(quantOffsets[0] + frame), stream1QuantDur)
    s1_rand_for_quant = portable_rand_u64(s1_quant_seed_aligned)
    s1_quant_level = quant_min + math.floor(s1_rand_for_quant * float(quant_range))

    # --- Stream 2 Quant Level ---
    s2_quant_seed_aligned = i64_align_down(int(quantOffsets[1] + frame), stream2QuantDur)
    s2_rand_for_quant = portable_rand_u64(s2_quant_seed_aligned)
    s2_quant_level = quant_min + math.floor(s2_rand_for_quant * float(quant_range))

    s1_quant_level = max(s1_quant_level, 1)
    s2_quant_level = max(s2_quant_level, 1)    # --- 3. Generate the two quantised wavetable streams ---
    if stream2FreqMult <= 0: stream2FreqMult = 3.7

    phase1 = (float(streamsOffset[0]) + float(frame)) * float(baseWaveFreq)
    phase2 = (float(streamsOffset[1]) + float(frame)) * float(baseWaveFreq) * float(stream2FreqMult)

    stream1_raw = fpsr_sample_wavetable(phase1, wavetable)
    stream2_raw = fpsr_sample_wavetable(phase2, wavetable)

    # Direct unipolar quantization [0.0, 1.0]
    stream1 = math.floor(stream1_raw * float(s1_quant_level)) / float(s1_quant_level)
    stream2 = math.floor(stream2_raw * float(s2_quant_level)) / float(s2_quant_level)

    # --- 4. Switch between the two streams ---
    r = i64_floor_mod(int(frame), streamSwitchDur)
    active_stream_val = stream1 if (2 * r) < streamSwitchDur else stream2

    # --- 5. Hash the final output to create a random-looking value (or bypass) ---
    if finalRandSwitch:
        hashed_int = math.floor(active_stream_val * 1000000.0)
        fpsr_output = portable_rand_u64(hashed_int)
    else:
        fpsr_output = active_stream_val
        
    return fpsr_output

# Sample code to call the FPS-R:QS function
# Parameters
frame = 103  # Current frame number
baseWaveFreq = 0.012  # Base frequency for the modulation wave of stream 1
stream2freqMult = 3.1  # Multiplier for the second stream's frequency
quantLevelsMinMax = [4, 12]  # Min, Max quantisation levels for the two streams
streamsOffset = [0, 76]  # Offset for the two streams' sine waves
quantOffsets = [10, 81] # Offset for the random quantisation selection
streamSwitchDur = 24  # Duration for switching streams in frames
stream1QuantDur = 16  # Duration for the first stream's quantisation switch cycle in frames
stream2QuantDur = 20  # Duration for the second stream's quantisation switch cycle in frames
use_final_random = True # Set to False to bypass final randomization

# # Call the FPS-R:QS function
# randVal = fpsr_qs(
#     frame, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
#     streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random
# )

# # Another call to fpsr_qs for the previous frame
# randVal_previous = fpsr_qs(
#     frame - 1, baseWaveFreq, stream2freqMult, quantLevelsMinMax, 
#     streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, use_final_random
# )
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Quantised Switching (QS) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}')

# end of fpsr_qs function

"""
------------------------------
FPS-R: Bitwise Decode (BD)
------------------------------
"""

def fpsr_bd_base(
    frame: int,
    block_size: int,
    streams_number: int = 1,
    streams_offset: int = 0,
    intra_op: str = "none",
    dynamic_shift_bits: int = 6,
    static_shift_amount: int = 1,
    inter_op: str = "xor",
    value_seed_offset: int = 0
):
    """
    Generates a phrased random value by decoding a deterministically generated bitstream.

    This algorithm is stateless. For any given frame, it calculates its state by:
    1. Finding the start of its macro-block (`outer_anchor`).
    2. Generating one or more raw bitstreams for the block.
    3. Applying transformations (intra-stream op) to each stream.
    4. Combining the transformed streams (inter-stream op).
    5. Decoding the final bitstream to produce phrased holds and jumps based on bit-flips.

    Args:
        frame (int): The current frame or time input.
        block_size (int): The size of the macro-rhythm in frames. Must be > 0.
        streams_number (int): The number of parallel bitstreams to generate.
        streams_offset (int): The frame offset between each parallel stream's seed.
        intra_op (str): The unary (intra-stream) operation.
                        Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
                        Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
        dynamic_shift_bits (int): For dynamic ops, the number of controller bits to determine shift amount.
        static_shift_amount (int): For static ops, the fixed number of bits to shift/rotate.
        inter_op (str): The binary (inter-stream) operation to combine streams ("xor", "or", "and").
        value_seed_offset (int): An additional seed offset for the final value calculation.
    Returns:
        float: A deterministic, phrased pseudo-random float between 0.0 and 1.0.
    """
    if block_size <= 0:
        block_size = 1
    if streams_number < 1:
        streams_number = 1

    # [PORT UPDATE] Match C's 'sanitized_static_shift'.
    # This ensures static shift amounts are always in the valid [0, 63] range
    # by masking, preventing undefined/inconsistent behavior for large shifts.
    sanitized_static_shift = static_shift_amount & (_CHUNK_BITS - 1)

    # --- Step 1: Find the Outer Anchor for the macro-block ---
    outer_anchor = i64_align_down(frame, block_size)

    # --- Step 2: Generate the raw bitstream(s) for the entire block ---
    num_chunks = (block_size + (_CHUNK_BITS - 1)) // _CHUNK_BITS
    raw_streams = []
    for i in range(streams_number):
        stream_seed = outer_anchor + (i * streams_offset)
        # [PORT UPDATE] Ensure seed components are int for uint64 emulation
        chunks = [_splitmix64(_to_uint64(int(stream_seed) + j)) for j in range(num_chunks)]
        raw_streams.append(chunks)

    # --- Step 3: Apply Intra-Stream Transformations ---
    transformed_streams = []
    unary_op = intra_op.lower()
    
    dynamic_ops = ["lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic"]
    is_dynamic = unary_op in dynamic_ops

    if is_dynamic:
        num_transformed_streams = (streams_number // 2) + (streams_number % 2)
        for i in range(0, streams_number // 2):
            data_stream = raw_streams[i * 2]
            controller_stream = raw_streams[i * 2 + 1]
            
            # [PORT UPDATE] Match C's logic for max_bits_for_shift
            # C: int max_bits_for_shift = 6; (for CHUNK_BITS=64)
            max_bits_for_shift = 6
            bit_mask_size = max(1, min(max_bits_for_shift, dynamic_shift_bits))
            bit_mask = (1 << bit_mask_size) - 1
            
            transformed_chunks = []
            for j in range(num_chunks):
                data_chunk = data_stream[j]
                controller_chunk = controller_stream[j]
                # [PORT UPDATE] Match C's logic: dynamic_shift is just the masked value
                dynamic_shift = (controller_chunk & bit_mask)
                # The modulo is applied during the shift/rotate call
                
                if unary_op == "lshift_dynamic":
                    # [PORT UPDATE] Apply modulo inside shift call to match C
                    transformed_chunks.append(_to_uint64(data_chunk << (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rshift_dynamic":
                    # [PORT UPDATE] Apply modulo inside shift call to match C
                    transformed_chunks.append(_to_uint64(data_chunk >> (dynamic_shift % _CHUNK_BITS)))
                elif unary_op == "rotl_dynamic":
                    # [PORT UPDATE] Pass raw dynamic_shift to helper, which will modulo
                    transformed_chunks.append(_circular_left_shift(data_chunk, dynamic_shift))
                elif unary_op == "rotr_dynamic":
                    # [PORT UPDATE] Pass raw dynamic_shift to helper, which will modulo
                    transformed_chunks.append(_circular_right_shift(data_chunk, dynamic_shift))
            transformed_streams.append(transformed_chunks)
        
        # [PORT UPDATE] Match C's logic for handling odd number of streams
        if streams_number % 2 != 0:
            transformed_streams.append(raw_streams[-1]) # Copy last stream as-is

    else: # Apply static operations
        num_transformed_streams = streams_number
        for stream_chunks in raw_streams:
            if unary_op == "not":
                transformed_chunks = [_to_uint64(~chunk) for chunk in stream_chunks]
            elif unary_op == "lshift":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_to_uint64(chunk << sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rshift":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_to_uint64(chunk >> sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotl":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_circular_left_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            elif unary_op == "rotr":
                # [PORT UPDATE] Use sanitized_static_shift
                transformed_chunks = [_circular_right_shift(chunk, sanitized_static_shift) for chunk in stream_chunks]
            else: # "none"
                transformed_chunks = list(stream_chunks) # "none", copy the stream
            transformed_streams.append(transformed_chunks)
        
    # --- Step 4: Combine Streams with Inter-Stream Operation ---
    # [PORT UPDATE] Match C's inter-op logic exactly.
    if num_transformed_streams > 0:
        # Start with a copy of the first transformed stream's chunks
        final_chunks = list(transformed_streams[0])
        
        op_map = { "xor": (lambda a, b: a ^ b), "or": (lambda a, b: a | b), "and": (lambda a, b: a & b) }
        chosen_op = op_map.get(inter_op.lower(), lambda a, b: a ^ b) # Default to xor

        # Loop from the *second* stream onwards
        for i in range(1, num_transformed_streams):
            for j in range(num_chunks):
                final_chunks[j] = chosen_op(final_chunks[j], transformed_streams[i][j])
                # Emulate uint64 wraparound
                final_chunks[j] = _to_uint64(final_chunks[j])
    else:
        # No streams, result is all zeros
        final_chunks = [0] * num_chunks


    # --- Step 5: Decode the final bitstream ---
    # We define get_bit inside fpsr_bd so it has access to
    # final_chunks, num_chunks, and block_size from its closure
    def get_bit(n):
        ''' 
        Helper to get a specific bit, matching C's logic
        
        n is the bit index in the final bitstream.
        
        returns 1 if the bit is set, 0 otherwise. Out-of-bounds returns 0.
        '''
        if not (0 <= n < block_size): return 0
        chunk_index, bit_index = n // _CHUNK_BITS, n % _CHUNK_BITS
        if chunk_index >= num_chunks: return 0
        return (final_chunks[chunk_index] >> bit_index) & 1

    current_pos_in_block = frame - outer_anchor
    last_flip_pos = 0
    
    # Scan backwards from current position to find the last bit-flip
    for i in range(current_pos_in_block, 0, -1):
        if get_bit(i) != get_bit(i - 1):
            last_flip_pos = i
            break
            
    # --- Step 6: Generate the final random value from the last bit-flip position ---
    # [PORT UPDATE] C logic relies on uint64 wraparound for the final seed addition.
    # We emulate this by summing the components as standard Python ints,
    # and portable_rand_u64 will handle the final _to_uint64 mask.
    final_seed = int(outer_anchor) + int(last_flip_pos) + int(value_seed_offset)
    
    return portable_rand_u64(final_seed)

# Sample code to call the FPS-R:BD function
# Parameters
frame = 100  # Current frame number
p_block_size = 64 # Size of the macro-rhythm in frames
p_streams_number = 2 # Number of parallel bitstreams to generate
p_streams_offset = 10 # Frame offset between each parallel stream's seed
# Intra-stream operation
# Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
# Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
p_intra_op = "rotl_dynamic" 
p_dynamic_shift_bits = 6 # For dynamic ops, number of controller bits to determine shift amount
p_static_shift_amount = 1 # For static ops, fixed number of bits to shift/rotate
p_inter_op = "xor" # Binary (inter-stream) operation to combine streams
p_value_seed_offset = 78901 # Additional seed offset for the final value calculation

# # Call the FPS-R:BD function
# randVal = fpsr_bd(
#     frame=frame,
#     block_size=p_block_size,
#     streams_number=p_streams_number,
#     streams_offset=p_streams_offset,
#     intra_op=p_intra_op,
#     dynamic_shift_bits=p_dynamic_shift_bits,
#     static_shift_amount=p_static_shift_amount,
#     inter_op=p_inter_op,
#     value_seed_offset=p_value_seed_offset
# )
# # Another call to fpsr_bd for the previous frame
# randVal_previous = fpsr_bd(
#     frame=frame - 1,
#     block_size=p_block_size,
#     streams_number=p_streams_number,
#     streams_offset=p_streams_offset,
#     intra_op=p_intra_op,
#     dynamic_shift_bits=p_dynamic_shift_bits,
#     static_shift_amount=p_static_shift_amount,
#     inter_op=p_inter_op,
#     value_seed_offset=p_value_seed_offset
# )
# # Check if the value has changed
# changed = 1 if randVal != randVal_previous else 0

# print("--- Bitwise Decode (BD) Sample ---")
# print(f'randVal_previous: {randVal_previous}')
# print(f'randVal: {randVal}')
# print(f'changed: {changed}')

# end of fpsr_bd sample


# /******************************************************************************/
# /* Main function to demonstrate usage of FPS-R algorithms                     */
# /******************************************************************************/
if __name__ == "__main__":
    # [PORT UPDATE] This entire test block has been updated to use the
    # exact same parameters and loop logic as the C 'main' function
    # for 1-to-1 comparison and verification of the port.
    
    # algorithms: 0 - sm, 1 - tm, 2 - qs, 3 - bd
    algo = 3  # Change this value to 0, 1, 2, or 3 to test different algorithms
    algo_name = ["SM", "TM", "QS", "BD"]  # Names for the algorithms
    print(f"Using algorithm FPS-R: {algo_name[algo]}")

    start_frames = [90, 100, 103, 100]  # starting frames for each algorithm
    num_frames = 30  # run a loop of x frames to demonstrate changes
    
    # create main for loop to demonstrate changes
    for loop_frame in range(num_frames):
        # [PORT UPDATE] Use the same frame logic as C for ALL algorithms
        frame = loop_frame + start_frames[algo]  # starting frame for the selected algorithm
        randVal = 0.0  # variable to hold the random value output
        randVal_previous = 0.0  # variable to hold the previous frame's random value
        changed = 0  # Variable to track if the value has changed

        if algo == 0:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:SM function
            # [PORT UPDATE] Parameters now match the new C main() function
            # --------------------------------------------------------------------------
            # Parameters
            minHoldFrames = 7  # probable minimum held period
            maxHoldFrames = 9  # maximum held period before cycling
            reseedFrames = 6    # inner mod cycle timing
            offsetInner = -41   # offsets the inner frame
            offsetOuter = 23    # offsets the outer frame
            finalRandSwitch = 1 # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:SM function        
            # call to fpsr_sm for the current frame
            # [PORT UPDATE] Calling renamed function fpsr_sm_base
            randVal = float(fpsr_sm_base(
                int(frame), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            # another call to fpsr_sm for the previous frame
            randVal_previous = float(fpsr_sm_base(
                int(frame - 1), int(minHoldFrames), int(maxHoldFrames), 
                int(reseedFrames), int(offsetInner), int(offsetOuter), bool(finalRandSwitch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        elif algo == 1:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:TM function
            # [PORT UPDATE] Parameters now match the new C main() function
            # --------------------------------------------------------------------------
            # Parameters
            period_A = 8         # The first hold duration
            period_B = 5         # The second hold duration
            periodSwitch = 6      # The toggle happens every 30 frames
            offset_inner = 15      # offsets the inner (toggle) clock
            offset_outer = 0      # offsets the outer (hold) clock
            final_rand_switch = 1 # 1 to apply the final randomisation step, 0 to skip it
            
            # Call the FPS-R:TM function
            # [PORT UPDATE] Calling renamed function fpsr_tm_base
            # call to fpsr_tm for the current frame
            randVal = float(fpsr_tm_base(
                int(frame), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            # another call to fpsr_tm for the previous frame
            randVal_previous = float(fpsr_tm_base(
                int(frame - 1), int(period_A), int(period_B), 
                int(periodSwitch), int(offset_inner), int(offset_outer), bool(final_rand_switch)))
            changed = 0
            if randVal != randVal_previous:
                changed = 1  # value has changed from the previous frame
        
        # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:QS function
            # [PORT UPDATE] Parameters now match the new C main() function
            # --------------------------------------------------------------------------
            # Parameters
            baseWaveFreq = 0.012 # Base frequency for the modulation wave of stream 1
            stream2freqMult = 3.1 # Multiplier for the second stream's frequency
            quantLevelsMinMax = [4, 12] # Min, Max quantisation levels for the two streams
            streamsOffset = [0, 76] # Offset for the two streams
            quantOffsets = [10, 81] # Offset for the random quantisation selection
            streamSwitchDur = 8 # Duration for switching streams in frames
            stream1QuantDur = 10 # Duration for the first stream's quantisation switch cycle in frames
            stream2QuantDur = 13 # Duration for the second stream's quantisation switch cycle in frames
            finalRandSwitch = 1 # 1 to apply the final randomisation step, 0 to skip it
            wavetable = None # Use default baked sine unipolar table
            
            # call to fpsr_qs_base for the current frame
            randVal = float(fpsr_qs_base(
                int(frame), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), 
                bool(finalRandSwitch), wavetable))
            # another call to fpsr_qs_base for the previous frame
            randVal_previous = float(fpsr_qs_base(
                int(frame - 1), float(baseWaveFreq), float(stream2freqMult), quantLevelsMinMax, 
                streamsOffset, quantOffsets, int(streamSwitchDur), int(stream1QuantDur), int(stream2QuantDur), 
                bool(finalRandSwitch), wavetable))
            changed = 0  # Variable to track if the value has changed
            if randVal != randVal_previous:
                changed = 1  # Mark as changed if the value has changed from the previous frame

        elif algo == 3:
            # --------------------------------------------------------------------------
            # Sample code to call the FPS-R:BD function
            # [PORT UPDATE] Parameters already matched C, no change needed here.
            # --------------------------------------------------------------------------
            # Parameters
            p_block_size = 64 # Size of the macro-rhythm in frames
            p_streams_number = 2 # Number of parallel bitstreams to generate
            p_streams_offset = 10 # Frame offset between each parallel stream's seed
            # Intra-stream operation
            # Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
            # Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
            p_intra_op = "rotl_dynamic"
            p_dynamic_shift_bits = 6 # For dynamic ops, number of controller bits to determine shift amount
            p_static_shift_amount = 1 # For static ops, fixed number of bits to shift/rotate
            p_inter_op = "xor" # Binary (inter-stream) operation to combine streams
            p_value_seed_offset = 78901 # Additional seed offset for the final value calculation

            randVal = fpsr_bd_base(
                frame=frame, block_size=p_block_size, streams_number=p_streams_number,
                streams_offset=p_streams_offset, intra_op=p_intra_op,
                dynamic_shift_bits=p_dynamic_shift_bits, static_shift_amount=p_static_shift_amount,
                inter_op=p_inter_op, value_seed_offset=p_value_seed_offset)
            
            randVal_previous = fpsr_bd_base(
                frame=frame - 1, block_size=p_block_size, streams_number=p_streams_number,
                streams_offset=p_streams_offset, intra_op=p_intra_op,
                dynamic_shift_bits=p_dynamic_shift_bits, static_shift_amount=p_static_shift_amount,
                inter_op=p_inter_op, value_seed_offset=p_value_seed_offset)

            if randVal != randVal_previous: changed = 1
        
        # [PORT UPDATE] Mirror C printf formatting (%.6f) and two-step print
        # Note: C `printf` with %f prints 6 decimal places by default.
        print(f"Frame {frame}: randVal {randVal:.6f}, randVal_previous {randVal_previous:.6f}, changed {changed} ", end="")
        print("(jumped)" if changed else "")