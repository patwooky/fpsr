// fpsr_core_lib.js — FPS-R core, extracted from fpsr_visualiser.html (17 Nov 2026).
//
// Classic (non-module) script: load it BEFORE the visualiser's inline script
// with <script src="fpsr_core_lib.js"></script>. It has no DOM access and no
// dependencies; the visualiser and the fit explorer can both load it.
//
// Everything below is verbatim from the visualiser, comments included, except
// for two changes forced by leaving the DOMContentLoaded closure:
//   1. Legacy A/B state now lives in fpsrLegacyState here, reset with
//      fpsr_legacy_reset(). The visualiser used to reset the same four
//      variables from updateParams(), rewindAnimation() and
//      resetStatefulWarmup(); those call sites now call fpsr_legacy_reset().
//   2. getFpsrDetails(appFrame, activeAlgorithm, globalParams) and
//      getFpsrValue(appFrame, activeAlgorithm, globalParams) now take the
//      algorithm and params explicitly instead of closing over the
//      visualiser's variables. All other core functions keep their exact
//      original signatures.
//   (The 1024-point sine LUT, BigInt helpers, SM/TM/QS/BD, Perlin/Worley,
//   LOD + HPQ wrappers, findChangeBoundary and executeWithLod are unchanged.)
//
// NOTE — this file supersedes the earlier draft fpsr.js (built 16 Nov 2026
// from FPSR_Tech.md): that draft's portable_rand used a sin-hash and its
// SM reseed / finalRandSwitch details were unverified. This file is the
// visualiser's own code, so its portable_rand is splitmix64-based,
// fpsr_sm_base_js quantises via the reseed anchor, and finalRandSwitch IS
// in the SM signature. Verify the fit explorer against THIS file, not fpsr.js.

// --- Core FPS-R Logic (64-bit BigInt Port) ---

const CHUNK_BITS = 64n;

// --- 1024-point Canonical Sine Lookup Table (mirrors C reference) ---
const FPSR_SINE_LUT_1024 = new Float64Array([
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
]);

const FPSR_DEFAULT_SINE_WAVETABLE = {
    data: FPSR_SINE_LUT_1024,
    size: 1024,
    mask: 1023
};

function fpsr_sample_wavetable(phase, wt = FPSR_DEFAULT_SINE_WAVETABLE) {
    const norm_phase = phase - Math.floor(phase);
    const scaled = norm_phase * wt.size;
    const idx1 = Math.floor(scaled);
    const frac = scaled - idx1;
    const i1 = idx1 & wt.mask;
    const i2 = (idx1 + 1) & wt.mask;
    return wt.data[i1] * (1.0 - frac) + wt.data[i2] * frac;
}

function i64_floor_mod(a, m) {
    if (m === 0n) return 0n;
    const r = a % m;
    return r < 0n ? r + m : r;
}

function i64_align_down(a, m) {
    if (m === 0n) return a;
    return a - i64_floor_mod(a, m);
}

function splitmix64(x) {
    x = BigInt.asUintN(64, x + 0x9E3779B97F4A7C15n);
    x = BigInt.asUintN(64, (x ^ (x >> 30n)) * 0xBF58476D1CE4E5B9n);
    x = BigInt.asUintN(64, (x ^ (x >> 27n)) * 0x94D049BB133111EBn);
    x = x ^ (x >> 31n);
    return BigInt.asUintN(64, x);
}

function portable_rand_u64(seed) {
    const r = splitmix64(BigInt(seed));
    return Number(r >> 11n) * (1.0 / 9007199254740992.0);
}

// --- Algorithm 1: FPS-R: SM (Stacked Modulo) ---
function fpsr_sm_base_js(frame, params) {
    let { minHold, maxHold, reseedInterval, seedInner, seedOuter, finalRandSwitch } = params;
    frame = BigInt(frame); // Ensure frame is BigInt for base logic
    if (reseedInterval < 1n) reseedInterval = 1n;
    const reseed_anchor = (seedInner + frame) - i64_floor_mod(seedInner + frame, reseedInterval);
    const rand_for_duration = portable_rand_u64(reseed_anchor);
    let holdDuration = minHold + BigInt(Math.floor(rand_for_duration * Number(maxHold - minHold)));
    if (holdDuration < 1n) holdDuration = 1n;
    const held_integer_state = i64_align_down((seedOuter + frame), holdDuration);
    if (finalRandSwitch) {
        const seed = BigInt.asUintN(64, held_integer_state);
        return portable_rand_u64(seed);
    } else {
        return Number(held_integer_state);
    }
}

