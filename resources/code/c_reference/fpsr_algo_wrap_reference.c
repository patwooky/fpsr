// SPDX-License-Identifier: Apache 2.0 — See LICENSE for full terms
// Created by Patrick Woo, 2025.
// This file is part of the FPS-R (Frame-Persistent Stateless Randomisation) project.
// https://github.com/patwooky/fpsr

/**
 * @file fpsr_wrapped.c
 * @brief This file demonstrates a wrapper-based approach for getting rich metadata
 * from the core FPS-R algorithms.
 * @details This implementation now contains the pure, stateless algorithms. The wrapper
 * functions perform a robust, two-phase search (exponential probe + binary search) to
 * populate the FPSR_Output struct. This method is highly efficient and avoids
 * "false positive" value collisions.
 *
 * This version includes the "Hierarchical Phrased Quantisation" (HPQ) wrapper logic,
 * which implements a "stretch-and-generate" model for time scaling (frame_multiplier).
 */

#include <math.h> // For sin(), floor(), ceil(), log2(), fabs()
#include <stdio.h> // For NULL, printf
#include <stdint.h> // For deterministic 64-bit integer types (int64_t, uint64_t)
#include <stdlib.h> // For malloc(), free(), size_t
#include <string.h> // For strcmp(), memset(), memcpy()

#if defined(_MSC_VER)
#include <malloc.h> // For _alloca
#define alloca _alloca
#elif defined(__GNUC__) || defined(__clang__)
#include <alloca.h> // For alloca
#endif

// Bit-width used for chunked bit operations.
// It must remain 64 for deterministic compatibility with SplitMix64. DO NOT MODIFY.
#define CHUNK_BITS 64
// Safety limit for stack allocation in fpsr_bd_base to prevent stack overflow.
#define BD_MAX_STACK_BLOCK_SIZE 8192

// Global constant for 2*PI (double precision)
#define TWO_PI 6.28318530718

// C Sine Lookup Table:
// Auto-generated 1024-point sine lookup table
// Maps normalized phase [0.0, 1.0) to [0.0, 1.0] (unipolar)
static const double fpsr_sine_lut_1024[1024] = {
    0.5000000000000000, 0.5030679423245772, 0.5061357691428600, 0.5092033649529024, 0.5122706142614561,
    0.5153374015883183, 0.5184036114706794, 0.5214691284674704, 0.5245338371637090, 0.5275976221748450,
    0.5306603681511043, 0.5337219597818320, 0.5367822817998337, 0.5398412189857150, 0.5428986561722200,
    0.5459544782485664, 0.5490085701647803, 0.5520608169360273, 0.5551111036469415, 0.5581593154559523,
    0.5612053375996081, 0.5642490553968966, 0.5672903542535631, 0.5703291196664246, 0.5733652372276808,
    0.5763985926292217, 0.5794290716669307, 0.5824565602449849, 0.5854809443801506, 0.5885021102060743,
    0.5915199439775705, 0.5945343320749031, 0.5975451610080641, 0.6005523174210460, 0.6035556880961093,
    0.6065551599580457, 0.6095506200784349, 0.6125419556798964, 0.6155290541403355, 0.6185118029971836,
    0.6214900899516319, 0.6244638028728601, 0.6274328298022573, 0.6303970589576378, 0.6333563787374492,
    0.6363106777249745, 0.6392598446925265, 0.6422037686056359, 0.6451423386272311, 0.6480754441218119,
    0.6510029746596140, 0.6539248200207675, 0.6568408701994457, 0.6597510154080078, 0.6626551460811314,
    0.6655531528799382, 0.6684449266961100, 0.6713303586559972, 0.6742093401247173, 0.6770817627102452,
    0.6799475182674941, 0.6828064989023870, 0.6856585969759188, 0.6885037051082091, 0.6913417161825449,
    0.6941725233494132, 0.6969960200305241, 0.6998120999228234, 0.7026206570024949, 0.7054215855289520,
    0.7082147800488185, 0.7110001353998998, 0.7137775467151410, 0.7165469094265760, 0.7193081192692639,
    0.7220610722852145, 0.7248056648273032, 0.7275417935631719, 0.7302693554791200, 0.7329882478839831,
    0.7356983684129988, 0.7383996150316611, 0.7410918860395613, 0.7437750800742180, 0.7464490961148920,
    0.7491138334863909, 0.7517691918628588, 0.7544150712715535, 0.7570513720966108, 0.7596779950827948,
    0.7622948413392345, 0.7649018123431472, 0.7674988099435486, 0.7700857363649465, 0.7726624942110232,
    0.7752289864683024, 0.7777851165098011, 0.7803307880986681, 0.7828659053918066, 0.7853903729434837,
    0.7879040957089227, 0.7904069790478823, 0.7928989287282194, 0.7953798509294371, 0.7978496522462166,
    0.8003082396919345, 0.8027555207021628, 0.8051914031381547, 0.8076157952903134, 0.8100286058816446,
    0.8124297440711932, 0.8148191194574634, 0.8171966420818227, 0.8195622224318879, 0.8219157714448957,
    0.8242572005110562, 0.8265864214768883, 0.8289033466485394, 0.8312078887950859, 0.8334999611518188,
    0.8357794774235092, 0.8380463517876580, 0.8403004988977265, 0.8425418338863502, 0.8447702723685334,
    0.8469857304448269, 0.8491881247044863, 0.8513773722286126, 0.8535533905932737, 0.8557160978726082,
    0.8578654126419093, 0.8600012539806908, 0.8621235414757334, 0.8642321952241125, 0.8663271358362064,
    0.8684082844386849, 0.8704755626774796, 0.8725288927207330, 0.8745681972617296, 0.8765933995218063,
    0.8786044232532423, 0.8806011927421309, 0.8825836328112295, 0.8845516688227898, 0.8865052266813684,
    0.8884442328366162, 0.8903686142860472, 0.8922782985777876, 0.8941732138133032, 0.8960532886501061,
    0.8979184523044417, 0.8997686345539525, 0.9016037657403224, 0.9034237767718996, 0.9052285991262974,
    0.9070181648529742, 0.9087924065757919, 0.9105512574955523, 0.9122946513925126, 0.9140225226288778,
    0.9157348061512727, 0.9174314374931900, 0.9191123527774190, 0.9207774887184492, 0.9224267826248536,
    0.9240601724016486, 0.9256775965526326, 0.9272789941827002, 0.9288643050001361, 0.9304334693188836,
    0.9319864280607933, 0.9335231227578463, 0.9350434955543556, 0.9365474892091450, 0.9380350470977032,
    0.9395061132143168, 0.9409606321741775, 0.9423985492154690, 0.9438198102014270, 0.9452243616223790,
    0.9466121505977576, 0.9479831248780926, 0.9493372328469769, 0.9506744235230110, 0.9519946465617217,
    0.9532978522574577, 0.9545839915452612, 0.9558530160027150, 0.9571048778517653, 0.9583395299605213,
    0.9595569258450289, 0.9607570196710209, 0.9619397662556434, 0.9631051210691557, 0.9642530402366079,
    0.9653834805394919, 0.9664963994173694, 0.9675917549694737, 0.9686695059562875, 0.9697296118010950,
    0.9707720325915103, 0.9717967290809801, 0.9728036626902606, 0.9737927955088705, 0.9747640902965183,
    0.9757175104845042, 0.9766530201770969, 0.9775705841528853, 0.9784701678661045, 0.9793517374479358,
    0.9802152597077829, 0.9810607021345208, 0.9818880328977200, 0.9826972208488447, 0.9834882355224260,
    0.9842610471372086, 0.9850156265972720, 0.9857519454931258, 0.9864699761027800, 0.9871696913927879,
    0.9878510650192642, 0.9885140713288771, 0.9891586853598138, 0.9897848828427203, 0.9903926402016152,
    0.9909819345547777, 0.9915527437156082, 0.9921050461934645, 0.9926388211944706, 0.9931540486222994,
    0.9936507090789293, 0.9941287838653747, 0.9945882549823906, 0.9950291051311486, 0.9954513177138899,
    0.9958548768345498, 0.9962397672993550, 0.9966059746173972, 0.9969534850011781, 0.9972822853671277,
    0.9975923633360984, 0.9978837072338299, 0.9981563060913889, 0.9984101496455828, 0.9986452283393451,
    0.9988615333220958, 0.9990590564500745, 0.9992377902866474, 0.9993977281025862, 0.9995388638763227,
    0.9996611922941747, 0.9997647087505466, 0.9998494093481021, 0.9999152908979116, 0.9999623509195723,
    0.9999905876413006, 1.0000000000000000, 0.9999905876413006, 0.9999623509195723, 0.9999152908979116,
    0.9998494093481021, 0.9997647087505466, 0.9996611922941747, 0.9995388638763227, 0.9993977281025862,
    0.9992377902866474, 0.9990590564500745, 0.9988615333220958, 0.9986452283393451, 0.9984101496455828,
    0.9981563060913889, 0.9978837072338299, 0.9975923633360985, 0.9972822853671277, 0.9969534850011781,
    0.9966059746173972, 0.9962397672993550, 0.9958548768345498, 0.9954513177138899, 0.9950291051311486,
    0.9945882549823906, 0.9941287838653747, 0.9936507090789293, 0.9931540486222994, 0.9926388211944706,
    0.9921050461934645, 0.9915527437156082, 0.9909819345547777, 0.9903926402016152, 0.9897848828427203,
    0.9891586853598138, 0.9885140713288771, 0.9878510650192642, 0.9871696913927879, 0.9864699761027801,
    0.9857519454931258, 0.9850156265972720, 0.9842610471372086, 0.9834882355224260, 0.9826972208488447,
    0.9818880328977200, 0.9810607021345208, 0.9802152597077829, 0.9793517374479358, 0.9784701678661045,
    0.9775705841528853, 0.9766530201770969, 0.9757175104845042, 0.9747640902965183, 0.9737927955088705,
    0.9728036626902608, 0.9717967290809801, 0.9707720325915103, 0.9697296118010950, 0.9686695059562875,
    0.9675917549694738, 0.9664963994173694, 0.9653834805394919, 0.9642530402366079, 0.9631051210691557,
    0.9619397662556434, 0.9607570196710210, 0.9595569258450289, 0.9583395299605213, 0.9571048778517653,
    0.9558530160027150, 0.9545839915452612, 0.9532978522574577, 0.9519946465617217, 0.9506744235230110,
    0.9493372328469769, 0.9479831248780926, 0.9466121505977576, 0.9452243616223790, 0.9438198102014270,
    0.9423985492154690, 0.9409606321741775, 0.9395061132143168, 0.9380350470977032, 0.9365474892091451,
    0.9350434955543557, 0.9335231227578464, 0.9319864280607935, 0.9304334693188836, 0.9288643050001361,
    0.9272789941827002, 0.9256775965526326, 0.9240601724016486, 0.9224267826248536, 0.9207774887184492,
    0.9191123527774191, 0.9174314374931900, 0.9157348061512727, 0.9140225226288778, 0.9122946513925125,
    0.9105512574955523, 0.9087924065757919, 0.9070181648529743, 0.9052285991262974, 0.9034237767718998,
    0.9016037657403224, 0.8997686345539526, 0.8979184523044418, 0.8960532886501061, 0.8941732138133032,
    0.8922782985777875, 0.8903686142860473, 0.8884442328366162, 0.8865052266813686, 0.8845516688227898,
    0.8825836328112295, 0.8806011927421309, 0.8786044232532424, 0.8765933995218063, 0.8745681972617296,
    0.8725288927207331, 0.8704755626774795, 0.8684082844386850, 0.8663271358362064, 0.8642321952241127,
    0.8621235414757334, 0.8600012539806909, 0.8578654126419094, 0.8557160978726084, 0.8535533905932737,
    0.8513773722286126, 0.8491881247044865, 0.8469857304448269, 0.8447702723685335, 0.8425418338863502,
    0.8403004988977266, 0.8380463517876580, 0.8357794774235092, 0.8334999611518188, 0.8312078887950860,
    0.8289033466485394, 0.8265864214768883, 0.8242572005110562, 0.8219157714448957, 0.8195622224318879,
    0.8171966420818227, 0.8148191194574637, 0.8124297440711932, 0.8100286058816447, 0.8076157952903135,
    0.8051914031381548, 0.8027555207021628, 0.8003082396919344, 0.7978496522462167, 0.7953798509294371,
    0.7928989287282195, 0.7904069790478823, 0.7879040957089227, 0.7853903729434837, 0.7828659053918068,
    0.7803307880986681, 0.7777851165098011, 0.7752289864683024, 0.7726624942110232, 0.7700857363649465,
    0.7674988099435486, 0.7649018123431475, 0.7622948413392345, 0.7596779950827949, 0.7570513720966109,
    0.7544150712715536, 0.7517691918628588, 0.7491138334863909, 0.7464490961148921, 0.7437750800742180,
    0.7410918860395614, 0.7383996150316611, 0.7356983684129990, 0.7329882478839831, 0.7302693554791201,
    0.7275417935631719, 0.7248056648273035, 0.7220610722852147, 0.7193081192692637, 0.7165469094265761,
    0.7137775467151410, 0.7110001353998999, 0.7082147800488185, 0.7054215855289521, 0.7026206570024950,
    0.6998120999228236, 0.6969960200305241, 0.6941725233494133, 0.6913417161825449, 0.6885037051082090,
    0.6856585969759188, 0.6828064989023869, 0.6799475182674941, 0.6770817627102452, 0.6742093401247173,
    0.6713303586559972, 0.6684449266961101, 0.6655531528799382, 0.6626551460811316, 0.6597510154080080,
    0.6568408701994457, 0.6539248200207675, 0.6510029746596140, 0.6480754441218119, 0.6451423386272312,
    0.6422037686056361, 0.6392598446925266, 0.6363106777249746, 0.6333563787374492, 0.6303970589576380,
    0.6274328298022573, 0.6244638028728601, 0.6214900899516320, 0.6185118029971836, 0.6155290541403357,
    0.6125419556798964, 0.6095506200784351, 0.6065551599580457, 0.6035556880961094, 0.6005523174210460,
    0.5975451610080643, 0.5945343320749031, 0.5915199439775705, 0.5885021102060745, 0.5854809443801506,
    0.5824565602449850, 0.5794290716669307, 0.5763985926292219, 0.5733652372276810, 0.5703291196664247,
    0.5672903542535631, 0.5642490553968965, 0.5612053375996082, 0.5581593154559523, 0.5551111036469416,
    0.5520608169360273, 0.5490085701647804, 0.5459544782485664, 0.5428986561722201, 0.5398412189857151,
    0.5367822817998339, 0.5337219597818321, 0.5306603681511043, 0.5275976221748451, 0.5245338371637089,
    0.5214691284674705, 0.5184036114706794, 0.5153374015883184, 0.5122706142614561, 0.5092033649529025,
    0.5061357691428600, 0.5030679423245774, 0.5000000000000001, 0.4969320576754227, 0.4938642308571401,
    0.4907966350470976, 0.4877293857385440, 0.4846625984116817, 0.4815963885293207, 0.4785308715325296,
    0.4754661628362911, 0.4724023778251551, 0.4693396318488959, 0.4662780402181680, 0.4632177182001663,
    0.4601587810142850, 0.4571013438277801, 0.4540455217514338, 0.4509914298352197, 0.4479391830639728,
    0.4448888963530585, 0.4418406845440478, 0.4387946624003919, 0.4357509446031036, 0.4327096457464370,
    0.4296708803335754, 0.4266347627723192, 0.4236014073707783, 0.4205709283330694, 0.4175434397550151,
    0.4145190556198495, 0.4114978897939257, 0.4084800560224296, 0.4054656679250970, 0.4024548389919358,
    0.3994476825789541, 0.3964443119038907, 0.3934448400419544, 0.3904493799215651, 0.3874580443201037,
    0.3844709458596645, 0.3814881970028166, 0.3785099100483681, 0.3755361971271400, 0.3725671701977428,
    0.3696029410423622, 0.3666436212625509, 0.3636893222750255, 0.3607401553074736, 0.3577962313943641,
    0.3548576613727690, 0.3519245558781881, 0.3489970253403861, 0.3460751799792326, 0.3431591298005544,
    0.3402489845919922, 0.3373448539188685, 0.3344468471200619, 0.3315550733038899, 0.3286696413440029,
    0.3257906598752827, 0.3229182372897549, 0.3200524817325059, 0.3171935010976132, 0.3143414030240813,
    0.3114962948917910, 0.3086582838174552, 0.3058274766505868, 0.3030039799694760, 0.3001879000771766,
    0.2973793429975051, 0.2945784144710480, 0.2917852199511816, 0.2889998646001002, 0.2862224532848591,
    0.2834530905734240, 0.2806918807307364, 0.2779389277147855, 0.2751943351726966, 0.2724582064368282,
    0.2697306445208800, 0.2670117521160170, 0.2643016315870012, 0.2616003849683390, 0.2589081139604387,
    0.2562249199257822, 0.2535509038851080, 0.2508861665136092, 0.2482308081371413, 0.2455849287284464,
    0.2429486279033892, 0.2403220049172052, 0.2377051586607656, 0.2350981876568527, 0.2325011900564515,
    0.2299142636350536, 0.2273375057889769, 0.2247710135316976, 0.2222148834901990, 0.2196692119013320,
    0.2171340946081934, 0.2146096270565164, 0.2120959042910773, 0.2095930209521178, 0.2071010712717806,
    0.2046201490705630, 0.2021503477537834, 0.1996917603080657, 0.1972444792978373, 0.1948085968618453,
    0.1923842047096866, 0.1899713941183554, 0.1875702559288069, 0.1851808805425365, 0.1828033579181774,
    0.1804377775681121, 0.1780842285551044, 0.1757427994889438, 0.1734135785231117, 0.1710966533514607,
    0.1687921112049141, 0.1665000388481813, 0.1642205225764908, 0.1619536482123421, 0.1596995011022735,
    0.1574581661136499, 0.1552297276314666, 0.1530142695551731, 0.1508118752955136, 0.1486226277713875,
    0.1464466094067263, 0.1442839021273918, 0.1421345873580908, 0.1399987460193092, 0.1378764585242666,
    0.1357678047758874, 0.1336728641637936, 0.1315917155613151, 0.1295244373225206, 0.1274711072792671,
    0.1254318027382705, 0.1234066004781938, 0.1213955767467579, 0.1193988072578690, 0.1174163671887705,
    0.1154483311772103, 0.1134947733186317, 0.1115557671633837, 0.1096313857139528, 0.1077217014222125,
    0.1058267861866971, 0.1039467113498938, 0.1020815476955583, 0.1002313654460476, 0.0983962342596775,
    0.0965762232281004, 0.0947714008737027, 0.0929818351470260, 0.0912075934242081, 0.0894487425044477,
    0.0877053486074875, 0.0859774773711223, 0.0842651938487274, 0.0825685625068101, 0.0808876472225811,
    0.0792225112815507, 0.0775732173751464, 0.0759398275983514, 0.0743224034473676, 0.0727210058172997,
    0.0711356949998640, 0.0695665306811165, 0.0680135719392068, 0.0664768772421537, 0.0649565044456443,
    0.0634525107908551, 0.0619649529022966, 0.0604938867856833, 0.0590393678258225, 0.0576014507845312,
    0.0561801897985730, 0.0547756383776211, 0.0533878494022424, 0.0520168751219076, 0.0506627671530231,
    0.0493255764769890, 0.0480053534382784, 0.0467021477425423, 0.0454160084547388, 0.0441469839972851,
    0.0428951221482348, 0.0416604700394786, 0.0404430741549712, 0.0392429803289791, 0.0380602337443567,
    0.0368948789308443, 0.0357469597633923, 0.0346165194605082, 0.0335036005826305, 0.0324082450305262,
    0.0313304940437126, 0.0302703881989052, 0.0292279674084896, 0.0282032709190199, 0.0271963373097394,
    0.0262072044911294, 0.0252359097034817, 0.0242824895154958, 0.0233469798229032, 0.0224294158471146,
    0.0215298321338956, 0.0206482625520643, 0.0197847402922172, 0.0189392978654792, 0.0181119671022801,
    0.0173027791511554, 0.0165117644775739, 0.0157389528627914, 0.0149843734027280, 0.0142480545068742,
    0.0135300238972199, 0.0128303086072121, 0.0121489349807358, 0.0114859286711229, 0.0108413146401862,
    0.0102151171572797, 0.0096073597983848, 0.0090180654452223, 0.0084472562843919, 0.0078949538065355,
    0.0073611788055294, 0.0068459513777007, 0.0063492909210708, 0.0058712161346253, 0.0054117450176095,
    0.0049708948688514, 0.0045486822861100, 0.0041451231654502, 0.0037602327006450, 0.0033940253826027,
    0.0030465149988220, 0.0027177146328723, 0.0024076366639015, 0.0021162927661701, 0.0018436939086110,
    0.0015898503544172, 0.0013547716606549, 0.0011384666779042, 0.0009409435499254, 0.0007622097133526,
    0.0006022718974138, 0.0004611361236773, 0.0003388077058253, 0.0002352912494534, 0.0001505906518979,
    0.0000847091020883, 0.0000376490804277, 0.0000094123586994, 0.0000000000000000, 0.0000094123586994,
    0.0000376490804277, 0.0000847091020883, 0.0001505906518979, 0.0002352912494534, 0.0003388077058252,
    0.0004611361236773, 0.0006022718974138, 0.0007622097133526, 0.0009409435499254, 0.0011384666779042,
    0.0013547716606549, 0.0015898503544172, 0.0018436939086110, 0.0021162927661701, 0.0024076366639015,
    0.0027177146328723, 0.0030465149988220, 0.0033940253826027, 0.0037602327006450, 0.0041451231654502,
    0.0045486822861100, 0.0049708948688514, 0.0054117450176095, 0.0058712161346253, 0.0063492909210708,
    0.0068459513777006, 0.0073611788055294, 0.0078949538065354, 0.0084472562843918, 0.0090180654452223,
    0.0096073597983848, 0.0102151171572797, 0.0108413146401861, 0.0114859286711229, 0.0121489349807357,
    0.0128303086072120, 0.0135300238972199, 0.0142480545068741, 0.0149843734027280, 0.0157389528627913,
    0.0165117644775739, 0.0173027791511553, 0.0181119671022800, 0.0189392978654792, 0.0197847402922171,
    0.0206482625520642, 0.0215298321338955, 0.0224294158471146, 0.0233469798229031, 0.0242824895154958,
    0.0252359097034816, 0.0262072044911293, 0.0271963373097394, 0.0282032709190198, 0.0292279674084895,
    0.0302703881989051, 0.0313304940437125, 0.0324082450305261, 0.0335036005826305, 0.0346165194605081,
    0.0357469597633922, 0.0368948789308443, 0.0380602337443567, 0.0392429803289790, 0.0404430741549711,
    0.0416604700394786, 0.0428951221482347, 0.0441469839972851, 0.0454160084547388, 0.0467021477425422,
    0.0480053534382783, 0.0493255764769889, 0.0506627671530230, 0.0520168751219075, 0.0533878494022423,
    0.0547756383776210, 0.0561801897985729, 0.0576014507845312, 0.0590393678258225, 0.0604938867856832,
    0.0619649529022965, 0.0634525107908550, 0.0649565044456443, 0.0664768772421536, 0.0680135719392067,
    0.0695665306811163, 0.0711356949998639, 0.0727210058172996, 0.0743224034473675, 0.0759398275983513,
    0.0775732173751464, 0.0792225112815506, 0.0808876472225810, 0.0825685625068099, 0.0842651938487273,
    0.0859774773711222, 0.0877053486074874, 0.0894487425044476, 0.0912075934242080, 0.0929818351470258,
    0.0947714008737026, 0.0965762232281003, 0.0983962342596774, 0.1002313654460475, 0.1020815476955582,
    0.1039467113498937, 0.1058267861866969, 0.1077217014222124, 0.1096313857139526, 0.1115557671633836,
    0.1134947733186316, 0.1154483311772102, 0.1174163671887704, 0.1193988072578689, 0.1213955767467577,
    0.1234066004781937, 0.1254318027382702, 0.1274711072792671, 0.1295244373225204, 0.1315917155613149,
    0.1336728641637934, 0.1357678047758875, 0.1378764585242665, 0.1399987460193091, 0.1421345873580905,
    0.1442839021273918, 0.1464466094067262, 0.1486226277713872, 0.1508118752955137, 0.1530142695551730,
    0.1552297276314664, 0.1574581661136496, 0.1596995011022735, 0.1619536482123420, 0.1642205225764907,
    0.1665000388481810, 0.1687921112049141, 0.1710966533514606, 0.1734135785231115, 0.1757427994889438,
    0.1780842285551043, 0.1804377775681120, 0.1828033579181770, 0.1851808805425365, 0.1875702559288068,
    0.1899713941183553, 0.1923842047096863, 0.1948085968618453, 0.1972444792978372, 0.1996917603080653,
    0.2021503477537834, 0.2046201490705629, 0.2071010712717805, 0.2095930209521175, 0.2120959042910774,
    0.2146096270565163, 0.2171340946081932, 0.2196692119013317, 0.2222148834901989, 0.2247710135316975,
    0.2273375057889766, 0.2299142636350536, 0.2325011900564514, 0.2350981876568525, 0.2377051586607653,
    0.2403220049172052, 0.2429486279033891, 0.2455849287284463, 0.2482308081371409, 0.2508861665136091,
    0.2535509038851079, 0.2562249199257818, 0.2589081139604387, 0.2616003849683389, 0.2643016315870010,
    0.2670117521160167, 0.2697306445208800, 0.2724582064368280, 0.2751943351726965, 0.2779389277147851,
    0.2806918807307361, 0.2834530905734239, 0.2862224532848587, 0.2889998646001002, 0.2917852199511813,
    0.2945784144710479, 0.2973793429975048, 0.3001879000771766, 0.3030039799694759, 0.3058274766505866,
    0.3086582838174548, 0.3114962948917909, 0.3143414030240811, 0.3171935010976128, 0.3200524817325060,
    0.3229182372897548, 0.3257906598752826, 0.3286696413440026, 0.3315550733038900, 0.3344468471200617,
    0.3373448539188683, 0.3402489845919923, 0.3431591298005542, 0.3460751799792324, 0.3489970253403857,
    0.3519245558781882, 0.3548576613727688, 0.3577962313943639, 0.3607401553074732, 0.3636893222750255,
    0.3666436212625507, 0.3696029410423620, 0.3725671701977428, 0.3755361971271399, 0.3785099100483679,
    0.3814881970028161, 0.3844709458596645, 0.3874580443201035, 0.3904493799215649, 0.3934448400419540,
    0.3964443119038907, 0.3994476825789539, 0.4024548389919356, 0.4054656679250970, 0.4084800560224295,
    0.4114978897939255, 0.4145190556198491, 0.4175434397550151, 0.4205709283330692, 0.4236014073707781,
    0.4266347627723188, 0.4296708803335754, 0.4327096457464368, 0.4357509446031032, 0.4387946624003920,
    0.4418406845440476, 0.4448888963530583, 0.4479391830639725, 0.4509914298352197, 0.4540455217514335,
    0.4571013438277798, 0.4601587810142846, 0.4632177182001663, 0.4662780402181679, 0.4693396318488955,
    0.4724023778251551, 0.4754661628362910, 0.4785308715325294, 0.4815963885293203, 0.4846625984116817,
    0.4877293857385438, 0.4907966350470974, 0.4938642308571397, 0.4969320576754228
};