// --- Algorithm 2: FPS-R: TM (Toggle Modulo) ---
function fpsr_tm_base_js(frame, params) {
    let { periodA, periodB, periodSwitch, seedInner, seedOuter, finalRandSwitch } = params;
    frame = BigInt(frame);
    if (periodSwitch < 1n) periodSwitch = 1n;
    const inner_clock_frame = seedInner + frame;
    const r = i64_floor_mod(inner_clock_frame, periodSwitch);
    let holdDuration = (2n * r < periodSwitch) ? periodA : periodB;
    if (holdDuration < 1n) holdDuration = 1n;
    const outer_clock_frame = seedOuter + frame;
    const held_integer_state = i64_align_down(outer_clock_frame, holdDuration);
    if (finalRandSwitch) {
        const seed = BigInt.asUintN(64, held_integer_state);
        return portable_rand_u64(seed);
    } else {
        return Number(held_integer_state);
    }
}

// --- Algorithm 3: FPS-R: QS (Quantized Sine) ---
function fpsr_qs_base_js(frame, params, wavetable = FPSR_DEFAULT_SINE_WAVETABLE) {
    frame = BigInt(frame);
    let {
        baseWaveFreq, stream2FreqMult, quantLevelsMinMax, streamsOffset,
        quantOffsets, streamSwitchDur, stream1QuantDur, stream2QuantDur,
        finalRandSwitch
    } = params;

    if (streamSwitchDur < 1n) streamSwitchDur = 1n;
    if (stream1QuantDur < 1n) stream1QuantDur = 1n;
    if (stream2QuantDur < 1n) stream2QuantDur = 1n;

    const quant_min = BigInt(quantLevelsMinMax[0]);
    const quant_max = BigInt(quantLevelsMinMax[1]);
    const quant_range = quant_max - quant_min + 1n;

    const s1_quant_seed_aligned = i64_align_down(BigInt(quantOffsets[0]) + frame, stream1QuantDur);
    const s1_rand_for_quant = portable_rand_u64(s1_quant_seed_aligned);
    let s1_quant_level = quant_min + BigInt(Math.floor(s1_rand_for_quant * Number(quant_range)));

    const s2_quant_seed_aligned = i64_align_down(BigInt(quantOffsets[1]) + frame, stream2QuantDur);
    const s2_rand_for_quant = portable_rand_u64(s2_quant_seed_aligned);
    let s2_quant_level = quant_min + BigInt(Math.floor(s2_rand_for_quant * Number(quant_range)));

    if (s1_quant_level < 1n) s1_quant_level = 1n;
    if (s2_quant_level < 1n) s2_quant_level = 1n;

    // Phase expressed in cycles [0.0, 1.0)
    const phase1 = (Number(streamsOffset[0]) + Number(frame)) * baseWaveFreq;
    const phase2 = (Number(streamsOffset[1]) + Number(frame)) * baseWaveFreq * stream2FreqMult;

    // Values sampled from the unipolar wavetable [0.0, 1.0]
    const stream1_raw_sine = fpsr_sample_wavetable(phase1, wavetable);
    const stream2_raw_sine = fpsr_sample_wavetable(phase2, wavetable);

    const stream1_val = Math.floor(stream1_raw_sine * Number(s1_quant_level)) / Number(s1_quant_level);
    const stream2_val = Math.floor(stream2_raw_sine * Number(s2_quant_level)) / Number(s2_quant_level);

    const r = i64_floor_mod(frame, streamSwitchDur);
    const selected_stream_idx = (2n * r < streamSwitchDur) ? 0 : 1;
    const active_stream_val = (selected_stream_idx === 0) ? stream1_val : stream2_val;

    let randVal = active_stream_val;
    if (finalRandSwitch === 1) {
        const hashed_int = BigInt(Math.floor(active_stream_val * 1000000.0));
        randVal = portable_rand_u64(hashed_int);
    }

    return {
        randVal: randVal,
        randStreams: [stream1_val, stream2_val],
        selected_stream_idx: selected_stream_idx
    };
}