/******************************************************************************/
/* Wavetable Type & Sampler                                                   */
/******************************************************************************/

typedef struct {
    const double* data; // Pointer to normalized [0.0, 1.0] unipolar cycle data
    int size;           // Number of elements (MUST be power-of-2)
    int mask;           // Precomputed size - 1 for bitwise modulo
} FPSR_Wavetable;

// Canonical 1024-point Sine Wavetable
static const FPSR_Wavetable FPSR_DEFAULT_SINE_WAVETABLE = {
    fpsr_sine_lut_1024,
    1024,
    1023
};

// ============================================================================
// CUSTOM WAVETABLE EXTENSION (Developer Drop-in Zone)
// Set FPSR_CUSTOM_LUT_SIZE to any power-of-2 (e.g. 256, 512, 1024, 2048, 4096)
// Map 1 full normalized cycle [0.0, 1.0) into the range [0.0, 1.0].
// ============================================================================
#define FPSR_CUSTOM_LUT_SIZE 1024

static const double fpsr_custom_lut[FPSR_CUSTOM_LUT_SIZE] = {
    /* Paste custom waveform points here to act as your algorithmic key */
    0.5
};

static const FPSR_Wavetable FPSR_CUSTOM_WAVETABLE = {
    fpsr_custom_lut,
    FPSR_CUSTOM_LUT_SIZE,
    FPSR_CUSTOM_LUT_SIZE - 1
};

/**
 * @brief Interpolates a value from any power-of-2 normalized wavetable [0.0, 1.0).
 * @param phase Input phase in normalized cycles [0.0, 1.0) (unbounded, supports negatives).
 * @param wt Pointer to the FPSR_Wavetable descriptor.
 * @return Interpolated value from the wavetable.
 */
static inline double fpsr_sample_wavetable(double phase, const FPSR_Wavetable* wt) {
    // Normalize phase into [0.0, 1.0) range using floor
    double norm_phase = phase - floor(phase);

    // Map to LUT space
    double scaled = norm_phase * (double)wt->size;
    int64_t idx1 = (int64_t)scaled;
    double frac = scaled - (double)idx1;

    // Fast power-of-2 wrap using precomputed bitmask
    int i1 = (int)(idx1 & wt->mask);
    int i2 = (int)((idx1 + 1) & wt->mask);

    return wt->data[i1] * (1.0 - frac) + wt->data[i2] * frac;
}

/******************************************************************************/
/* Core Components (Deterministic PRNG and Integer Math)                      */
/******************************************************************************/
/**
 * Deterministic integer math helpers and PRNG
 *
 * Rationale for determinism across C and Python:
 * - Python's % and // are floor-based for negatives; C's % and / truncate toward zero.
 * Using floor-mod alignment here ensures identical behavior for negative frames/seeds.
 * - All integer counters, frames, durations, and seeds are int64_t for large-range support.
 * - All fractional math that converts to/from integers uses double to match Python's float.
 * - The PRNG uses a uint64_t mixer (SplitMix64) with well-defined wraparound, then maps the
 * top 53 bits to a double in [0,1). This yields bit-for-bit identical results across
 * compilers and mirrors a standard reference implementation in Python.
 */

// Floor-based modulo that matches Python's semantics for negative inputs.
// C's a % m truncates toward zero; Python's a % m is always in [0, m-1] for m>0.
// By normalizing remainders this way, all alignments and toggles match Python exactly.
static inline int64_t i64_floor_mod(int64_t a, int64_t m) {
    // assume m > 0
    int64_t r = a % m;
    if (r < 0) r += m;
    return r;
}

// Align-down to the nearest multiple of m using floor-mod semantics.
// This mirrors Python's a - (a % m) even when a is negative, guaranteeing parity
// between C and Python for all frame-alignment logic.
static inline int64_t i64_align_down(int64_t a, int64_t m) {
    return a - i64_floor_mod(a, m);
}

// SplitMix64: simple, robust 64-bit mixer using well-defined uint64_t wraparound.
// Produces identical bit patterns across platforms/compilers. Suitable for hashing
// integer seeds into pseudo-random 64-bit values.
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    x ^= (x >> 31);
    return x;
}

// Map a uint64_t to a double in [0,1) by taking the top 53 bits (double mantissa width).
// Using the identical bit-extraction and scale factor (2^-53) in Python ensures that
// both languages produce the exact same floating value for the same 64-bit seed.
static inline double portable_rand_u64(uint64_t seed) {
    uint64_t r = splitmix64(seed);
    return (double)(r >> 11) * (1.0 / 9007199254740992.0); // 2^53
}

/**
 * A simple, portable pseudo-random number generator.
 * @brief Back-compat wrapper: generates a deterministic float in [0, 1] from an integer seed.
 * @details This now forwards to the 64-bit deterministic PRNG above to guarantee parity with Python.
 * @param seed An integer used to generate the random number.
 * @return A pseudo-random float between 0.0 and 1.0.
 */
static inline float portable_rand(int seed) {
    // Converts the given seed to a 64-bit unsigned integer, generates a pseudo-random number using portable_rand_u64,
    // and casts the result to a float for return.
    // Keep seeding strictly in integer domain; cast via int64_t to preserve sign.
    return (float)portable_rand_u64((uint64_t)(int64_t)seed);
}

// --- Bitwise Rotation Helpers ---
// Performs a circular (rotate) left shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_left_shift(uint64_t value, int shift) {
    // The modulo operator ensures the shift amount is always within [0, CHUNK_BITS-1],
    // preventing undefined behavior from shifts >= the type's bit-width.
    int s = shift % CHUNK_BITS;
    if (s == 0) return value;
    // The C standard guarantees that for unsigned types, right-shift is a logical shift
    // (fills with zeros), which is the correct behavior for rotation.
    return (value << s) | (value >> (CHUNK_BITS - s));
}

// Performs a circular (rotate) right shift on a 64-bit unsigned integer.
static inline uint64_t u64_circular_right_shift(uint64_t value, int shift) {
    // The modulo operator ensures the shift amount is always within [0, CHUNK_BITS-1],
    // preventing undefined behavior.
    int s = shift % CHUNK_BITS;
    if (s == 0) return value;
    // The right-shift on the unsigned 'value' is a well-defined logical shift.
    return (value >> s) | (value << (CHUNK_BITS - s));
}

// Helper to get a specific bit from a chunk array, matching Python's out-of-bounds logic
static int get_bit(int64_t n, int64_t block_size, const uint64_t* chunks, int64_t num_chunks) {
    if (n < 0 || n >= block_size) return 0;
    // Bit ordering: chunk_index progresses block-wise, bit_index is LSB-first within a chunk.
    int64_t chunk_index = n / CHUNK_BITS;
    int bit_index = n % CHUNK_BITS;
    if (chunk_index >= num_chunks) return 0; // Should not happen with correct logic but safe
    return (chunks[chunk_index] >> bit_index) & 1;
}

/******************************************************************************/
/* FPS-R Output Structure                                                     */
/******************************************************************************/
/* * This structure holds the output of the FPS-R algorithms.
* The LOD (Level of Detail) determines the computational overhead and the amount of information returned.
*
* Different LODs will return different sets of fields:
* - LOD 0: randVal
* - LOD 1: randVal, has_changed
* - LOD 2: randVal, has_changed, hold_progress, last_changed_frame, next_changed_frame,
* randVal_next_changed_frame, randStreams[2], selected_stream_idx (for QS algorithm)
* Note: All fields will be set to 0 if the LOD is not applicable.
*
* The fields are:
* double randVal: LOD 0, 1, 2. The random value generated by the FPS-R algorithm.
* int has_changed: LOD 1, 2. A flag indicating whether randVal has changed from the previous frame.
* double randVal_previous: LOD 1, 2. The random value from the previous frame for change detection.
* double hold_progress: LOD 2. The progress of the hold duration, normalised to [0, 1].
* int last_changed_frame: LOD 2. The precise frame (integer) when the random value last changed.
* int next_changed_frame: LOD 2. The precise frame (integer) when the random value will next change.
* double randVal_next_changed_frame: LOD 2. The value that the algorithm will jump to at next_changed_frame.
* double randStreams[2]: LOD 2. (Exclusive to QS) The raw values of stream1_double and stream2_double.
* int selected_stream_idx: LOD 2. (Exclusive to QS) The index of the stream (0 for stream1, 1 for stream2).
*/
typedef struct {
    double randVal;                     // The random value output of FPS-R algorithm. (LOD 0,1,2)
    int has_changed;                    // Flag indicating if randVal changed from previous frame. (LOD 1,2)
    double randVal_previous;            // The random value from the previous frame. (LOD 1,2)
    double hold_progress;               // Normalised progress of the hold duration [0,1]. (LOD 2)
    int last_changed_frame;             // The frame when randVal last changed. (LOD 2)
    int next_changed_frame;             // The frame when randVal will next change. (LOD 2)
    double randVal_next_changed_frame;  // The value at next_changed_frame. (LOD 2)
    double randStreams[2];              // stream1_double and stream2_double raw values (LOD 2)
    int selected_stream_idx;            // 0 for stream1, 1 for stream2 (LOD 2)
} FPSR_Output;

/******************************************************************************/
/* Pure, Canonical FPS-R Algorithms                                           */
/******************************************************************************/
// These functions are the pure, canonical reference implementations. They operate
// on a 64-bit integer timeline for absolute determinism.

//-----------------------------------------------------------------------------/
// FPS-R: Stacked Modulo (SM)                                                  /
//-----------------------------------------------------------------------------/
// The pure 'base' version of SM for the wrapper. It returns just the float value.
/*
* @brief Generates a persistent random value that holds for a calculated duration.
* @details This function uses a two-step process. First, it determines a random
* "hold duration". Second, it generates a stable integer for that duration,
* which is then used as a seed to produce the final, held random value.
*
* int frame: The current frame or time input.
* int minHold: The minimum duration (in frames) for a value to hold.
* int maxHold: The maximum duration (in frames) for a value to hold.
* int reseedInterval: The fixed interval at which a new hold duration is calculated.
* int seedInner: An offset for the random duration calculation to create unique sequences.
* int seedOuter: An offset for the final value calculation to create unique sequences.
* int finalRandSwitch: A flag that can turn off the final randomisation step.
* int lod: The level of detail (LOD) that controls computational overhead. Valid values are 0 to 2.
* * return 
* FPSR_Output struct containing the random value and other details.
* Output fields depend on the LOD level.
* Refer to the FPSR_Output structure for details on the return values.
*
* float randVal: this is the main output, which is a float value between [0.0, 1.0]
* when finalRandSwitch is 0: 
    * randVal will be a whole number representing the currently held   frame 
    * that remains constant for the hold duration.
* when finalRandSwitch is 1: 
    * A float value between 0.0 and 1.0 that remains constant 
    * for the held duration.
*/
double fpsr_sm_base(
    int64_t frame, int64_t minHold, int64_t maxHold,
    int64_t reseedInterval, int64_t seedInner, int64_t seedOuter, int finalRandSwitch)
{
    // --- 1. Calculate the random hold duration ---
    if (reseedInterval < 1) { reseedInterval = 1; } // Prevent division by zero.

    // Use floor-based modulo to match Python for negative frames.
    // Seed stays in integer domain for reproducibility.
    int64_t reseed_anchor = (seedInner + frame) - i64_floor_mod(frame, reseedInterval);
    
    // Deterministic PRNG over 64-bit integer seed; result is double in [0,1].
    double rand_for_duration = portable_rand_u64((uint64_t)reseed_anchor);
    
    // Compute duration with double intermediates then floor to int64.
    // Double math here mirrors Python's float behavior for cross-language parity.
    int64_t holdDuration = (int64_t)floor((double)minHold + rand_for_duration * (double)(maxHold - minHold));
    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // Align down using floor-mod semantics for negative inputs.
    int64_t held_integer_state = i64_align_down((seedOuter + frame), holdDuration);

    // --- 3. Use the stable state as a seed for the final random value ---
    // Keep all seed math in 64-bit integer space; rely on uint64 wraparound (well-defined).
    double fpsr_output = 0.0;
    if (finalRandSwitch) {
        // The held_integer_state is already the unique identifier for this hold segment.
        // We pass it directly to the SplitMix64 hasher without needing an additional multiplier.
        // --- FIX: Removed `* 100000ULL` to match canonical _base.c implementation ---
        uint64_t seed = (uint64_t)held_integer_state;
        fpsr_output = portable_rand_u64(seed);
    } else {
        // Return the active stream value directly as a double (cast by caller if needed).
        fpsr_output = (double)held_integer_state; 
    }
    return fpsr_output;
}

//-----------------------------------------------------------------------------/
// FPS-R: Toggled Modulo (TM)                                                  /
//-----------------------------------------------------------------------------/
// This is the pure 'base' version of TM for the wrapper. It returns just the float value.
/**
 * @brief Generates a persistent value that holds for a rhythmically toggled duration.
 * @details This function uses a deterministic switch to toggle the hold duration
 * between two fixed periods. This creates a predictable, rhythmic, or mechanical
 * "move-and-hold" pattern, as opposed to the organic randomness of SM.
 *
 * int frame: The current frame or time input.
 * int periodA: The first hold duration (in frames).
 * int periodB: The second hold duration (in frames).
 * int periodSwitch: The fixed interval at which the hold duration is toggled to switch between periodA and periodB.
 * int seedInner: An offset for the toggle clock to de-sync it from the main clock.
 * int seedOuter: An offset for the main clock to create unique sequences.
 * int finalRandSwitch: A flag that can turn off the final randomisation step.
 * return 
 * when finalRandSwitch is 0: 
 * An integer value representing the currently held frame state.
 * when finalRandSwitch is 1: 
 * A float value between 0.0 and 1.0 that holds for the toggled duration.
 */
double fpsr_tm_base(
    int64_t frame, int64_t periodA, int64_t periodB,
    int64_t periodSwitch, int64_t seedInner, int64_t seedOuter,
    int finalRandSwitch)
{
    // --- 1. Determine the hold duration by toggling between two periods ---
    if (periodSwitch < 1) { periodSwitch = 1; } // Prevent division by zero.

    
    // The "inner clock" is offset by seedInner to de-correlate it from the main frame.
    int64_t inner_clock_frame = seedInner + frame;

    // Use floor-based modulo for cross-language consistency.
    int64_t r = i64_floor_mod(inner_clock_frame, periodSwitch);

    // Toggle threshold at exactly half the period using integer math.
    // Equivalent to: (r < 0.5 * periodSwitch) without floating-point rounding.
    int64_t holdDuration = (2 * r < periodSwitch) ? periodA : periodB;
    if (holdDuration < 1) { holdDuration = 1; } // Prevent division by zero.

    // --- 2. Generate the stable integer "state" for the hold period ---
    // The "outer clock" is offset by seedOuter to create unique output sequences.
    int64_t outer_clock_frame = seedOuter + frame;
    int64_t held_integer_state = i64_align_down(outer_clock_frame, holdDuration);
    
    // --- 3. Use the stable state as a seed for the final random value (or bypass) ---
    double fpsr_output;
    if (finalRandSwitch) {
        // The held_integer_state is the unique identifier for the hold segment.
        // Pass it directly to the SplitMix64 hasher for a well-distributed random value.
        // --- FIX: Removed `* 100000ULL` to match canonical _base.c implementation ---
        uint64_t seed = (uint64_t)held_integer_state;
        fpsr_output = portable_rand_u64(seed);
    } else {
        // Return the raw integer state directly.
        fpsr_output = (double)held_integer_state; 
    }
    return fpsr_output;
}

//-----------------------------------------------------------------------------/
// FPS-R: Quantised Switching (QS)                                             /
//-----------------------------------------------------------------------------/
// This special 'base' version of QS is for the wrapper. It returns the full
// struct needed for rich output, and uses the Sine-LUT for determinism.
/**
 * @brief Generates a quantized sine-based persistent random value using two streams.
 * @param frame The current frame or time input.
 * @param baseWaveFreq The base frequency for the sine waves.
 * @param stream2FreqMult A multiplier for the second stream's frequency.
 * @param quantLevelsMinMax An array defining the minimum and maximum quantization levels.
 * @param streamsOffset An array defining the phase offsets for each sine stream.
 * @param quantOffsets An array defining the quantization seed offsets for each stream.
 * @param streamSwitchDur The duration (in frames) before switching between streams.
 * @param stream1QuantDur The quantization duration (in frames) for stream 1.
 * @param stream2QuantDur The quantization duration (in frames) for stream 2.
 * @param finalRandSwitch A flag that can turn off the final randomisation step.
 * @param wavetable Optional custom wavetable descriptor (pass NULL to use default sine).
 * @return FPSR_Output struct containing the random value and other details.
 */
FPSR_Output fpsr_qs_base(
    int64_t frame, double baseWaveFreq, double stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int64_t streamSwitchDur, int64_t stream1QuantDur, int64_t stream2QuantDur,
    int finalRandSwitch, const FPSR_Wavetable* wavetable)
{
    FPSR_Output output = {0};

    // Default fallback to canonical sine table if NULL
    const FPSR_Wavetable* wt = (wavetable != NULL) ? wavetable : &FPSR_DEFAULT_SINE_WAVETABLE;

    if (streamSwitchDur < 1) { streamSwitchDur = 1; }
    if (stream1QuantDur < 1) { stream1QuantDur = 1; }
    if (stream2QuantDur < 1) { stream2QuantDur = 1; }

    // --- 2. Calculate random quantisation levels for each stream ---
    int64_t quant_min = (int64_t)quantLevelsMinMax[0];
    int64_t quant_max = (int64_t)quantLevelsMinMax[1];
    int64_t quant_range = quant_max - quant_min + 1;

    // --- Stream 1 Quant Level ---
    int64_t s1_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[0] + frame, stream1QuantDur);
    double s1_rand_for_quant = portable_rand_u64((uint64_t)s1_quant_seed_aligned);
    int64_t s1_quant_level = quant_min + (int64_t)floor(s1_rand_for_quant * (double)quant_range);

    // --- Stream 2 Quant Level ---
    int64_t s2_quant_seed_aligned = i64_align_down((int64_t)quantOffsets[1] + frame, stream2QuantDur);
    double s2_rand_for_quant = portable_rand_u64((uint64_t)s2_quant_seed_aligned);
    int64_t s2_quant_level = quant_min + (int64_t)floor(s2_rand_for_quant * (double)quant_range);

    if (s1_quant_level < 1) { s1_quant_level = 1; }
    if (s2_quant_level < 1) { s2_quant_level = 1; }
    
    // --- 3. Generate the two quantised sine wave streams ---
    if (stream2FreqMult <= 0) { stream2FreqMult = 3.7; }
    
    // Phase expressed in cycles [0.0, 1.0)
    double phase1 = ((double)streamsOffset[0] + (double)frame) * baseWaveFreq;
    double phase2 = ((double)streamsOffset[1] + (double)frame) * baseWaveFreq * stream2FreqMult;

    double stream1_raw_sine = fpsr_sample_wavetable(phase1, wt);
    double stream2_raw_sine = fpsr_sample_wavetable(phase2, wt);    

    // Values are already unipolar [0.0, 1.0] from the wavetable
    output.randStreams[0] = floor(stream1_raw_sine * (double)s1_quant_level) / (double)s1_quant_level;
    output.randStreams[1] = floor(stream2_raw_sine * (double)s2_quant_level) / (double)s2_quant_level;
    
    // --- 4. Switch between the two streams based on streamSwitchDur ---
    int64_t r = i64_floor_mod(frame, streamSwitchDur);
    output.selected_stream_idx = (2 * r < streamSwitchDur) ? 0 : 1;
    double active_stream_val = (output.selected_stream_idx == 0) ? output.randStreams[0] : output.randStreams[1];

    // --- 5. Hash the final output or bypass ---
    if (finalRandSwitch == 1) {
        int64_t hashed_int = (int64_t)floor(active_stream_val * 1000000.0);
        output.randVal = portable_rand_u64((uint64_t)hashed_int);
    } else {
        output.randVal = active_stream_val;
    }
    return output;
}

//-----------------------------------------------------------------------------/
// FPS-R: Bitwise Decode (BD)                                                  /
//-----------------------------------------------------------------------------/
// This is the pure 'base' version of BD for the wrapper. It returns just the float value.
/**
 * @brief Generates a phrased random value by decoding a deterministically generated bitstream.
 * @details This algorithm is stateless. For any given frame, it calculates its state by:
 * 1. Finding the start of its macro-block (`outer_anchor`).
 * 2. Generating one or more raw bitstreams for the block.
 * 3. Applying transformations (intra-stream op) to each stream, possibly in pairs for dynamic ops.
 * 4. Combining the transformed streams (inter-stream op).
 * 5. Decoding the final bitstream to produce phrased holds and jumps based on bit-flips.
 *
 * int64_t frame: The current frame or time input.
 * int64_t block_size: The size of the macro-rhythm in frames. Must be > 0.
 * int streams_number: The number of parallel bitstreams to generate.
 * int64_t streams_offset: The frame offset between each parallel stream's seed.
 * const char* intra_op: The unary (intra-stream) operation.
 *      Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
 *      Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
 * int dynamic_shift_bits: For dynamic ops, the number of controller bits to read
 *      to determine the shift/rotate amount (1-6 when chunk_bits=64).
 * int static_shift_amount: For static ops, the fixed number of bits to shift/rotate.
 * const char* inter_op: The binary (inter-stream) operation to combine multiple
 *      transformed streams. Options: "xor", "or", "and".
 * int64_t value_seed_offset: An additional seed offset for the final value calculation.
 * @return A deterministic, phrased pseudo-random double between 0.0 and 1.0.
 */