// --- Algorithm 4: FPS-R: BD (Bitstream Distortion) ---
function u64_circular_left_shift(value, shift) {
    const s = BigInt(shift) % CHUNK_BITS;
    if (s === 0n) return value;
    return BigInt.asUintN(64, (value << s) | (value >> (CHUNK_BITS - s)));
}

function u64_circular_right_shift(value, shift) {
    const s = BigInt(shift) % CHUNK_BITS;
    if (s === 0n) return value;
    return BigInt.asUintN(64, (value >> s) | (value << (CHUNK_BITS - s)));
}

function get_bit(n, block_size, chunks, num_chunks) {
    if (n < 0n || n >= block_size) return 0n;
    const chunk_index = n / CHUNK_BITS;
    const bit_index = n % CHUNK_BITS;
    if (chunk_index >= num_chunks) return 0n;
    return (chunks[Number(chunk_index)] >> bit_index) & 1n;
}

function fpsr_bd_base_js(frame, params) {
    frame = BigInt(frame);
    let {
        blockSize, streamsNumber, streamsOffset, intraOp,
        dynamicShiftBits, staticShiftAmount, interOp, valueSeedOffset
    } = params;

    if (blockSize <= 0n) blockSize = 1n;
    if (streamsNumber < 1) streamsNumber = 1;

    const sanitized_static_shift = BigInt(staticShiftAmount) & (CHUNK_BITS - 1n);

    const outer_anchor = i64_align_down(frame, blockSize);
    const num_chunks = (blockSize + (CHUNK_BITS - 1n)) / CHUNK_BITS;

    let raw_streams = new Array(streamsNumber);
    for (let i = 0; i < streamsNumber; i++) {
        raw_streams[i] = new Array(Number(num_chunks)).fill(0n);
    }

    let transformed_streams = new Array(streamsNumber);
    for (let i = 0; i < streamsNumber; i++) {
        transformed_streams[i] = new Array(Number(num_chunks)).fill(0n);
    }

    let final_chunks = new Array(Number(num_chunks)).fill(0n);

    for (let i = 0; i < streamsNumber; i++) {
        let stream_seed = outer_anchor + (BigInt(i) * streamsOffset);
        for (let j = 0n; j < num_chunks; j++) {
            raw_streams[i][Number(j)] = splitmix64(stream_seed + j);
        }
    }

    const is_dynamic = (intraOp.endsWith("_dynamic"));
    const num_transformed_streams = is_dynamic ? (Math.floor(streamsNumber / 2) + streamsNumber % 2) : streamsNumber;

    if (is_dynamic) {
        const max_bits_for_shift = 6n;
        let bit_mask_size = BigInt(dynamicShiftBits);
        if (bit_mask_size < 1n) bit_mask_size = 1n;
        if (bit_mask_size > max_bits_for_shift) bit_mask_size = max_bits_for_shift;
        const bit_mask = (1n << bit_mask_size) - 1n;

        for (let i = 0; i < Math.floor(streamsNumber / 2); i++) {
            const data_idx = i * 2;
            const controller_idx = i * 2 + 1;
            const target_idx = i;
            const data_stream = raw_streams[data_idx];
            const controller_stream = raw_streams[controller_idx];

            for (let j = 0n; j < num_chunks; j++) {
                const dynamic_shift = Number(controller_stream[Number(j)] & bit_mask);
                switch (intraOp) {
                    case "lshift_dynamic": transformed_streams[target_idx][Number(j)] = BigInt.asUintN(64, data_stream[Number(j)] << BigInt(dynamic_shift % 64)); break;
                    case "rshift_dynamic": transformed_streams[target_idx][Number(j)] = data_stream[Number(j)] >> BigInt(dynamic_shift % 64); break;
                    case "rotl_dynamic": transformed_streams[target_idx][Number(j)] = u64_circular_left_shift(data_stream[Number(j)], dynamic_shift); break;
                    case "rotr_dynamic": transformed_streams[target_idx][Number(j)] = u64_circular_right_shift(data_stream[Number(j)], dynamic_shift); break;
                }
            }
        }
        if (streamsNumber % 2 != 0) {
            transformed_streams[num_transformed_streams - 1] = raw_streams[streamsNumber - 1];
        }
    } else {
        for (let i = 0; i < streamsNumber; i++) {
            for (let j = 0n; j < num_chunks; j++) {
                switch (intraOp) {
                    case "not": transformed_streams[i][Number(j)] = BigInt.asUintN(64, ~raw_streams[i][Number(j)]); break;
                    case "lshift": transformed_streams[i][Number(j)] = BigInt.asUintN(64, raw_streams[i][Number(j)] << sanitized_static_shift); break;
                    case "rshift": transformed_streams[i][Number(j)] = raw_streams[i][Number(j)] >> sanitized_static_shift; break;
                    case "rotl": transformed_streams[i][Number(j)] = u64_circular_left_shift(raw_streams[i][Number(j)], Number(sanitized_static_shift)); break;
                    case "rotr": transformed_streams[i][Number(j)] = u64_circular_right_shift(raw_streams[i][Number(j)], Number(sanitized_static_shift)); break;
                    default: transformed_streams[i][Number(j)] = raw_streams[i][Number(j)]; // "none"
                }
            }
        }
    }

    if (num_transformed_streams > 0) {
        final_chunks = [...transformed_streams[0]];
        for (let i = 1; i < num_transformed_streams; i++) {
            for (let j = 0n; j < num_chunks; j++) {
                switch (interOp) {
                    case "or": final_chunks[Number(j)] |= transformed_streams[i][Number(j)]; break;
                    case "and": final_chunks[Number(j)] &= transformed_streams[i][Number(j)]; break;
                    default: final_chunks[Number(j)] ^= transformed_streams[i][Number(j)]; // "xor"
                }
                final_chunks[Number(j)] = BigInt.asUintN(64, final_chunks[Number(j)]);
            }
        }
    }

    const current_pos_in_block = frame - outer_anchor;
    let last_flip_pos = 0n;
    for (let i = current_pos_in_block; i > 0n; i--) {
        if (get_bit(i, blockSize, final_chunks, num_chunks) != get_bit(i - 1n, blockSize, final_chunks, num_chunks)) {
            last_flip_pos = i;
            break;
        }
    }

    const final_seed = outer_anchor + last_flip_pos + valueSeedOffset;
    return portable_rand_u64(final_seed);
}

// --- Two-Phase Boundary Search Helper for LOD 2 ---
// Uses exponential probing followed by binary search matching fpsr_algo_wrap_reference.c
function findChangeBoundary(evalFunc, startFrame, direction, maxSearchFrames, targetVal) {
    let low = 0;
    let high = 1;

    // Phase 1: Exponential probe
    while (high <= maxSearchFrames) {
        const testFrame = startFrame + BigInt(direction * high);
        const out = evalFunc(testFrame);
        if (out.randVal !== targetVal) {
            break;
        }
        low = high;
        high *= 2;
    }

    if (high > maxSearchFrames) {
        const testFrame = startFrame + BigInt(direction * maxSearchFrames);
        const out = evalFunc(testFrame);
        if (out.randVal === targetVal) {
            return { frame: Number(testFrame), val: out.randVal };
        }
        high = maxSearchFrames;
    }

    // Phase 2: Binary search between low and high
    let boundaryOffset = high;
    let boundaryVal = evalFunc(startFrame + BigInt(direction * high)).randVal;

    while (low <= high) {
        const mid = Math.floor((low + high) / 2);
        const testFrame = startFrame + BigInt(direction * mid);
        const out = evalFunc(testFrame);

        if (out.randVal !== targetVal) {
            boundaryOffset = mid;
            boundaryVal = out.randVal;
            high = mid - 1; // Seek closer mismatch
        } else {
            low = mid + 1;
        }
    }

    return {
        frame: Number(startFrame + BigInt(direction * boundaryOffset)),
        val: boundaryVal
    };
}