double fpsr_bd_base(
    int64_t frame,
    int64_t block_size,
    int streams_number,
    int64_t streams_offset,
    const char* intra_op,
    int dynamic_shift_bits,
    int static_shift_amount,
    const char* inter_op,
    int64_t value_seed_offset
) {
    if (block_size <= 0) block_size = 1;
    if (streams_number < 1) streams_number = 1;
    
    // --- Hardening: Sanitize static_shift_amount to prevent Undefined Behavior ---
    // This ensures the shift is always within the valid range [0, 63].
    int sanitized_static_shift = static_shift_amount & (CHUNK_BITS - 1);

    // --- Step 1: Find the Outer Anchor for the macro-block ---
    int64_t outer_anchor = i64_align_down(frame, block_size);
    int64_t num_chunks = (block_size + (CHUNK_BITS - 1)) / CHUNK_BITS;

    // --- Safer Memory Allocation with comprehensive overflow checks ---
    // Calculate total memory needed for all buffers to make one contiguous allocation.
    size_t chunk_data_sz = num_chunks * sizeof(uint64_t);

    // --- FIX: Added comprehensive overflow checks ---
    // Check multiplication for total_chunk_data_sz
    if (streams_number > 0 && (SIZE_MAX / (size_t)streams_number < chunk_data_sz)) {
        fprintf(stderr, "ERROR in fpsr_bd_base: Overflow calculating total_chunk_data_sz. Returning 0.0.\n");
        return 0.0; // Overflow
    }
    size_t total_chunk_data_sz = (size_t)streams_number * chunk_data_sz;

    // Check multiplication for ptr_arrays_sz
    if ((SIZE_MAX / sizeof(uint64_t*)) / 2 < (size_t)streams_number) {
        fprintf(stderr, "ERROR in fpsr_bd_base: Overflow calculating ptr_arrays_sz. Returning 0.0.\n");
        return 0.0; // Overflow check
    }
    size_t ptr_arrays_sz = (size_t)streams_number * sizeof(uint64_t*) * 2; // raw_streams pointers + transformed_streams pointers

    // Check additions for total_alloc_size
    if (SIZE_MAX - ptr_arrays_sz < chunk_data_sz) {
        fprintf(stderr, "ERROR in fpsr_bd_base: Overflow calculating total_alloc_size (step 1). Returning 0.0.\n");
        return 0.0; // Overflow check
    }
    size_t temp_size = ptr_arrays_sz + chunk_data_sz; // Size for pointers + final_chunks
    if (SIZE_MAX - temp_size < 2 * total_chunk_data_sz) {
        fprintf(stderr, "ERROR in fpsr_bd_base: Overflow calculating total_alloc_size (step 2). Returning 0.0.\n");
        return 0.0; // Overflow check (adding space for raw_streams + transformed_streams)
    }
    size_t total_alloc_size = temp_size + (2 * total_chunk_data_sz); // final calculation

    void* buffer_base = malloc(total_alloc_size);
    if (!buffer_base) { 
        fprintf(stderr, "ERROR in fpsr_bd_base: malloc failed to allocate %zu bytes. Returning 0.0.\n", total_alloc_size);
        return 0.0; // Allocation failed, return neutral value.
    } 
    
    // Carve up the single buffer into the pointer arrays and per-stream chunk arrays.
    uint8_t* p = (uint8_t*)buffer_base;
    uint64_t** raw_streams = (uint64_t**)p;
    p += streams_number * sizeof(uint64_t*);
    uint64_t** transformed_streams = (uint64_t**)p;
    p += streams_number * sizeof(uint64_t*);
    
    for (int i = 0; i < streams_number; ++i) {
        raw_streams[i] = (uint64_t*)p;
        p += chunk_data_sz;
    }
    for (int i = 0; i < streams_number; ++i) {
        transformed_streams[i] = (uint64_t*)p;
        p += chunk_data_sz;
    }
    uint64_t* final_chunks = (uint64_t*)p;

    // --- Safety: Initialize transformed_streams to zero ---
    for (int i = 0; i < streams_number; ++i) {
        memset(transformed_streams[i], 0, chunk_data_sz);
    }
    
    // --- Step 2: Generate the raw bitstream(s) for the entire block ---
    for (int i = 0; i < streams_number; ++i) {
        // Casting negative offsets to uint64_t is well-defined and deterministic.
        int64_t stream_seed = outer_anchor + (i * streams_offset);
        for (int j = 0; j < num_chunks; ++j) {
            raw_streams[i][j] = splitmix64((uint64_t)(stream_seed + j));
        }
    }
    
    // --- Step 3: Apply Intra-Stream Transformations ---
    int is_dynamic = (strcmp(intra_op, "lshift_dynamic") == 0 || strcmp(intra_op, "rshift_dynamic") == 0 ||
                    strcmp(intra_op, "rotl_dynamic") == 0 || strcmp(intra_op, "rotr_dynamic") == 0);

    int num_transformed_streams = is_dynamic ? (streams_number / 2 + streams_number % 2) : streams_number;

    if (is_dynamic) {
        // For dynamic ops, streams are processed in pairs (data, controller).
        for (int i = 0; i < streams_number / 2; ++i) {
            int data_idx = i * 2;
            int controller_idx = i * 2 + 1;
            int target_idx = i; // Simplified index for clarity

            uint64_t* data_stream = raw_streams[data_idx];
            uint64_t* controller_stream = raw_streams[controller_idx];
            
            // max_bits_for_shift is ceil(log2(CHUNK_BITS)), which is 6 for 64.
            int max_bits_for_shift = 6;
            int bit_mask_size = dynamic_shift_bits;
            if (bit_mask_size < 1) bit_mask_size = 1;
            if (bit_mask_size > max_bits_for_shift) bit_mask_size = max_bits_for_shift;
            uint64_t bit_mask = (1ULL << bit_mask_size) - 1;

            for (int j = 0; j < num_chunks; ++j) {
                int dynamic_shift = (controller_stream[j] & bit_mask); // Shift amount is derived from controller
                if (strcmp(intra_op, "lshift_dynamic") == 0) transformed_streams[target_idx][j] = data_stream[j] << (dynamic_shift % CHUNK_BITS);
                else if (strcmp(intra_op, "rshift_dynamic") == 0) transformed_streams[target_idx][j] = data_stream[j] >> (dynamic_shift % CHUNK_BITS);
                else if (strcmp(intra_op, "rotl_dynamic") == 0) transformed_streams[target_idx][j] = u64_circular_left_shift(data_stream[j], dynamic_shift);
                else if (strcmp(intra_op, "rotr_dynamic") == 0) transformed_streams[target_idx][j] = u64_circular_right_shift(data_stream[j], dynamic_shift);
            }
        }
        // If there's an odd number of streams, the last one is unpaired and copied directly.
        if (streams_number % 2 != 0) {
            int last_raw_idx = streams_number - 1;
            int last_target_idx = num_transformed_streams - 1;
            memcpy(transformed_streams[last_target_idx], raw_streams[last_raw_idx], chunk_data_sz);
        }
    } else { // Static operations
        for (int i = 0; i < streams_number; ++i) {
            for (int j = 0; j < num_chunks; ++j) {
                if (strcmp(intra_op, "not") == 0) transformed_streams[i][j] = ~raw_streams[i][j];
                else if (strcmp(intra_op, "lshift") == 0) transformed_streams[i][j] = raw_streams[i][j] << sanitized_static_shift;
                else if (strcmp(intra_op, "rshift") == 0) transformed_streams[i][j] = raw_streams[i][j] >> sanitized_static_shift;
                else if (strcmp(intra_op, "rotl") == 0) transformed_streams[i][j] = u64_circular_left_shift(raw_streams[i][j], sanitized_static_shift);
                else if (strcmp(intra_op, "rotr") == 0) transformed_streams[i][j] = u64_circular_right_shift(raw_streams[i][j], sanitized_static_shift);
                else transformed_streams[i][j] = raw_streams[i][j]; // "none"
            }
        }
    }
    
    // --- Step 4: Combine Streams with Inter-Stream Operation ---
    if (num_transformed_streams > 0) {
        memcpy(final_chunks, transformed_streams[0], chunk_data_sz);
        for (int i = 1; i < num_transformed_streams; ++i) {
            for (int j = 0; j < num_chunks; ++j) {
                if (strcmp(inter_op, "or") == 0) final_chunks[j] |= transformed_streams[i][j];
                else if (strcmp(inter_op, "and") == 0) final_chunks[j] &= transformed_streams[i][j];
                else final_chunks[j] ^= transformed_streams[i][j]; // "xor" is default
            }
        }
    } else {
        memset(final_chunks, 0, chunk_data_sz);
    }
    
    // --- Step 5: Decode the final bitstream ---
    int64_t current_pos_in_block = frame - outer_anchor;
    int64_t last_flip_pos = 0;
    // Scan backwards from the current frame's position to find the most recent bit flip.
    // If no flip is found (constant bitstream), last_flip_pos remains 0.
    // The hold is defined by the distance to this last flip.
    for (int64_t i = current_pos_in_block; i > 0; --i) {
        if (get_bit(i, block_size, final_chunks, num_chunks) != get_bit(i - 1, block_size, final_chunks, num_chunks)) {
            last_flip_pos = i;
            break;
        }
    }
    
    // --- Step 6: Generate the final random value from the last bit-flip position ---
    // The seed is a combination of the block start, the position of the last flip,
    // and a user-provided offset, ensuring a unique value for each held segment.
    uint64_t final_seed = (uint64_t)outer_anchor + (uint64_t)last_flip_pos + (uint64_t)value_seed_offset;
    double result = portable_rand_u64(final_seed);

    // --- Cleanup for heap allocation ---
    free(buffer_base);
    
    return result;
}

//-----------------------------------------------------------------------------/
// FPS-R: Hash (H)                                                             /
//-----------------------------------------------------------------------------/
/**
 * @brief Pure coordinate-hash baseline (Stateless Continuous Noise).
 * @details
 * This base helper treats the provided 64-bit coordinate (typically the frame/time)
 * as the deterministic seed for the canonical portable spatial hash (SplitMix64 -> double).
 * It intentionally bypasses all non-linear phrasing wrappers (SM, TM) to provide 
 * continuous stochastic entropy.
 * Callers are responsible for composing any richer coordinate or seed (e.g. frame + offset,
 * spatial encoding, or packing multiple integers) into the single int64_t argument.
 * This keeps the function's responsibility narrow and consistent with the other
 * fpsr_*_base functions that accept only a single frame/coordinate argument.
 *
 * @param frame  A 64-bit coordinate (e.g. frame/time or caller-encoded coordinate+seed).
 * @return       A deterministic pseudo-random double in [0.0, 1.0).
 */
double fpsr_h_base(int64_t frame)
{
    uint64_t seed = (uint64_t)frame;
    return portable_rand_u64(seed);
}
 


/******************************************************************************/
/* High-Level Wrapper Functions with Hierarchical Time                        */
/******************************************************************************/

// --- Forward declarations are needed for recursive calls ---
FPSR_Output fpsr_sm_get_details(int64_t frame, double frame_multiplier, double* p_scaled_frame_pos_out, int minHold, int maxHold, int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch, int lod, int max_search_frames, int seg_block_length, int varispeed_hold_block_count);
FPSR_Output fpsr_tm_get_details(int64_t frame, double frame_multiplier, double* p_scaled_frame_pos_out, int periodA, int periodB, int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch, int lod, int max_search_frames, int seg_block_length, int varispeed_hold_block_count);
FPSR_Output fpsr_qs_get_details(int64_t frame, double frame_multiplier, double* p_scaled_frame_pos_out, double baseWaveFreq, double stream2FreqMult, const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2], int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch, const FPSR_Wavetable* wavetable, int lod, int max_search_frames, int seg_block_length, int varispeed_hold_block_count);
FPSR_Output fpsr_bd_get_details(int64_t frame, double frame_multiplier, double* p_scaled_frame_pos_out, int block_size, int streams_number, int streams_offset, const char* intra_op, int dynamic_shift_bits, int static_shift_amount, const char* inter_op, int value_seed_offset, int lod, int max_search_frames, int seg_block_length, int varispeed_hold_block_count);