// Generic LOD Decorator executing LOD 0, 1, and 2 calculations
function executeWithLod(evalLod0Func, appFrame, lod, maxSearchFrames = 300) {
    const appFrameBigInt = BigInt(appFrame);
    const baseOutput = evalLod0Func(appFrameBigInt);

    let output = {
        randVal: typeof baseOutput === 'object' ? baseOutput.randVal : baseOutput,
        has_changed: 0,
        randVal_previous: 0.0,
        hold_progress: 0.0,
        last_changed_frame: Number(appFrameBigInt),
        next_changed_frame: Number(appFrameBigInt),
        randVal_next_changed_frame: 0.0,
        randStreams: (baseOutput && baseOutput.randStreams) ? baseOutput.randStreams : [0.0, 0.0],
        selected_stream_idx: (baseOutput && baseOutput.selected_stream_idx !== undefined) ? baseOutput.selected_stream_idx : 0
    };

    if (lod <= 0) return output;

    // --- LOD 1: Previous value & change detection ---
    const prevBase = evalLod0Func(appFrameBigInt - 1n);
    output.randVal_previous = typeof prevBase === 'object' ? prevBase.randVal : prevBase;
    output.has_changed = (output.randVal !== output.randVal_previous) ? 1 : 0;

    if (lod <= 1) return output;

    // --- LOD 2: Boundary search & progress calculation ---
    const evalAdapter = (f) => {
        const res = evalLod0Func(f);
        return { randVal: typeof res === 'object' ? res.randVal : res };
    };

    // Backward search for last changed frame
    const backwardResult = findChangeBoundary(evalAdapter, appFrameBigInt, -1, maxSearchFrames, output.randVal);
    output.last_changed_frame = backwardResult.frame + 1;

    // Forward search for next changed frame
    const forwardResult = findChangeBoundary(evalAdapter, appFrameBigInt, 1, maxSearchFrames, output.randVal);
    output.next_changed_frame = forwardResult.frame;
    output.randVal_next_changed_frame = forwardResult.val;

    // Progress calculation normalized to [0, 1]
    const totalSpan = output.next_changed_frame - output.last_changed_frame;
    if (totalSpan > 0) {
        output.hold_progress = (Number(appFrameBigInt) - output.last_changed_frame) / totalSpan;
    } else {
        output.hold_progress = 0.0;
    }

    return output;
}

// --- Algorithm-Specific HPQ + LOD Wrappers ---

function fpsr_sm_get_details(appFrame, params, fm, segBlockLength, varispeedHoldBlockCount, lod) {
    const evalLod0 = (f) => {
        const effectiveFm = (fm === 0.0) ? 1.0 : fm;
        const scaled_pos = Number(f) * effectiveFm;
        const master_frame = BigInt(Math.floor(scaled_pos));
        const master_start = BigInt(Math.ceil(Number(master_frame) / effectiveFm));
        const gap = f - master_start;
        const segLen = BigInt(segBlockLength);
        const seg_idx = segLen > 0n ? gap / segLen : 0n;
        const holdCount = BigInt(varispeedHoldBlockCount);

        // Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension)
        if (holdCount < 0n || seg_idx < holdCount) {
            // MODE 1: "Tape Varispeed" (Anchor)
            return fpsr_sm_base_js(master_frame, params);
        } else {
            // MODE 2: "Telescopic Extension" (Generative Phrase)
            const local_progress = segLen > 0n ? (gap % segLen) : 0n;
            const gap_params = { ...params, seedInner: splitmix64(master_frame + seg_idx) };
            return fpsr_sm_base_js(local_progress, gap_params);
        }
    };
    return executeWithLod(evalLod0, appFrame, lod);
}

function fpsr_tm_get_details(appFrame, params, fm, segBlockLength, varispeedHoldBlockCount, lod) {
    const evalLod0 = (f) => {
        const effectiveFm = (fm === 0.0) ? 1.0 : fm;
        const scaled_pos = Number(f) * effectiveFm;
        const master_frame = BigInt(Math.floor(scaled_pos));
        const master_start = BigInt(Math.ceil(Number(master_frame) / effectiveFm));
        const gap = f - master_start;
        const segLen = BigInt(segBlockLength);
        const seg_idx = segLen > 0n ? gap / segLen : 0n;
        const holdCount = BigInt(varispeedHoldBlockCount);

        // Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension)
        if (holdCount < 0n || seg_idx < holdCount) {
            // MODE 1: "Tape Varispeed" (Anchor)
            return fpsr_tm_base_js(master_frame, params);
        } else {
            // MODE 2: "Telescopic Extension" (Generative Phrase)
            const local_progress = segLen > 0n ? (gap % segLen) : 0n;
            const gap_params = { ...params, seedInner: splitmix64(master_frame + seg_idx) };
            return fpsr_tm_base_js(local_progress, gap_params);
        }
    };
    return executeWithLod(evalLod0, appFrame, lod);
}