/**
 * ---- SM: Stacked Modulo Wrapper with Details ----
 * @brief Wrapper for fpsr_sm that returns a detailed FPSR_Output struct.
 * @param frame (int64_t) The current frame or time input.
 * @param frame_multiplier (double) The time scaling factor.
 * < 1.0 = Slow-Motion (Time Stretch)
 * = 1.0 = Normal Speed
 * > 1.0 = Fast-Motion (Time Compression)
 * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
 * @param minHold (int) The minimum duration (in frames) for a value to hold.
 * @param maxHold (int) The maximum duration (in frames) for a value to hold.
 * @param reseedInterval (int) The fixed interval at which a new hold duration is calculated.
 * @param seedInner (int) An offset for the random duration calculation to create unique sequences.
 * @param seedOuter (int) An offset for the final value calculation to create unique sequences.
 * @param finalRandSwitch (bool) A flag that can turn off the final randomisation step.
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search.
 * @param seg_block_length (int) The "runway" length for HPQ logic.
 * @param varispeed_hold_block_count (int) Anchor persistence threshold:
 *        -1 = Pure Tape Varispeed (Exact 1:1 Ground-Truth Hold; Metrology mode)
 *         0 = Obfuscation / Alternate Timeline (Leaves no trace of the original
 *             values at their underlying frames; 'paints over the original painting'
 *             while preserving the macro rhythm grid)
 *       >=1 = Phrased Anchor + Infill (Anchor milestone holds for N runway blocks
 *             before generative sub-phrasing)
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_sm_get_details(
    int64_t frame, double frame_multiplier,
    double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
    int minHold, int maxHold,
    int reseedInterval, int seedInner, int seedOuter, int finalRandSwitch,
    int lod, int max_search_frames,
    int seg_block_length,
    int varispeed_hold_block_count)
{
    FPSR_Output out = {0};
    
    // *** NEW: HPQ Timeline Definitions ***
    /*
    * --- HPQ Timeline Definitions ---
    * This logic maps between two distinct timelines:
    *
    * 1. "Application Timeline":
    * - This is the `frame` parameter (e.g., 0, 1, 2, 3...).
    * - It's the "wall clock" of the user's application.
    * - All LOD 2 outputs (`last_changed_frame`, `next_changed_frame`,
    * `hold_progress`) are returned relative to this timeline.
    *
    * 2. "Content Timeline":
    * - This is the *original* algorithm's timeline (e.g., `master_frame` 0, 1, 2...).
    * - `scaled_frame_position` is the floating-point coordinate on this timeline.
    *
    * - `frame_multiplier` (fm) is the ratio that maps between them:
    * (Application Timeline Frame) * fm = (Content Timeline Frame)
    */
    
    // --- Sanitize frame_multiplier (now "playback_speed") once at the start ---
    // Use 1.0 if 0.0 is passed to avoid division by zero later.
    double fm = (frame_multiplier == 0.0) ? 1.0 : frame_multiplier;

    // --- (START) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    // --- 1. Find coordinate on "Content Timeline" ---
    // *** MODIFIED: Changed from division to multiplication ***
    // This calculation now matches the intuitive "playback_speed" convention.
    // e.g., fm = 0.5 (slow-mo): App frame 1 -> Content frame 0.5
    // e.g., fm = 2.0 (fast-mo): App frame 1 -> Content frame 2.0
    double scaled_frame_position = (double)frame * fm;
    if (p_scaled_frame_pos_out) {
        *p_scaled_frame_pos_out = scaled_frame_position;
    }
    // `master_frame` is the integer "anchor" on the Content Timeline.
    int64_t master_frame = (int64_t)floor(scaled_frame_position);

    // --- 2. Find "Start Line" on "Application Timeline" ---
    // This finds the *first* application frame that maps to this master_frame.
    // *** MODIFIED: Changed from multiplication to division ***
    // e.g., fm = 0.5 (slow-mo), master_frame = 1.0. Start line = ceil(1.0 / 0.5) = 2.
    int64_t master_frame_start_app_frame = (int64_t)ceil((double)master_frame / fm);

    // --- 3. Calculate Local Coordinates (all on "Application Timeline") ---
    // How many application frames has it been since this master_frame began?
    int64_t app_frames_into_gap = frame - master_frame_start_app_frame;
    int64_t segment_index = 0;
    int64_t local_progress_in_segment = 0;

    if (seg_block_length > 0) {
        segment_index = app_frames_into_gap / seg_block_length;
        local_progress_in_segment = app_frames_into_gap % seg_block_length;
    } else {
        segment_index = 0; 
        local_progress_in_segment = 0;
    }

    // --- 4. Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension) ---
    // varispeed_hold_block_count < 0: Mode 1 pure varispeed (Ground truth anchor holds infinitely)
    // segment_index < varispeed_hold_block_count: Mode 1 anchor holds for grace period
    // varispeed_hold_block_count == 0: Mode 2 immediately (Obfuscation / Alternate Timeline: paints over original values)
    if (varispeed_hold_block_count < 0 || segment_index < varispeed_hold_block_count) {
        // --- MODE 1: "Tape Varispeed" (Anchor) ---
        // Repeat the value of the `master_frame` from the Content Timeline.
        out.randVal = fpsr_sm_base(master_frame, (int64_t)minHold, (int64_t)maxHold, (int64_t)reseedInterval, (int64_t)seedInner, (int64_t)seedOuter, finalRandSwitch);
    } else {
        // --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        uint64_t gap_seed = splitmix64((uint64_t)master_frame + (uint64_t)segment_index);

        // Call using `local_progress_in_segment` (from Application Timeline)
        // and inject the unique `gap_seed` as 'seedInner'.
        out.randVal = fpsr_sm_base(local_progress_in_segment, (int64_t)minHold, (int64_t)maxHold, (int64_t)reseedInterval, (int64_t)gap_seed, (int64_t)seedOuter, finalRandSwitch);
    }
    // --- (END) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if (lod < 1) return out;

    // LOD 1: Compare with previous frame to check for change.
    // This call is on the "Application Timeline".
    FPSR_Output prev_out = fpsr_sm_get_details(frame - 1, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count);
    out.randVal_previous = prev_out.randVal; 
    out.has_changed = (out.randVal != prev_out.randVal);

    if (lod < 2) return out;

    // --- LOD 2: MODIFIED Robust Two-Phase Search ---
    int64_t low_int, high_int, mid_int, result_int; 
    double next_val_candidate = 0.0;
    int64_t step_int = 1;

    // --- Backwards Search for last_changed_frame (on Application Timeline) ---
    if (out.has_changed) {
        out.last_changed_frame = (int)frame;
    } else {
        int64_t bound_low_int = frame;
        step_int = 1;
        while (frame - step_int > frame - max_search_frames) { 
            double val_at_probe = fpsr_sm_get_details(frame - step_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (val_at_probe != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2; 
        }
        
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame - max_search_frames + 1;
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2; 
            double mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (mid_val == out.randVal) {
                double prev_mid_val = fpsr_sm_get_details(mid_int - 1, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
                if (prev_mid_val != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = (int)result_int;
    }

    // --- Forwards Search for next_changed_frame (on Application Timeline) ---
    int64_t bound_high_int = frame;
    step_int = 1;
    while (frame + step_int < frame + max_search_frames) { 
        double val_at_probe = fpsr_sm_get_details(frame + step_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe;
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2; 
    }

    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames;
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2; 
        double mid_val = fpsr_sm_get_details(mid_int, frame_multiplier, NULL, minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val;
            high_int = mid_int - 1; 
        } else {
            low_int = mid_int + 1; 
        }
    }
    out.next_changed_frame = (int)result_int;
    out.randVal_next_changed_frame = next_val_candidate; 
    
    // --- UPDATED hold_progress Calculation ---
    double hold_duration_app_frames = (double)out.next_changed_frame - (double)out.last_changed_frame;
    if (hold_duration_app_frames > 0.0) {
        out.hold_progress = ((double)frame - (double)out.last_changed_frame) / hold_duration_app_frames;
    } else {
        out.hold_progress = 0.0;
    }
    
    return out;
}

/**
 * ---- TM: Toggle Modulo Wrapper with Details ----
 * @brief Wrapper for fpsr_tm that returns a detailed FPSR_Output struct.
 * @param frame (int64_t) The current frame or time input.
 * @param frame_multiplier (double) The time scaling factor.
 * < 1.0 = Slow-Motion (Time Stretch)
 * = 1.0 = Normal Speed
 * > 1.0 = Fast-Motion (Time Compression)
 * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
 * @param periodA (int) The first hold duration (in frames).
 * @param periodB (int) The second hold duration (in frames).
 * @param periodSwitch (int) The fixed interval at which the hold duration is toggled.
 * @param seedInner (int) An offset for the toggle clock to de-sync it from the main clock.
 * @param seedOuter (int) An offset for the main clock to create unique output sequences.
 * @param finalRandSwitch (bool) A flag to enable/disable the final randomisation step.
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search.
 * @param seg_block_length (int) The "runway" length for HPQ logic.
 * @param varispeed_hold_block_count (int) Anchor persistence threshold:
 *        -1 = Pure Tape Varispeed (Exact 1:1 Ground-Truth Hold; Metrology mode)
 *         0 = Obfuscation / Alternate Timeline (Leaves no trace of the original
 *             values at their underlying frames; 'paints over the original painting'
 *             while preserving the macro rhythm grid)
 *       >=1 = Phrased Anchor + Infill (Anchor milestone holds for N runway blocks
 *             before generative sub-phrasing)
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_tm_get_details(
    int64_t frame, double frame_multiplier,
    double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
    int periodA, int periodB,
    int periodSwitch, int seedInner, int seedOuter, int finalRandSwitch,
    int lod, int max_search_frames,
    int seg_block_length,
    int varispeed_hold_block_count)
{
    FPSR_Output out = {0};
    
    // *** NEW: HPQ Timeline Definitions ***
    /*
    * --- HPQ Timeline Definitions ---
    * 1. "Application Timeline": The user's `frame` (e.g., 0, 1, 2...).
    * 2. "Content Timeline": The *original* algorithm's timeline (e.g., `master_frame` 0, 1, 2...).
    * `frame_multiplier` (fm) is the ratio that maps between them:
    * (Application Timeline Frame) * fm = (Content Timeline Frame)
    */
    
    // --- Sanitize frame_multiplier (now "playback_speed") ---
    double fm = (frame_multiplier == 0.0) ? 1.0 : frame_multiplier;

    // --- (START) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    // --- 1. Find coordinate on "Content Timeline" ---
    // *** MODIFIED: Changed from division to multiplication ***
    double scaled_frame_position = (double)frame * fm;
    if (p_scaled_frame_pos_out) {
        *p_scaled_frame_pos_out = scaled_frame_position;
    }
    int64_t master_frame = (int64_t)floor(scaled_frame_position);

    // --- 2. Find "Start Line" on "Application Timeline" ---
    // *** MODIFIED: Changed from multiplication to division ***
    int64_t master_frame_start_app_frame = (int64_t)ceil((double)master_frame / fm);

    // --- 3. Calculate Local Coordinates (all on "Application Timeline") ---
    int64_t app_frames_into_gap = frame - master_frame_start_app_frame;
    int64_t segment_index = 0;
    int64_t local_progress_in_segment = 0;

    if (seg_block_length > 0) {
        segment_index = app_frames_into_gap / seg_block_length;
        local_progress_in_segment = app_frames_into_gap % seg_block_length;
    } else {
        segment_index = 0;
        local_progress_in_segment = 0;
    }

    // --- 4. Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension) ---
    // varispeed_hold_block_count < 0: Mode 1 pure varispeed (Ground truth anchor holds infinitely)
    // segment_index < varispeed_hold_block_count: Mode 1 anchor holds for grace period
    // varispeed_hold_block_count == 0: Mode 2 immediately (Obfuscation / Alternate Timeline: paints over original values)
    if (varispeed_hold_block_count < 0 || segment_index < varispeed_hold_block_count) {
        // --- MODE 1: "Tape Varispeed" (Anchor) ---
        // Repeat the value of the `master_frame` from the Content Timeline.
        out.randVal = (float)fpsr_tm_base(master_frame, (int64_t)periodA, (int64_t)periodB, (int64_t)periodSwitch, (int64_t)seedInner, (int64_t)seedOuter, finalRandSwitch);
    } else {
        // --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        uint64_t gap_seed = splitmix64((uint64_t)master_frame + (uint64_t)segment_index);
        
        // Call using `local_progress_in_segment` (from Application Timeline)
        // and inject the unique `gap_seed` as 'seedInner'.
        out.randVal = (float)fpsr_tm_base(local_progress_in_segment, (int64_t)periodA, (int64_t)periodB, (int64_t)periodSwitch, (int64_t)gap_seed, (int64_t)seedOuter, finalRandSwitch);
    }
    // --- (END) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if (lod < 1) return out;

    // LOD 1
    FPSR_Output prev_out = fpsr_tm_get_details(frame - 1, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count);
    out.randVal_previous = prev_out.randVal; 
    out.has_changed = (out.randVal != prev_out.randVal);
    
    if (lod < 2) return out;

    // --- LOD 2: MODIFIED Robust Search (on Application Timeline) ---
    int64_t low_int, high_int, mid_int, result_int; 
    float next_val_candidate = 0.0f;
    int64_t step_int = 1;

    // --- Backwards Search for last_changed_frame ---
    if (out.has_changed) {
        out.last_changed_frame = (int)frame;
    } else {
        int64_t bound_low_int = frame;
        step_int = 1;
        while (frame - step_int > frame - max_search_frames) {
            float val_at_probe = fpsr_tm_get_details(frame - step_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (val_at_probe != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2;
        }
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame - max_search_frames + 1;
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2;
            float mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (mid_val == out.randVal) {
                float prev_mid_val = fpsr_tm_get_details(mid_int - 1, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
                if (prev_mid_val != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = (int)result_int;
    }

    // --- Forwards search ---
    int64_t bound_high_int = frame;
    step_int = 1;
    while (frame + step_int < frame + max_search_frames) {
        float val_at_probe = fpsr_tm_get_details(frame + step_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe;
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2;
    }
    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames;
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2;
        float mid_val = fpsr_tm_get_details(mid_int, frame_multiplier, NULL, periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val;
            high_int = mid_int - 1;
        } else {
            low_int = mid_int + 1;
        }
    }
    out.next_changed_frame = (int)result_int;
    out.randVal_next_changed_frame = next_val_candidate;
    
    // --- (START) REPLACEMENT: UPDATED hold_progress Calculation ---
    // Calculate progress based *purely* on the "Application Timeline".
    double hold_duration_app_frames = (double)out.next_changed_frame - (double)out.last_changed_frame;
    if (hold_duration_app_frames > 0.0) {
        out.hold_progress = (float)(((double)frame - (double)out.last_changed_frame) / hold_duration_app_frames);
    } else {
        out.hold_progress = 0.0f;
    }
    // --- (END) REPLACEMENT: UPDATED hold_progress Calculation ---

    return out;
}

/**
 * ---- QS: Quantised Switching Wrapper with Details ----
 * @brief Wrapper for fpsr_qs that returns a detailed FPSR_Output struct. Please refer to the base function for parameter explanations.
 * @param frame (int64_t) The current frame or time input.
 * @param frame_multiplier (double) The time scaling factor.
 * < 1.0 = Slow-Motion (Time Stretch)
 * = 1.0 = Normal Speed
 * > 1.0 = Fast-Motion (Time Compression)
 * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
 * @param baseWaveFreq (double) The base frequency for the sine waves.
 * @param stream2FreqMult (double) A multiplier for the second stream's frequency.
 * @param quantLevelsMinMax (int[2]) A list [min, max] quantization levels.
 * @param streamsOffset (int[2]) A list [offset1, offset2] for each sine stream.
 * @param quantOffsets (int[2]) A list [q_offset1, q_offset2] for each stream.
 * @param streamSwitchDur (int) The duration (in frames) before switching between streams.
 * @param stream1QuantDur (int) The quantization duration (in frames) for stream 1.
 * @param stream2QuantDur (int) The quantization duration (in frames) for stream 2.
 * @param finalRandSwitch (bool) A flag that can turn off the final randomisation step.
 * @param wavetable Optional custom wavetable (pass NULL to use default sine).
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search.
 * @param seg_block_length (int) The "runway" length for HPQ logic.
 * @param varispeed_hold_block_count (int) Anchor persistence threshold:
 *        -1 = Pure Tape Varispeed (Exact 1:1 Ground-Truth Hold; Metrology mode)
 *         0 = Obfuscation / Alternate Timeline (Leaves no trace of the original
 *             values at their underlying frames; 'paints over the original painting'
 *             while preserving the macro rhythm grid)
 *       >=1 = Phrased Anchor + Infill (Anchor milestone holds for N runway blocks
 *             before generative sub-phrasing)
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_qs_get_details(
    int64_t frame, double frame_multiplier,
    double* p_scaled_frame_pos_out,
    double baseWaveFreq, double stream2FreqMult,
    const int quantLevelsMinMax[2], const int streamsOffset[2], const int quantOffsets[2],
    int streamSwitchDur, int stream1QuantDur, int stream2QuantDur, int finalRandSwitch,
    const FPSR_Wavetable* wavetable,
    int lod, int max_search_frames,
    int seg_block_length,
    int varispeed_hold_block_count)
{
    FPSR_Output out = {0};
    
    // *** NEW: HPQ Timeline Definitions ***
    /*
    * --- HPQ Timeline Definitions ---
    * 1. "Application Timeline": The user's `frame` (e.g., 0, 1, 2...).
    * 2. "Content Timeline": The *original* algorithm's timeline (e.g., `master_frame` 0, 1, 2...).
    * `frame_multiplier` (fm) is the ratio that maps between them:
    * (Application Timeline Frame) * fm = (Content Timeline Frame)
    */

    // --- Sanitize frame_multiplier (now "playback_speed") ---
    double fm = (frame_multiplier == 0.0) ? 1.0 : frame_multiplier;

    // --- 1. Find coordinate on "Content Timeline" ---
    double scaled_frame_position = (double)frame * fm;
    if (p_scaled_frame_pos_out) {
        *p_scaled_frame_pos_out = scaled_frame_position;
    }
    int64_t master_frame = (int64_t)floor(scaled_frame_position);
    
    // --- 2. Find "Start Line" on "Application Timeline" ---
    int64_t master_frame_start_app_frame = (int64_t)ceil((double)master_frame / fm);

    // --- 3. Calculate Local Coordinates (all on "Application Timeline") ---
    int64_t app_frames_into_gap = frame - master_frame_start_app_frame;
    int64_t segment_index = 0;
    int64_t local_progress_in_segment = 0;

    if (seg_block_length > 0) {
        segment_index = app_frames_into_gap / seg_block_length;
        local_progress_in_segment = app_frames_into_gap % seg_block_length;
    } else {
        segment_index = 0;
        local_progress_in_segment = 0;
    }

    FPSR_Output base_qs_output;
    // --- 4. Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension) ---
    if (varispeed_hold_block_count < 0 || segment_index < varispeed_hold_block_count) {
        // --- MODE 1: "Tape Varispeed" (Anchor) ---
        base_qs_output = fpsr_qs_base(master_frame, (double)baseWaveFreq, (double)stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, wavetable);
    } else {
        // --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        uint64_t gap_seed = splitmix64((uint64_t)master_frame + (uint64_t)segment_index);
        
        int new_quantOffsets[2] = {
            quantOffsets[0] + (int)(gap_seed & 0xFFFFFFFF), 
            quantOffsets[1] + (int)((gap_seed >> 32) & 0xFFFFFFFF)
        };
        
        base_qs_output = fpsr_qs_base(local_progress_in_segment, (double)baseWaveFreq, (double)stream2FreqMult, quantLevelsMinMax, streamsOffset, new_quantOffsets, (int64_t)streamSwitchDur, (int64_t)stream1QuantDur, (int64_t)stream2QuantDur, finalRandSwitch, wavetable);
    }
    
    out.randVal = base_qs_output.randVal;
    out.randStreams[0] = base_qs_output.randStreams[0];
    out.randStreams[1] = base_qs_output.randStreams[1];
    out.selected_stream_idx = base_qs_output.selected_stream_idx;

    if (lod < 1) return out;

    // LOD 1
    FPSR_Output prev_out = fpsr_qs_get_details(frame - 1, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count);
    out.randVal_previous = prev_out.randVal;
    out.has_changed = (out.randVal != out.randVal_previous);
    
    if (lod < 2) return out;

    // --- LOD 2: Backwards & Forwards Search (on Application Timeline) --- 
    int64_t low_int, high_int, mid_int, result_int; 
    double next_val_candidate = 0.0;
    int64_t step_int = 1;

    // --- Backwards Search for last_changed_frame (on Application Timeline) ---
    if (out.has_changed) {
        out.last_changed_frame = (int)frame;
    } else {
        int64_t bound_low_int = frame;
        step_int = 1;
        while (frame - step_int > frame - max_search_frames) { 
            double val_at_probe = fpsr_qs_get_details(frame - step_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (val_at_probe != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2; 
        }
        
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame - max_search_frames + 1;
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2; 
            double mid_val = fpsr_qs_get_details(mid_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (mid_val == out.randVal) {
                double prev_mid_val = fpsr_qs_get_details(mid_int - 1, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
                if (prev_mid_val != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = (int)result_int;
    }

    // --- Forwards Search for next_changed_frame (on Application Timeline) ---
    int64_t bound_high_int = frame;
    step_int = 1;
    while (frame + step_int < frame + max_search_frames) { 
        double val_at_probe = fpsr_qs_get_details(frame + step_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe;
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2; 
    }

    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames;
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2; 
        double mid_val = fpsr_qs_get_details(mid_int, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur, finalRandSwitch, wavetable, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val;
            high_int = mid_int - 1; 
        } else {
            low_int = mid_int + 1; 
        }
    }
    out.next_changed_frame = (int)result_int;
    out.randVal_next_changed_frame = next_val_candidate; 
    
    // --- UPDATED hold_progress Calculation ---
    double hold_duration_app_frames = (double)out.next_changed_frame - (double)out.last_changed_frame;
    if (hold_duration_app_frames > 0.0) {
        out.hold_progress = ((double)frame - (double)out.last_changed_frame) / hold_duration_app_frames;
    } else {
        out.hold_progress = 0.0;
    }
    
    return out;
}

/**
 * ---- BD: Bitwise Decode Wrapper with Details ----
 * @brief Wrapper for fpsr_bd_base that returns a detailed FPSR_Output struct. Please refer to the base function for parameter explanations.
 * @param frame (int64_t) The current frame or time input.
 * @param frame_multiplier (double) The time scaling factor.
 * < 1.0 = Slow-Motion (Time Stretch)
 * = 1.0 = Normal Speed
 * > 1.0 = Fast-Motion (Time Compression)
 * @param p_scaled_frame_pos_out (double*) Optional output pointer to get the scaled frame position.
 * @param block_size (int) The size of the macro-rhythm in frames. Must be > 0.
 * @param streams_number (int) The number of parallel bitstreams to generate.
 * @param streams_offset (int) The frame offset between each parallel stream's seed.
 * @param intra_op (str) The unary (intra-stream) operation.
 *     Static ops: "none", "not", "lshift", "rshift", "rotl", "rotr".
 *     Dynamic ops: "lshift_dynamic", "rshift_dynamic", "rotl_dynamic", "rotr_dynamic".
 * @param dynamic_shift_bits (int) For dynamic ops, the number of controller bits to read
 *              to determine the shift/rotate amount (1-6 when chunk_bits=64).
 * @param static_shift_amount (int) For static ops, the fixed number of bits to shift/rotate.
 * @param inter_op (str) The binary (inter-stream) operation to combine multiple
 *              transformed streams. Options: "xor", "or", "and".
 * @param value_seed_offset (int) An additional seed offset for the final value calculation.
 * @param lod (int) The level of detail to calculate.
 * @param max_search_frames (int) A safety limit for the backward/forward search.
 * @param seg_block_length (int) The "runway" length for HPQ logic.
 * @param varispeed_hold_block_count (int) Anchor persistence threshold:
 *        -1 = Pure Tape Varispeed (Exact 1:1 Ground-Truth Hold; Metrology mode)
 *         0 = Obfuscation / Alternate Timeline (Leaves no trace of the original
 *             values at their underlying frames; 'paints over the original painting'
 *             while preserving the macro rhythm grid)
 *       >=1 = Phrased Anchor + Infill (Anchor milestone holds for N runway blocks
 *             before generative sub-phrasing)
 * @return FPSR_Output struct with metadata populated based on the LOD.
 */
FPSR_Output fpsr_bd_get_details(
    int64_t frame, double frame_multiplier,
    double* p_scaled_frame_pos_out, // Optional pointer to get the scaled time
    int block_size,
    int streams_number,
    int streams_offset,
    const char* intra_op,
    int dynamic_shift_bits,
    int static_shift_amount,
    const char* inter_op,
    int value_seed_offset,
    int lod, int max_search_frames,
    int seg_block_length,
    int varispeed_hold_block_count)
{
    FPSR_Output out = {0};
    
    // *** NEW: HPQ Timeline Definitions ***
    /*
    * --- HPQ Timeline Definitions ---
    * 1. "Application Timeline": The user's `frame` (e.g., 0, 1, 2...).
    * 2. "Content Timeline": The *original* algorithm's timeline (e.g., `master_frame` 0, 1, 2...).
    * `frame_multiplier` (fm) is the ratio that maps between them:
    * (Application Timeline Frame) * fm = (Content Timeline Frame)
    */
    
    // --- Sanitize frame_multiplier (now "playback_speed") ---
    double fm = (frame_multiplier == 0.0) ? 1.0 : frame_multiplier;

    // --- (START) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    // --- 1. Find coordinate on "Content Timeline" ---
    // *** MODIFIED: Changed from division to multiplication ***
    double scaled_frame_position = (double)frame * fm;
    if (p_scaled_frame_pos_out) {
        *p_scaled_frame_pos_out = scaled_frame_position;
    }
    int64_t master_frame = (int64_t)floor(scaled_frame_position);
    
    // --- 2. Find "Start Line" on "Application Timeline" ---
    // *** MODIFIED: Changed from multiplication to division ***
    int64_t master_frame_start_app_frame = (int64_t)ceil((double)master_frame / fm);

    // --- 3. Calculate Local Coordinates (all on "Application Timeline") ---
    int64_t app_frames_into_gap = frame - master_frame_start_app_frame;
    int64_t segment_index = 0;
    int64_t local_progress_in_segment = 0;

    if (seg_block_length > 0) {
        segment_index = app_frames_into_gap / seg_block_length;
        local_progress_in_segment = app_frames_into_gap % seg_block_length;
    } else {
        segment_index = 0;
        local_progress_in_segment = 0;
    }

    // --- 4. Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension) ---
    // varispeed_hold_block_count < 0: Mode 1 pure varispeed (Ground truth anchor holds infinitely)
    // segment_index < varispeed_hold_block_count: Mode 1 anchor holds for grace period
    // varispeed_hold_block_count == 0: Mode 2 immediately (Obfuscation / Alternate Timeline: paints over original values)
    if (varispeed_hold_block_count < 0 || segment_index < varispeed_hold_block_count) {
        // --- MODE 1: "Tape Varispeed" (Anchor) ---
        // Repeat the value of the `master_frame` from the Content Timeline.
        out.randVal = (float)fpsr_bd_base(
            master_frame, (int64_t)block_size, streams_number, (int64_t)streams_offset,
            intra_op, dynamic_shift_bits, static_shift_amount, inter_op, (int64_t)value_seed_offset
        );
    } else {
        // --- MODE 2: "Telescopic Extension" (Generative Phrase) ---
        uint64_t gap_seed = splitmix64((uint64_t)master_frame + (uint64_t)segment_index);
        
        // For BD, we inject the unique seed as the 'value_seed_offset'.
        // We also pass `local_progress_in_segment` (from Application Timeline) as the frame.
        out.randVal = (float)fpsr_bd_base(
            local_progress_in_segment, (int64_t)block_size, streams_number, (int64_t)streams_offset,
            intra_op, dynamic_shift_bits, static_shift_amount, inter_op, (int64_t)gap_seed // Cast seed to int64_t
        );
    }
    // --- (END) REPLACEMENT: HIERARCHICAL PHRASED QUANTISATION (HPQ) LOGIC ---
    
    if (lod < 1) return out;

    // LOD 1
    FPSR_Output prev_out = fpsr_bd_get_details(frame - 1, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count);
    out.randVal_previous = prev_out.randVal;
    out.has_changed = (out.randVal != out.randVal_previous);

    if (lod < 2) return out;

    // --- LOD 2: MODIFIED Robust Search (on Application Timeline) ---
    int64_t low_int, high_int, mid_int, result_int; 
    float next_val_candidate = 0.0f;
    int64_t step_int = 1;

    // --- Backwards Search for last_changed_frame ---
    if (out.has_changed) {
        out.last_changed_frame = (int)frame;
    } else {
        int64_t bound_low_int = frame;
        step_int = 1;
        while (frame - step_int > frame - max_search_frames) {
            float val_at_probe = fpsr_bd_get_details(frame - step_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (val_at_probe != out.randVal) {
                bound_low_int = frame - step_int;
                break;
            }
            bound_low_int = frame - step_int;
            step_int *= 2;
        }
        low_int = bound_low_int;
        high_int = frame;
        result_int = frame - max_search_frames + 1;
        while(low_int <= high_int) {
            mid_int = low_int + (high_int - low_int) / 2;
            float mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
            if (mid_val == out.randVal) {
                float prev_mid_val = fpsr_bd_get_details(mid_int - 1, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
                if (prev_mid_val != out.randVal) {
                    result_int = mid_int; break;
                }
                high_int = mid_int - 1; 
            } else {
                low_int = mid_int + 1; 
            }
        }
        out.last_changed_frame = (int)result_int;
    }

    // --- Forwards search ---
    int64_t bound_high_int = frame;
    step_int = 1;
    while (frame + step_int < frame + max_search_frames) {
        float val_at_probe = fpsr_bd_get_details(frame + step_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (val_at_probe != out.randVal) {
            bound_high_int = frame + step_int;
            next_val_candidate = val_at_probe;
            break;
        }
        bound_high_int = frame + step_int;
        step_int *= 2;
    }
    low_int = frame;
    high_int = bound_high_int;
    result_int = frame + max_search_frames;
    while(low_int <= high_int) {
        mid_int = low_int + (high_int - low_int) / 2;
        float mid_val = fpsr_bd_get_details(mid_int, frame_multiplier, NULL, block_size, streams_number, streams_offset, intra_op, dynamic_shift_bits, static_shift_amount, inter_op, value_seed_offset, 0, 0, seg_block_length, varispeed_hold_block_count).randVal;
        if (mid_val != out.randVal) {
            result_int = mid_int;
            next_val_candidate = mid_val;
            high_int = mid_int - 1;
        } else {
            low_int = mid_int + 1;
        }
    }
    out.next_changed_frame = (int)result_int;
    out.randVal_next_changed_frame = next_val_candidate;
    
    // --- (START) REPLACEMENT: UPDATED hold_progress Calculation ---
    // Calculate progress based *purely* on the "Application Timeline".
    double hold_duration_app_frames = (double)out.next_changed_frame - (double)out.last_changed_frame;
    if (hold_duration_app_frames > 0.0) {
        out.hold_progress = (float)(((double)frame - (double)out.last_changed_frame) / hold_duration_app_frames);
    } else {
        out.hold_progress = 0.0f;
    }
    // --- (END) REPLACEMENT: UPDATED hold_progress Calculation ---

    return out;
}


int main() {
    // Example usage of the FPSR algorithms with detailed output
    
    // Report OS type
    // if (os == 0) {
    //     printf("Windows OS.\n");
    // } else if (os == 1) {
    //     printf("POSIX OS.\n");
    // } else {
    //     printf("Unknown OS.\n");
    // }
    
    // Algorithms: 0 - SM, 1 - TM, 2 - QS, 3 - BD
    int algo = 3; // Change this value to 0, 1, 2, or 3 to test different algorithms
    char algo_name[][4] = {"SM", "TM", "QS", "BD"}; // Names for the algorithms
    printf("Using algorithm FPS-R: %s\n", algo_name[algo]);

    int start_frames[] = {90, 100, 103, 100}; // Starting frames for each algorithm
    int num_frames = 30; // Run a loop of 30 frames to demonstrate changes
    int lod = 2; // Level of detail (0, 1, or 2) for rich output
    
    // *** MODIFIED: This comment now reflects the new, intuitive logic ***
    // 1.0 = normal speed
    // 0.5 = 0.5x speed (slow motion / time stretch)
    // 2.0 = 2.0x speed (fast motion / time compression)
    double main_frame_multiplier = 1.0; // Default value representing "Normal Speed"
    // create a string to indicate speed up or slow down comment
    char speed_mode_description[20];
    // Check if the frame multiplier is less than 1.0, indicating "Slow-Down" mode
    if (main_frame_multiplier < 1.0) {
        snprintf(speed_mode_description, sizeof(speed_mode_description), "Slow-Down");
    } else if (main_frame_multiplier > 1.0) { // Speed-Up mode
        snprintf(speed_mode_description, sizeof(speed_mode_description), "Speed-Up");
    } else {
        snprintf(speed_mode_description, sizeof(speed_mode_description), "Normal Speed");
    }
    printf("Frame Multiplier: %.2f (%s)\n", main_frame_multiplier, speed_mode_description);
    
    // *** HPQ Parameters ***
    // A value of 5 means the gap is segmented into 5-frame runway segments.
    int seg_block_length = 5;
    // Anchor persistence threshold:
    // -1 = Pure Tape Varispeed (Exact 1:1 Ground-Truth Hold; Metrology mode)
    //  0 = Obfuscation / Alternate Timeline (Leaves no trace of the original
    //      values at their underlying frames; 'paints over the original painting'
    //      while preserving the macro rhythm grid)
    // >=1 = Phrased Anchor + Infill (Anchor milestone holds for N runway blocks
    //      before generative sub-phrasing)
    int varispeed_hold_block_count = 1;

    for (int loop_frame = 0; loop_frame < num_frames; loop_frame++) {
        int64_t frame = (int64_t)loop_frame + (int64_t)start_frames[algo]; // Use int64_t for frame
        double frame_multiplier = main_frame_multiplier; // Use double for multiplier
        FPSR_Output output = {0};
        
        if (algo == 0) {
            // Parameters for FPS-R:SM
            int minHoldFrames = 7;      // Minimum hold duration
            int maxHoldFrames = 9;      // Maximum hold duration
            int reseedFrames = 6;       // Reseed interval
            int offsetInner = -41;      // Inner seed offset
            int offsetOuter = 23;       // Outer seed offset
            int finalRandSwitch = 1;    // Final randomisation switch
            int max_search_frames = 50; // Safety limit for search

            // Call fpsr_sm_get_details
            output = fpsr_sm_get_details(frame, frame_multiplier, NULL, minHoldFrames, maxHoldFrames, reseedFrames, offsetInner, offsetOuter, finalRandSwitch, lod, max_search_frames, seg_block_length, varispeed_hold_block_count);
        } else if (algo == 1) {
            // Parameters for FPS-R:TM
            int periodA = 8;            // First hold duration
            int periodB = 5;            // Second hold duration
            int periodSwitch = 6;       // Period switch interval
            int offsetInner = 15;       // Inner seed offset
            int offsetOuter = 0;        // Outer seed offset
            int finalRandSwitch = 1;    // Final randomisation switch
            int max_search_frames = 50; // Safety limit for search

            // Call fpsr_tm_get_details
            output = fpsr_tm_get_details(frame, frame_multiplier, NULL,
                periodA, periodB, periodSwitch, offsetInner, offsetOuter, 
                finalRandSwitch, lod, max_search_frames, seg_block_length, varispeed_hold_block_count);
        } else if (algo == 2) {
            // Parameters for FPS-R:QS
            float baseWaveFreq = 0.012f;    // Base wave frequency for stream 1
            float stream2FreqMult = 3.1f;   // Frequency multiplier for stream 2
            int quantLevelsMinMax[2] = {4, 12}; // Min and max quantisation levels
            int streamsOffset[2] = {0, 76}; // Frame offsets for each stream
            int quantOffsets[2] = {10, 81}; // Quantisation level offsets
            int streamSwitchDur = 8;        // Duration after which streams switch
            int stream1QuantDur = 10;       // Duration for stream 1 quantisation hold
            int stream2QuantDur = 13;       // Duration for stream 2 quantisation hold
            int finalRandSwitch = 1;        // Final randomisation switch

            // Pass NULL to use canonical sine (&FPSR_DEFAULT_SINE_WAVETABLE)
            // Or pass &FPSR_CUSTOM_WAVETABLE to test your proprietary custom key
            const FPSR_Wavetable* wavetable = NULL;

            int max_search_frames = 50;     // Safety limit for search

            // Call fpsr_qs_get_details
            output = fpsr_qs_get_details(frame, frame_multiplier, NULL, baseWaveFreq, stream2FreqMult, 
                quantLevelsMinMax, streamsOffset, quantOffsets, streamSwitchDur, 
                stream1QuantDur, stream2QuantDur, finalRandSwitch, 
                wavetable, lod, max_search_frames, seg_block_length, varispeed_hold_block_count);
        } else if (algo == 3) {
            // Parameters for FPS-R:BD
            int p_block_size = 64;           // Size of the macro-rhythm block
            int p_streams_number = 2;        // Number of parallel bitstreams
            int p_streams_offset = 10;       // Frame offset between each stream's seed
            const char* p_intra_op = "rotl_dynamic"; // Intra-stream operation on each stream
            int p_dynamic_shift_bits = 6;    // Dynamic shift bits for intra-op
            int p_static_shift_amount = 1;   // Static shift amount for intra-op
            const char* p_inter_op = "xor";  // Inter-stream operation to combine transformed streams
            int p_value_seed_offset = 78901; // Additional seed offset for final value
            int max_search_frames = 100; // BD blocks can be large

            output = fpsr_bd_get_details(
                frame, frame_multiplier, NULL, p_block_size, p_streams_number, p_streams_offset,
                p_intra_op, p_dynamic_shift_bits, p_static_shift_amount,
                p_inter_op, p_value_seed_offset, lod, max_search_frames, seg_block_length, varispeed_hold_block_count
            );
        }


        // Print the output for the current frame
        // Use %lld for int64_t frame
        printf("Frame %lld: randVal %.6f, prevVal %.6f, changed %d, progress %.3f, last %d, next %d ",
            frame, output.randVal, output.randVal_previous, output.has_changed, output.hold_progress, output.last_changed_frame, output.next_changed_frame);
        if (algo == 2) {
            printf("| s_idx %d, s[0] %.3f, s[1] %.3f ", output.selected_stream_idx, output.randStreams[0], output.randStreams[1]);
        }
        if (output.has_changed) printf("(jumped)");
        printf("\n");
        
    }

    return 0;
}