function fpsr_qs_get_details(appFrame, params, fm, segBlockLength, varispeedHoldBlockCount, lod) {
    const evalLod0 = (f) => {
        const effectiveFm = (fm === 0.0) ? 1.0 : fm;
        const scaled_pos = Number(f) * effectiveFm;
        const master_frame = BigInt(Math.floor(scaled_pos));
        const master_start = BigInt(Math.ceil(Number(master_frame) / effectiveFm));
        const gap = f - master_start;
        const segLen = BigInt(segBlockLength);
        const seg_idx = segLen > 0n ? gap / segLen : 0n;
        const holdCount = BigInt(varispeedHoldBlockCount);

        // Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension)
        if (holdCount < 0n || seg_idx < holdCount) {
            // MODE 1: "Tape Varispeed" (Anchor)
            return fpsr_qs_base_js(master_frame, params);
        } else {
            // MODE 2: "Telescopic Extension" (Generative Phrase)
            const local_progress = segLen > 0n ? (gap % segLen) : 0n;
            const gap_seed = splitmix64(master_frame + seg_idx);
            const seedPart1 = Number(BigInt.asIntN(32, gap_seed & 0xFFFFFFFFn));
            const seedPart2 = Number(BigInt.asIntN(32, (gap_seed >> 32n) & 0xFFFFFFFFn));
            const gap_params = {
                ...params,
                quantOffsets: [
                    params.quantOffsets[0] + seedPart1,
                    params.quantOffsets[1] + seedPart2
                ]
            };
            return fpsr_qs_base_js(local_progress, gap_params);
        }
    };
    return executeWithLod(evalLod0, appFrame, lod);
}

function fpsr_bd_get_details(appFrame, params, fm, segBlockLength, varispeedHoldBlockCount, lod) {
    const evalLod0 = (f) => {
        const effectiveFm = (fm === 0.0) ? 1.0 : fm;
        const scaled_pos = Number(f) * effectiveFm;
        const master_frame = BigInt(Math.floor(scaled_pos));
        const master_start = BigInt(Math.ceil(Number(master_frame) / effectiveFm));
        const gap = f - master_start;
        const segLen = BigInt(segBlockLength);
        const seg_idx = segLen > 0n ? gap / segLen : 0n;
        const holdCount = BigInt(varispeedHoldBlockCount);

        // Execute Unified Continuum Logic (Anchor Persistence vs Telescopic Extension)
        if (holdCount < 0n || seg_idx < holdCount) {
            // MODE 1: "Tape Varispeed" (Anchor)
            return fpsr_bd_base_js(master_frame, params);
        } else {
            // MODE 2: "Telescopic Extension" (Generative Phrase)
            const local_progress = segLen > 0n ? (gap % segLen) : 0n;
            const gap_params = { ...params, valueSeedOffset: splitmix64(master_frame + seg_idx) };
            return fpsr_bd_base_js(local_progress, gap_params);
        }
    };
    return executeWithLod(evalLod0, appFrame, lod);
}

// --- Stateful Legacy Model Persistent Variables ---
// Moved here from fpsr_visualiser.html, where these four variables lived
// inside the DOMContentLoaded closure and were reset from updateParams(),
// rewindAnimation() and resetStatefulWarmup(). They now live in this one
// object so the library owns the state; call fpsr_legacy_reset() to reset
// them exactly as the visualiser used to (see those call sites).
const fpsrLegacyState = {
    aNextJumpFrame: 0,
    aHeldValue: Math.random(),
    bLastJumpFrame: 0,
    bHeldValue: Math.random()
};

function fpsr_legacy_reset() {
    fpsrLegacyState.aNextJumpFrame = 0;
    fpsrLegacyState.aHeldValue = Math.random();
    fpsrLegacyState.bLastJumpFrame = 0;
    fpsrLegacyState.bHeldValue = Math.random();
}

// --- Legacy Paradigm A: Pre-Calculated Accumulator ---
function fpsr_legacy_a_base_js(frame, params) {
    frame = Number(frame);
    const minHold = params.minHold || 10;
    const maxHold = params.maxHold || 60;
    const variance = Math.max(0, maxHold - minHold);

    if (frame >= fpsrLegacyState.aNextJumpFrame) {
        fpsrLegacyState.aHeldValue = Math.random();
        const randomDuration = minHold + Math.floor(Math.random() * (variance + 1));
        fpsrLegacyState.aNextJumpFrame = frame + randomDuration;
    }
    return fpsrLegacyState.aHeldValue;
}

// --- Legacy Paradigm B: Continuous Coin-Flip ---
function fpsr_legacy_b_base_js(frame, params) {
    frame = Number(frame);
    const minHold = params.minHold || 10;
    const probability = params.threshold || 0.10;

    // Stateful check: "Are we there yet?" Tier 1 & Tier 2 evaluation
    if ((frame - fpsrLegacyState.bLastJumpFrame) > minHold) {
        if (Math.random() < probability) {
            // Tier 3: Jump granted, write-cycles to memory
            fpsrLegacyState.bHeldValue = Math.random();
            fpsrLegacyState.bLastJumpFrame = frame;
        }
    }
    return fpsrLegacyState.bHeldValue;
}

// --- Stateless Spatial Noise 1: 1D Perlin Noise (fBm) ---
function perlin1D_grad(intCoord) {
    // Hashing integer coordinate to deterministic gradient [-1, 1]
    const h = splitmix64(BigInt(Math.floor(intCoord)) + 0xB5526C09n);
    return (Number(h & 0xFFFFn) / 32767.5) - 1.0;
}

function perlin1D_single(x) {
    const i = Math.floor(x);
    const f = x - i;
    // Quintic fade polynomial: 6t^5 - 15t^4 + 10t^3
    const s = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    const g0 = perlin1D_grad(i);
    const g1 = perlin1D_grad(i + 1);
    const d0 = g0 * f;
    const d1 = g1 * (f - 1.0);
    return d0 + s * (d1 - d0);
}

function fpsr_perlin_base_js(frame, params) {
    const octaves = params.octaves;
    if (octaves === 0) return 0.5;

    const baseFreq = params.baseFreq || 0.015;
    const freqMult = params.freqMult || 2.0;
    const octaveOffset = params.octaveOffset || 53.1;
    const persistence = params.persistence || 0.5;
    const quantSteps = Math.max(2, params.quantSteps || 8);
    const finalRandSwitch = params.finalRandSwitch || 0;

    let total = 0.0;
    let maxAmp = 0.0;
    let currentAmp = 1.0;
    let currentFreq = baseFreq;

    for (let o = 0; o < octaves; o++) {
        const samplePos = Number(frame) * currentFreq + (o * octaveOffset);
        total += perlin1D_single(samplePos) * currentAmp;
        maxAmp += currentAmp;
        currentAmp *= persistence;
        currentFreq *= freqMult;
    }

    // Normalize from [-maxAmp, maxAmp] to [0, 1]
    let normalized = (total / (maxAmp * 0.7) + 1.0) * 0.5;
    normalized = Math.max(0.0, Math.min(1.0, normalized));

    // The Crushed Gradient: active suppression via quantization
    const quantized = Math.floor(normalized * quantSteps) / (quantSteps - 1);

    if (finalRandSwitch) {
        const seed = BigInt(Math.floor(quantized * 1000000));
        return portable_rand_u64(seed);
    }
    return Math.max(0.0, Math.min(1.0, quantized));
}

// --- Stateless Spatial Noise 2: 1D Worley / Cellular Noise (fBm) ---
function worley1D_cell_point(cellIndex) {
    // Random position of the feature point inside cell [cellIndex, cellIndex + 1]
    const h = splitmix64(BigInt(Math.floor(cellIndex)) + 0x68E31DA4n);
    return Number(h & 0xFFFFn) / 65535.0;
}

function worley1D_single(x, metric) {
    const cell = Math.floor(x);
    let d1 = 999.0;
    let d2 = 999.0;

    // Inspect neighboring 1D cells
    for (let offset = -1; offset <= 2; offset++) {
        const neighborCell = cell + offset;
        const featurePos = neighborCell + worley1D_cell_point(neighborCell);
        const dist = Math.abs(x - featurePos);

        if (dist < d1) {
            d2 = d1;
            d1 = dist;
        } else if (dist < d2) {
            d2 = dist;
        }
    }

    let value = 0.0;
    if (metric === 'F2') {
        value = d2;
    } else if (metric === 'F2_minus_F1') {
        value = d2 - d1;
    } else {
        value = d1; // default 'F1'
    }
    return Math.max(0.0, Math.min(1.0, value));
}

function fpsr_worley_base_js(frame, params) {
    const octaves = params.octaves;
    if (octaves === 0) return 0.5;

    const baseFreq = params.baseFreq || 0.02;
    const freqMult = params.freqMult || 2.0;
    const octaveOffset = params.octaveOffset || 79.3;
    const persistence = params.persistence || 0.5;
    const metric = params.metric || 'F1';
    const quantSteps = Math.max(2, params.quantSteps || 8);
    const finalRandSwitch = params.finalRandSwitch || 0;

    let total = 0.0;
    let maxAmp = 0.0;
    let currentAmp = 1.0;
    let currentFreq = baseFreq;

    for (let o = 0; o < octaves; o++) {
        const samplePos = Number(frame) * currentFreq + (o * octaveOffset);
        total += worley1D_single(samplePos, metric) * currentAmp;
        maxAmp += currentAmp;
        currentAmp *= persistence;
        currentFreq *= freqMult;
    }

    let normalized = maxAmp > 0 ? (total / maxAmp) : 0.0;
    normalized = Math.max(0.0, Math.min(1.0, normalized));

    // The Crushed Gradient: active suppression via quantization
    const quantized = Math.floor(normalized * quantSteps) / (quantSteps - 1);

    if (finalRandSwitch) {
        const seed = BigInt(Math.floor(quantized * 1000000));
        return portable_rand_u64(seed);
    }
    return Math.max(0.0, Math.min(1.0, quantized));
}

// --- Unified Evaluation Dispatcher ---
// Dispatches to the respective algorithm's wrapper (with integrated HPQ + LOD)
function getFpsrDetails(appFrame, activeAlgorithm, globalParams) {
    const fm = globalParams.frameMultiplier;
    const segLen = globalParams.segBlockLength;
    const holdCount = globalParams.varispeedHoldBlockCount;

    switch (activeAlgorithm) {
        case 'SM':
            return fpsr_sm_get_details(appFrame, globalParams.sm, fm, segLen, holdCount, globalParams.sm.lod);
        case 'TM':
            return fpsr_tm_get_details(appFrame, globalParams.tm, fm, segLen, holdCount, globalParams.tm.lod);
        case 'QS':
            return fpsr_qs_get_details(appFrame, globalParams.qs, fm, segLen, holdCount, globalParams.qs.lod);
        case 'BD':
            return fpsr_bd_get_details(appFrame, globalParams.bd, fm, segLen, holdCount, globalParams.bd.lod);
        case 'LEGACY_A':
            return { randVal: fpsr_legacy_a_base_js(appFrame, globalParams.st_a) };
        case 'LEGACY_B':
            return { randVal: fpsr_legacy_b_base_js(appFrame, globalParams.st_b) };
        case 'PERLIN':
            return { randVal: fpsr_perlin_base_js(appFrame, globalParams.perlin) };
        case 'WORLEY':
            return { randVal: fpsr_worley_base_js(appFrame, globalParams.worley) };
        default:
            return { randVal: 0.5 };
    }
}

// Extract the scalar value for visualizer animation & canvas plotting
function getFpsrValue(appFrame, activeAlgorithm, globalParams) {
    const details = getFpsrDetails(appFrame, activeAlgorithm, globalParams);
    return details.randVal;
}
