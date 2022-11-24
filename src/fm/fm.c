#include "fm.h"

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../concurrency/interconnect.h"
#include "../concurrency/worker.h"
#include "../dsp/fir_filter.h"
#include "../dsp/resampler.h"
#include "../io/io.h"
#include "../types/iq.h"
#include "../types/real.h"

static const float FIR_FILT_250kFS_15kPA_19kST[] = {
    0.000044219730495340614f, 0.00009440017413002246f, 0.00018365481566381955f,
    0.000314099657926496f,    0.0004888737347517136f,  0.0007052772638324323f,
    0.0009530618154668994f,   0.001213501937310202f,   0.0014596543220620815f,
    0.0016580911975254515f,   0.001772185264006052f,   0.0017667596615661733f,
    0.0016136092510598552f,   0.001297190984372922f,   0.0008195541472049096f,
    0.00020354235282485188f,  -0.0005066031610256816f, -0.0012477912293477878f,
    -0.001943943396882769f,   -0.00251478878088645f,   -0.0028866415213852177f,
    -0.0030037479174223553f,  -0.0028384913862145285f, -0.0023986640082025623f,
    -0.0017302297502465027f,  -0.0009145766054104768f, -0.00006000902620814281f,
    0.0007118808025280118f,   0.001283360665884154f,   0.0015588594476816553f,
    0.001481605191451303f,    0.0010457625891767785f,  0.000301574090541716f,
    -0.0006479550570090435f,  -0.0016591971640395236f, -0.0025677034644882584f,
    -0.0032131562544198277f,  -0.003465828593184935f,  -0.0032503729137945982f,
    -0.002562689289902561f,   -0.0014764994872995415f, -0.00013761807392919704f,
    0.0012542869500616768f,   0.0024743875585363107f,  0.0033080964852317337f,
    0.0035876984968087194f,   0.0032243131182875307f,  0.0022293060231395614f,
    0.0007207561105883507f,   -0.0010874563644707642f, -0.0029135440066924215f,
    -0.004449451207866056f,   -0.005410406821700571f,  -0.005584735973844371f,
    -0.004875586071167914f,   -0.0033268844512126033f, -0.0011278522188292914f,
    0.0014064950686490984f,   0.00387759706914003f,    0.005863192233064355f,
    0.0069862331966464705f,   0.006982135774058983f,   0.005753020181664978f,
    0.00339881578041101f,     0.00021810770475836572f, -0.0033240087439066236f,
    -0.006658563768166884f,   -0.009198145015204788f,  -0.010433066286096221f,
    -0.010023483549336699f,   -0.00787209465450969f,   -0.004163924710020362f,
    0.0006360702272000808f,   0.005831057067516668f,   0.010582242431390191f,
    0.014030521700937876f,    0.01543312771302555f,    0.01429615792087495f,
    0.010481576834571455f,    0.0042709935291500465f,  -0.003627267347436748f,
    -0.012133396981147423f,   -0.019910142427153145f,  -0.025523793696773867f,
    -0.027632445479650606f,   -0.025176691782452826f,  -0.017546427721962674f,
    -0.004699384557322631f,   0.012787565557979339f,   0.033744553187915624f,
    0.056512266250085125f,    0.07911237948405572f,    0.09946490871347209f,
    0.11562550933926899f,     0.12601248946447888f,    0.1295942215312001f,
    0.12601248946447888f,     0.11562550933926899f,    0.09946490871347209f,
    0.07911237948405572f,     0.056512266250085125f,   0.033744553187915624f,
    0.012787565557979339f,    -0.004699384557322631f,  -0.017546427721962674f,
    -0.025176691782452826f,   -0.027632445479650606f,  -0.025523793696773867f,
    -0.019910142427153145f,   -0.012133396981147423f,  -0.003627267347436748f,
    0.0042709935291500465f,   0.010481576834571455f,   0.01429615792087495f,
    0.01543312771302555f,     0.014030521700937876f,   0.010582242431390191f,
    0.005831057067516668f,    0.0006360702272000808f,  -0.004163924710020362f,
    -0.00787209465450969f,    -0.010023483549336699f,  -0.010433066286096221f,
    -0.009198145015204788f,   -0.006658563768166884f,  -0.0033240087439066236f,
    0.00021810770475836572f,  0.00339881578041101f,    0.005753020181664978f,
    0.006982135774058983f,    0.0069862331966464705f,  0.005863192233064355f,
    0.00387759706914003f,     0.0014064950686490984f,  -0.0011278522188292914f,
    -0.0033268844512126033f,  -0.004875586071167914f,  -0.005584735973844371f,
    -0.005410406821700571f,   -0.004449451207866056f,  -0.0029135440066924215f,
    -0.0010874563644707642f,  0.0007207561105883507f,  0.0022293060231395614f,
    0.0032243131182875307f,   0.0035876984968087194f,  0.0033080964852317337f,
    0.0024743875585363107f,   0.0012542869500616768f,  -0.00013761807392919704f,
    -0.0014764994872995415f,  -0.002562689289902561f,  -0.0032503729137945982f,
    -0.003465828593184935f,   -0.0032131562544198277f, -0.0025677034644882584f,
    -0.0016591971640395236f,  -0.0006479550570090435f, 0.000301574090541716f,
    0.0010457625891767785f,   0.001481605191451303f,   0.0015588594476816553f,
    0.001283360665884154f,    0.0007118808025280118f,  -0.00006000902620814281f,
    -0.0009145766054104768f,  -0.0017302297502465027f, -0.0023986640082025623f,
    -0.0028384913862145285f,  -0.0030037479174223553f, -0.0028866415213852177f,
    -0.00251478878088645f,    -0.001943943396882769f,  -0.0012477912293477878f,
    -0.0005066031610256816f,  0.00020354235282485188f, 0.0008195541472049096f,
    0.001297190984372922f,    0.0016136092510598552f,  0.0017667596615661733f,
    0.001772185264006052f,    0.0016580911975254515f,  0.0014596543220620815f,
    0.001213501937310202f,    0.0009530618154668994f,  0.0007052772638324323f,
    0.0004888737347517136f,   0.000314099657926496f,   0.00018365481566381955f,
    0.00009440017413002246f,  0.000044219730495340614f};
static const float FIR_FILT_250kFS_100kPA_105kST[] = {
    0.006070605487454018f,    0.01443402125039274f,
    0.003957176954417116f,    -0.006535244989599065f,
    0.0031187571195364274f,   0.0008788288645458518f,
    -0.003152063493691762f,   0.0033074824188423385f,
    -0.0019563566347321594f,  -0.0000025383003585240167f,
    0.0016799331349582338f,   -0.002575570344332358f,
    0.00241397113454167f,     -0.001397456697452616f,
    -0.00011472040180456252f, 0.0015983789247068835f,
    -0.002512407806746633f,   0.0025711208951452666f,
    -0.0017085805309174612f,  0.0001728688337901377f,
    0.001522693696331991f,    -0.0027756503415741125f,
    0.003139961991236923f,    -0.0023877168791097054f,
    0.0006911070754891852f,   0.0013954938375801867f,
    -0.0031639324578624805f,  0.003946348838187892f,
    -0.003370945227201318f,   0.0015043655171701233f,
    0.0010929587055851092f,   -0.003540088206073499f,
    0.004936743470639754f,    -0.004686088584383647f,
    0.0027040886187351806f,   0.0004727755075597306f,
    -0.0037923630167455685f,  0.006069663758519273f,
    -0.00637512086870205f,    0.004401003996342946f,
    -0.0006286072117454427f,  -0.0037692431030611163f,
    0.007271847207576821f,    -0.008509681144083387f,
    0.006790694495379134f,    -0.0024281973430471883f,
    -0.0032954149989525695f,  0.008471502086090477f,
    -0.011181525650613497f,   0.01015676206577768f,
    -0.005315208528685315f,   -0.00205813889816767f,
    0.009619887176772717f,    -0.01466953669794694f,
    0.015051233680384236f,    -0.00998261729272495f,
    0.0005146939348710277f,   0.01060827917417486f,
    -0.019635329327555812f,   0.02294867703898887f,
    -0.01832826815369223f,    0.00592488168896409f,
    0.011383598799338635f,    -0.028448255024472447f,
    0.03904460237343456f,     -0.03753170300608063f,
    0.02052489922392698f,     0.011868484728511467f,
    -0.055678532884019305f,   0.10370293145139495f,
    -0.1470442752839363f,     0.1771743251172417f,
    0.8120392115232855f,      0.1771743251172417f,
    -0.1470442752839363f,     0.10370293145139495f,
    -0.055678532884019305f,   0.011868484728511467f,
    0.02052489922392698f,     -0.03753170300608063f,
    0.03904460237343456f,     -0.028448255024472447f,
    0.011383598799338635f,    0.00592488168896409f,
    -0.01832826815369223f,    0.02294867703898887f,
    -0.019635329327555812f,   0.01060827917417486f,
    0.0005146939348710277f,   -0.00998261729272495f,
    0.015051233680384236f,    -0.01466953669794694f,
    0.009619887176772717f,    -0.00205813889816767f,
    -0.005315208528685315f,   0.01015676206577768f,
    -0.011181525650613497f,   0.008471502086090477f,
    -0.0032954149989525695f,  -0.0024281973430471883f,
    0.006790694495379134f,    -0.008509681144083387f,
    0.007271847207576821f,    -0.0037692431030611163f,
    -0.0006286072117454427f,  0.004401003996342946f,
    -0.00637512086870205f,    0.006069663758519273f,
    -0.0037923630167455685f,  0.0004727755075597306f,
    0.0027040886187351806f,   -0.004686088584383647f,
    0.004936743470639754f,    -0.003540088206073499f,
    0.0010929587055851092f,   0.0015043655171701233f,
    -0.003370945227201318f,   0.003946348838187892f,
    -0.0031639324578624805f,  0.0013954938375801867f,
    0.0006911070754891852f,   -0.0023877168791097054f,
    0.003139961991236923f,    -0.0027756503415741125f,
    0.001522693696331991f,    0.0001728688337901377f,
    -0.0017085805309174612f,  0.0025711208951452666f,
    -0.002512407806746633f,   0.0015983789247068835f,
    -0.00011472040180456252f, -0.001397456697452616f,
    0.00241397113454167f,     -0.002575570344332358f,
    0.0016799331349582338f,   -0.0000025383003585240167f,
    -0.0019563566347321594f,  0.0033074824188423385f,
    -0.003152063493691762f,   0.0008788288645458518f,
    0.0031187571195364274f,   -0.006535244989599065f,
    0.003957176954417116f,    0.01443402125039274f,
    0.006070605487454018f};
static const float FIR_FILT_250kFS_21kST_38kPA_55kST[] = {
    6.372625261784849e-7F,      -0.0000023024556718233633f,
    -4.955085473896705e-7F,     0.0000012065198402227953f,
    -0.0000031904774079180788f, -0.000018844949449040948f,
    -0.000023982610243116078f,  0.00001198073269278318f,
    0.00008084345551124371f,    0.00010510994277804214f,
    0.000008510412077472417f,   -0.00016792305685592828f,
    -0.0002493860330620959f,    -0.00009632003311845607f,
    0.00020385344974085276f,    0.0003700034903965011f,
    0.00021375701782342507f,    -0.00013246674650206112f,
    -0.00033650257817311927f,   -0.00022950775173396636f,
    0.000020670892438986945f,   0.00013549448398791816f,
    0.00006346107053799506f,    -0.000003992426572148535f,
    0.00006708169978984343f,    0.0001609078217517942f,
    0.00009346392299009918f,    -0.00010399818505168195f,
    -0.00021082658173311286f,   -0.00011956145714530652f,
    0.000023493940503079483f,   0.0000390048886658014f,
    -0.000030038528491537682f,  -0.000005832256905992838f,
    0.00013223739938958323f,    0.00019697210293003826f,
    0.00007004274887078521f,    -0.00010998063553902543f,
    -0.00013908821716315356f,   -0.00003860265461811551f,
    -0.000005319753887857383f,  -0.00009633826135205702f,
    -0.00013373039991372264f,   0.000014017146841197999f,
    0.0002032789698620751f,     0.0002042027217593962f,
    0.0000361591287292477f,     -0.00006342608260836195f,
    0.0000015103206152972877f,  0.0000474275665378305f,
    -0.0000843601673782699f,    -0.00024922927894837363f,
    -0.00019476843304410652f,   0.00004992388020652069f,
    0.00019474874471710579f,    0.0001085951706026328f,
    0.0000016305502266895017f,  0.00008088342954247259f,
    0.00020779392828737222f,    0.00010514275841150396f,
    -0.00018796919663842943f,   -0.00032810217848227506f,
    -0.00016071429517206865f,   0.00005148271106219351f,
    0.00003138764541190902f,    -0.00008885731668892842f,
    0.000009440048946413767f,   0.000293811452466387f,
    0.00037746558693823906f,    0.00009498669840985558f,
    -0.0002210056453983601f,    -0.0002152595023057884f,
    -0.00002840166799152342f,   -0.000050712286399786155f,
    -0.00027998544976947265f,   -0.000294664278222034f,
    0.00007985121577270399f,    0.00044046947034842f,
    0.0003645255259516175f,     0.000030772161156119065f,
    -0.00006425373901176229f,   0.00012274411564784884f,
    0.00012469731985443057f,    -0.00026277706318130595f,
    -0.0005834297174204423f,    -0.00036606510560812825f,
    0.00014568712688756342f,    0.0003278975034709589f,
    0.00009786473491516411f,    0.000001811910450988123f,
    0.0003117388744330201f,     0.0005409909055452476f,
    0.00018084164554648473f,    -0.00045339362621575006f,
    -0.0006165617748224016f,    -0.00021911610409554986f,
    0.00006435589289909395f,    -0.000140521548769928f,
    -0.000318978092475863f,     0.00009371364897661076f,
    0.0007314576008276356f,     0.0007532565734807366f,
    0.00010201751845910428f,    -0.0003784555466568918f,
    -0.0002003379013458035f,    0.00006260690092142204f,
    -0.00026029750367074725f,   -0.0007914797719025878f,
    -0.00061880788035831f,      0.0002623415801381104f,
    0.0008313047643417132f,     0.0005135610395573751f,
    0.000003190559988098336f,   0.00013763914671806204f,
    0.0005545854774803395f,     0.0002643165244137004f,
    -0.0007043157517802343f,    -0.0011826811817652232f,
    -0.0005609897079369032f,    0.0002811445610345171f,
    0.00030020751103341213f,    -0.00014142074584196494f,
    0.00007738734078091856f,    0.0009490059257154622f,
    0.001199445602340429f,      0.00022511801612684293f,
    -0.0008752565144924198f,    -0.0008596876374668404f,
    -0.0001561986910964127f,    -0.00010633876744791365f,
    -0.0007844129884380372f,    -0.0008276377730683978f,
    0.0003683260742617793f,     0.001507109183281324f,
    0.0012026819615980537f,     0.000038318720704889805f,
    -0.0003426356108560383f,    0.00023676477473746245f,
    0.00028250168125244096f,    -0.0008761281445504856f,
    -0.0018074832050384668f,    -0.0010482696449876224f,
    0.0005943987062490147f,     0.0011470738579058982f,
    0.0003802483750172522f,     0.000025925129751830377f,
    0.0009195415846596116f,     0.0015476941630354648f,
    0.0003921460973265136f,     -0.001512921281629606f,
    -0.0019058203164196092f,    -0.0006118392213295606f,
    0.00026832113721887366f,    -0.0003416164428786511f,
    -0.0008362028688887629f,    0.0004183156599159787f,
    0.0022319416628022747f,     0.002141882538880437f,
    0.00013915130516090567f,    -0.0012186854087201954f,
    -0.0006188379165634699f,    0.00013722790132697803f,
    -0.0008335466693175412f,    -0.0022848736838904987f,
    -0.001617517196304371f,     0.0009650692855304027f,
    0.0024509867143869936f,     0.0013906519231127f,
    -0.00004498490591557199f,   0.0004249472286489817f,
    0.001548477442035727f,      0.0005604997166238044f,
    -0.0021826236024929733f,    -0.003306025248044724f,
    -0.001372573772749671f,     0.000910683967020684f,
    0.000786578766093259f,      -0.000421410112301318f,
    0.00036886342898768005f,    0.002790858650396172f,
    0.003218941705400202f,      0.0003357367573319676f,
    -0.002542491801391238f,     -0.0022298351239940072f,
    -0.0003113190121988565f,    -0.0004139397414231794f,
    -0.0023002789148580667f,    -0.002124979695704227f,
    0.0013287414400432083f,     0.004196869134585856f,
    0.0030233300625580645f,     -0.0000986389065495893f,
    -0.0007959935366315361f,    0.0008485061054183475f,
    0.0006383799791334561f,     -0.0027111902835302865f,
    -0.0049386229223364235f,    -0.002488803544004603f,
    0.0018533143502591843f,     0.002900131805217405f,
    0.0007301997061303493f,     0.0001828000260336144f,
    0.002857350321140492f,      0.0042131277598890056f,
    0.000648293923041688f,      -0.004340008349110046f,
    -0.004854365938129485f,     -0.001265721512564019f,
    0.0005885464958022085f,     -0.0013889163128345805f,
    -0.0023204339669092512f,    0.0016034455955244332f,
    0.006325874079622277f,      0.005442305166252038f,
    -0.00007245379208335314f,   -0.0031132960605019886f,
    -0.0010972077476938207f,    0.0004508049046423156f,
    -0.002848789633955633f,     -0.006597666981609202f,
    -0.004017930018145977f,     0.003153452465818779f,
    0.006479298328722489f,      0.0031419087414619714f,
    -0.00016987433285016202f,   0.0019203657784312204f,
    0.004761275793791812f,      0.0010813513265368069f,
    -0.00671770870923116f,      -0.009000203569239218f,
    -0.003105651154804102f,     0.0025480969335165368f,
    0.0012913420990534788f,     -0.001740647264010951f,
    0.0016992635631616303f,     0.00885997098284537f,
    0.009068611973065757f,      0.00013610797212273718f,
    -0.007369484737430418f,     -0.005460904422474f,
    -0.00038403161115767754f,   -0.002163829210453646f,
    -0.008035814745812213f,     -0.006189232492983325f,
    0.005094993514014465f,      0.012915134136050888f,
    0.008192830427208019f,      -0.0007739939441714864f,
    -0.0012255612720831653f,    0.004127816036932634f,
    0.0016593117193063117f,     -0.010334992873556692f,
    -0.016549240728540027f,     -0.007052086715732165f,
    0.006731714643592299f,      0.008354754953263216f,
    0.0009436303337028137f,     0.0014760812242828284f,
    0.012519060064745683f,      0.015971388748150583f,
    0.0008301129030823552f,     -0.01725296370174855f,
    -0.017087163505340213f,     -0.0033564058262300317f,
    0.0008779264380816006f,     -0.009013003831980265f,
    -0.010600009430175002f,     0.00961027240547274f,
    0.030352340755610347f,      0.02370377177963889f,
    -0.0022617782987756707f,    -0.013429716157430218f,
    -0.0013572983318934583f,    0.002757525590580179f,
    -0.021400537403651384f,     -0.04386808634834941f,
    -0.023652916024883345f,     0.02532030997827304f,
    0.045540274470392404f,      0.01916067250717576f,
    -0.00031920401897420753f,   0.030685755619438086f,
    0.06285039369625281f,       0.007342286608217937f,
    -0.12381916262265824f,      -0.18909794525284523f,
    -0.0752539024465157f,       0.1415108114016382f,
    0.2515098277982166f,        0.1415108114016382f,
    -0.0752539024465157f,       -0.18909794525284523f,
    -0.12381916262265824f,      0.007342286608217937f,
    0.06285039369625281f,       0.030685755619438086f,
    -0.00031920401897420753f,   0.01916067250717576f,
    0.045540274470392404f,      0.02532030997827304f,
    -0.023652916024883345f,     -0.04386808634834941f,
    -0.021400537403651384f,     0.002757525590580179f,
    -0.0013572983318934583f,    -0.013429716157430218f,
    -0.0022617782987756707f,    0.02370377177963889f,
    0.030352340755610347f,      0.00961027240547274f,
    -0.010600009430175002f,     -0.009013003831980265f,
    0.0008779264380816006f,     -0.0033564058262300317f,
    -0.017087163505340213f,     -0.01725296370174855f,
    0.0008301129030823552f,     0.015971388748150583f,
    0.012519060064745683f,      0.0014760812242828284f,
    0.0009436303337028137f,     0.008354754953263216f,
    0.006731714643592299f,      -0.007052086715732165f,
    -0.016549240728540027f,     -0.010334992873556692f,
    0.0016593117193063117f,     0.004127816036932634f,
    -0.0012255612720831653f,    -0.0007739939441714864f,
    0.008192830427208019f,      0.012915134136050888f,
    0.005094993514014465f,      -0.006189232492983325f,
    -0.008035814745812213f,     -0.002163829210453646f,
    -0.00038403161115767754f,   -0.005460904422474f,
    -0.007369484737430418f,     0.00013610797212273718f,
    0.009068611973065757f,      0.00885997098284537f,
    0.0016992635631616303f,     -0.001740647264010951f,
    0.0012913420990534788f,     0.0025480969335165368f,
    -0.003105651154804102f,     -0.009000203569239218f,
    -0.00671770870923116f,      0.0010813513265368069f,
    0.004761275793791812f,      0.0019203657784312204f,
    -0.00016987433285016202f,   0.0031419087414619714f,
    0.006479298328722489f,      0.003153452465818779f,
    -0.004017930018145977f,     -0.006597666981609202f,
    -0.002848789633955633f,     0.0004508049046423156f,
    -0.0010972077476938207f,    -0.0031132960605019886f,
    -0.00007245379208335314f,   0.005442305166252038f,
    0.006325874079622277f,      0.0016034455955244332f,
    -0.0023204339669092512f,    -0.0013889163128345805f,
    0.0005885464958022085f,     -0.001265721512564019f,
    -0.004854365938129485f,     -0.004340008349110046f,
    0.000648293923041688f,      0.0042131277598890056f,
    0.002857350321140492f,      0.0001828000260336144f,
    0.0007301997061303493f,     0.002900131805217405f,
    0.0018533143502591843f,     -0.002488803544004603f,
    -0.0049386229223364235f,    -0.0027111902835302865f,
    0.0006383799791334561f,     0.0008485061054183475f,
    -0.0007959935366315361f,    -0.0000986389065495893f,
    0.0030233300625580645f,     0.004196869134585856f,
    0.0013287414400432083f,     -0.002124979695704227f,
    -0.0023002789148580667f,    -0.0004139397414231794f,
    -0.0003113190121988565f,    -0.0022298351239940072f,
    -0.002542491801391238f,     0.0003357367573319676f,
    0.003218941705400202f,      0.002790858650396172f,
    0.00036886342898768005f,    -0.000421410112301318f,
    0.000786578766093259f,      0.000910683967020684f,
    -0.001372573772749671f,     -0.003306025248044724f,
    -0.0021826236024929733f,    0.0005604997166238044f,
    0.001548477442035727f,      0.0004249472286489817f,
    -0.00004498490591557199f,   0.0013906519231127f,
    0.0024509867143869936f,     0.0009650692855304027f,
    -0.001617517196304371f,     -0.0022848736838904987f,
    -0.0008335466693175412f,    0.00013722790132697803f,
    -0.0006188379165634699f,    -0.0012186854087201954f,
    0.00013915130516090567f,    0.002141882538880437f,
    0.0022319416628022747f,     0.0004183156599159787f,
    -0.0008362028688887629f,    -0.0003416164428786511f,
    0.00026832113721887366f,    -0.0006118392213295606f,
    -0.0019058203164196092f,    -0.001512921281629606f,
    0.0003921460973265136f,     0.0015476941630354648f,
    0.0009195415846596116f,     0.000025925129751830377f,
    0.0003802483750172522f,     0.0011470738579058982f,
    0.0005943987062490147f,     -0.0010482696449876224f,
    -0.0018074832050384668f,    -0.0008761281445504856f,
    0.00028250168125244096f,    0.00023676477473746245f,
    -0.0003426356108560383f,    0.000038318720704889805f,
    0.0012026819615980537f,     0.001507109183281324f,
    0.0003683260742617793f,     -0.0008276377730683978f,
    -0.0007844129884380372f,    -0.00010633876744791365f,
    -0.0001561986910964127f,    -0.0008596876374668404f,
    -0.0008752565144924198f,    0.00022511801612684293f,
    0.001199445602340429f,      0.0009490059257154622f,
    0.00007738734078091856f,    -0.00014142074584196494f,
    0.00030020751103341213f,    0.0002811445610345171f,
    -0.0005609897079369032f,    -0.0011826811817652232f,
    -0.0007043157517802343f,    0.0002643165244137004f,
    0.0005545854774803395f,     0.00013763914671806204f,
    0.000003190559988098336f,   0.0005135610395573751f,
    0.0008313047643417132f,     0.0002623415801381104f,
    -0.00061880788035831f,      -0.0007914797719025878f,
    -0.00026029750367074725f,   0.00006260690092142204f,
    -0.0002003379013458035f,    -0.0003784555466568918f,
    0.00010201751845910428f,    0.0007532565734807366f,
    0.0007314576008276356f,     0.00009371364897661076f,
    -0.000318978092475863f,     -0.000140521548769928f,
    0.00006435589289909395f,    -0.00021911610409554986f,
    -0.0006165617748224016f,    -0.00045339362621575006f,
    0.00018084164554648473f,    0.0005409909055452476f,
    0.0003117388744330201f,     0.000001811910450988123f,
    0.00009786473491516411f,    0.0003278975034709589f,
    0.00014568712688756342f,    -0.00036606510560812825f,
    -0.0005834297174204423f,    -0.00026277706318130595f,
    0.00012469731985443057f,    0.00012274411564784884f,
    -0.00006425373901176229f,   0.000030772161156119065f,
    0.0003645255259516175f,     0.00044046947034842f,
    0.00007985121577270399f,    -0.000294664278222034f,
    -0.00027998544976947265f,   -0.000050712286399786155f,
    -0.00002840166799152342f,   -0.0002152595023057884f,
    -0.0002210056453983601f,    0.00009498669840985558f,
    0.00037746558693823906f,    0.000293811452466387f,
    0.000009440048946413767f,   -0.00008885731668892842f,
    0.00003138764541190902f,    0.00005148271106219351f,
    -0.00016071429517206865f,   -0.00032810217848227506f,
    -0.00018796919663842943f,   0.00010514275841150396f,
    0.00020779392828737222f,    0.00008088342954247259f,
    0.0000016305502266895017f,  0.0001085951706026328f,
    0.00019474874471710579f,    0.00004992388020652069f,
    -0.00019476843304410652f,   -0.00024922927894837363f,
    -0.0000843601673782699f,    0.0000474275665378305f,
    0.0000015103206152972877f,  -0.00006342608260836195f,
    0.0000361591287292477f,     0.0002042027217593962f,
    0.0002032789698620751f,     0.000014017146841197999f,
    -0.00013373039991372264f,   -0.00009633826135205702f,
    -0.000005319753887857383f,  -0.00003860265461811551f,
    -0.00013908821716315356f,   -0.00010998063553902543f,
    0.00007004274887078521f,    0.00019697210293003826f,
    0.00013223739938958323f,    -0.000005832256905992838f,
    -0.000030038528491537682f,  0.0000390048886658014f,
    0.000023493940503079483f,   -0.00011956145714530652f,
    -0.00021082658173311286f,   -0.00010399818505168195f,
    0.00009346392299009918f,    0.0001609078217517942f,
    0.00006708169978984343f,    -0.000003992426572148535f,
    0.00006346107053799506f,    0.00013549448398791816f,
    0.000020670892438986945f,   -0.00022950775173396636f,
    -0.00033650257817311927f,   -0.00013246674650206112f,
    0.00021375701782342507f,    0.0003700034903965011f,
    0.00020385344974085276f,    -0.00009632003311845607f,
    -0.0002493860330620959f,    -0.00016792305685592828f,
    0.000008510412077472417f,   0.00010510994277804214f,
    0.00008084345551124371f,    0.00001198073269278318f,
    -0.000023982610243116078f,  -0.000018844949449040948f,
    -0.0000031904774079180788f, 0.0000012065198402227953f,
    -4.955085473896705e-7F,     -0.0000023024556718233633f,
    6.372625261784849e-7F};

void convert_data_ftos(const float *input, int16_t *output, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    output[i] = input[i] * INT16_MAX;
  }
}

void deemphasis_filtering(const float *input, size_t input_len,
                          float **output, size_t *output_len,
                          float *prev_deemph_input,
                          float *prev_deemph_output) {
  //           w_ca         1                 1 - (-1) z^-1
  //    H(z) = ---- * ----------- * --------------------------------
  //           2 fs        -w_ca                   -w_ca
  //                   1 - -----               1 + -----
  //                        2 fs                    2 fs
  //                                     1 - -------------- z^-1
  //                                               -w_ca
  //                                           1 - -----
  //                                                2 fs
  //                               w_ca + w_ca z^-1
  //    H(z) = ------------------------------------------------------------
  //                                 (2 fs - w_ca) (w_ca - 2 fs)
  //                (2 fs - w_ca) + ------------------------------ z^-1
  //                                           2 fs + w_ca
  //
  //                Y(z)
  //    let H(z) = ------
  //                X(z)
  //
  //    Y(z) = w_ca + w_ca z^-1
  //
  //                            (2 fs - w_ca) (w_ca - 2 fs)
  //    X(z) = (2 fs - w_ca) + ------------------------------ z^-1
  //                                     2 fs + w_ca
  //
  //    let k_1 = w_ca
  //    let k_2 = 2 fs - w_ca
  //
  //                (2 fs - w_ca) (w_ca - 2 fs)
  //    let k_3 = ------------------------------
  //                         2 fs + w_ca
  //
  //    Y(z) = k_1 + k_1 z^-1
  //    X(x) = k_2 + k_3 z^-1
  //    (k_2 + k_3 z^-1) Y(z) = (k_1 + k_1 z^-1) X(z)
  //    y[n] = k_1/k_2 * x[n] + k_1/k_2 * x[n-1] - k_3/k_2 * y[n-1]
  //
  //    let k_4 = k_1/k_2
  //    let k_5 = k_3/k_2
  //
  //    let t = 75 us
  //    let w_ca = t^-1
  //    let fs = 44.1 kHz
  //
  //    k_4 = w_ca / (2 fs - w_ca)
  //    k_5 = (w_ca - 2 fs) / (2fs + w_ca)
  //
  static const float k_4 = 0.17809439002671415f;
  static const float k_5 = -0.7373604727511491f;

  *output_len = input_len;
  float *output_buffer = (float *)malloc(*output_len * sizeof(float));
  *output = output_buffer;

  for (size_t i = 0; i < *output_len; ++i) {
    output_buffer[i] =
        k_4 * input[i] + k_4 * *prev_deemph_input - k_5 * *prev_deemph_output;
    *prev_deemph_input = input[i];
    *prev_deemph_output = output_buffer[i];
  }
}

void polar_discriminant(const float complex *input, size_t input_len,
                        float **output, size_t *output_len,
                        float complex *prev_sample) {
  *output_len = input_len;
  float *output_buffer = (float *)malloc(*output_len * sizeof(float));
  *output = output_buffer;

  for (size_t i = 0; i < input_len; ++i) {
    output_buffer[i] = cargf(input[i] * conjf(*prev_sample)) / M_PI;
    *prev_sample = input[i];
  }
}

void extract_audio(const real_data_t *input, resampler_r_t *resampler,
                   float **output, size_t *output_len,
                   float *prev_deemph_input,
                   float *prev_deemph_output) {
  real_data_t audio_data;

  // Resample to 44.1kHz
  apply_resampler_r(resampler, input, &audio_data);

  // Apply deemphasis filter in order to compensate for the pre-emphasis
  // performed on the transmit side
  deemphasis_filtering(audio_data.samples, audio_data.num_samples, output,
                       output_len, prev_deemph_input, prev_deemph_output);

  destroy_real_data(&audio_data);
}

float goertzel_algorithm(uint64_t sample_rate_Hz,
                         uint64_t frequency_of_interest_Hz,
                         const float *const samples, size_t num_samples) {
  const float w = 2.0f * M_PI * frequency_of_interest_Hz / sample_rate_Hz;
  const float c_r = cosf(w);
  const float c_i = sinf(w);
  const float coeff = 2 * c_r;
  float s_prev = 0.0f;
  float s_prev2 = 0.0f;

  for (size_t i = 0; i < num_samples; ++i) {
    float s = samples[i] + coeff * s_prev - s_prev2;
    s_prev2 = s_prev;
    s_prev = s;
  }

  const float X_r = c_r * s_prev - s_prev2;
  const float X_i = c_i * s_prev;

  const float complex v = cexpf(-1 * I * w * num_samples) * (X_r + I * X_i);
  const float phi = cargf(v);

  return phi;
}

void extract_stereo_delta_audio(const real_data_t *demodulated_data,
                                fir_filter_r_t *filter,
                                resampler_r_t *resampler, float **output,
                                size_t *output_len,
                                float *prev_deemph_input,
                                float *prev_deemph_output) {
  const uint64_t pilot_tone_frequency_Hz = 19000;
  const uint64_t sample_rate_Hz = demodulated_data->sample_rate_Hz;
  const size_t num_input_samples = demodulated_data->num_samples;
  const float *const input_samples = demodulated_data->samples;

  real_data_t filtered_delta_channel_data = {
    .sample_rate_Hz = demodulated_data->sample_rate_Hz,
  };
  real_data_t demodulated_stereo_data = {
    .sample_rate_Hz = demodulated_data->sample_rate_Hz,
  };

  size_t num_carrier_samples = 0;
  float *carrier_samples = NULL;

  const float carrier_phase =
      goertzel_algorithm(sample_rate_Hz, pilot_tone_frequency_Hz, input_samples,
                         num_input_samples);

  apply_filter_r(filter, 1, input_samples, num_input_samples,
                 &filtered_delta_channel_data.samples,
                 &filtered_delta_channel_data.num_samples);

  num_carrier_samples = filtered_delta_channel_data.num_samples;
  carrier_samples = (float *)malloc(num_carrier_samples * sizeof(float));

  const uint64_t carrier_frequency_Hz = 38000;
  const float time_resolution_s = 1.0f / sample_rate_Hz;
  for (size_t i = 0; i < num_carrier_samples; ++i) {
    carrier_samples[i] =
        cosf(2.0f * M_PI * carrier_frequency_Hz *
                 (i + (num_input_samples - num_carrier_samples)) *
                 time_resolution_s +
             carrier_phase);
  }

  demodulated_stereo_data.num_samples = filtered_delta_channel_data.num_samples;
  demodulated_stereo_data.samples =
      (float *)malloc(demodulated_stereo_data.num_samples * sizeof(float));
  for (size_t i = 0; i < filtered_delta_channel_data.num_samples; ++i) {
    demodulated_stereo_data.samples[i] =
        carrier_samples[i] * filtered_delta_channel_data.samples[i];
  }

  extract_audio(&demodulated_stereo_data, resampler, output, output_len,
                prev_deemph_input, prev_deemph_output);

  free(carrier_samples);
  destroy_real_data(&demodulated_stereo_data);
  destroy_real_data(&filtered_delta_channel_data);
}

void normalise(float *const data, size_t len) {
  float abs_max = 0.0f;
  for (size_t i = 0; i < len; i++) {
    abs_max = fmaxf(fabsf(data[i]), abs_max);
  }
  if (abs_max != 0.0f) {
    for (size_t i = 0; i < len; i++) {
      data[i] /= abs_max;
    }
  }
}

void *demodulate_fm(void *args) {
  fm_demod_t *demod = (fm_demod_t *)args;

  worker_t *worker = &demod->worker;

  fir_filter_c_t *input_filter = &demod->input_filter;
  fir_filter_r_t *stereo_delta_channel_filter =
      &demod->stereo_delta_channel_filter;
  resampler_r_t *mono_audio_resampler = &demod->mono_audio_resampler;
  resampler_r_t *stereo_audio_resampler = &demod->stereo_audio_resampler;

  bpsk_demod_t *bpsk_demod = &demod->bpsk_demod;

  float complex *polar_discrim_prev_sample = &demod->polar_discrim_prev_sample;
  float *prev_mono_deemph_input = &demod->prev_mono_deemph_input;
  float *prev_mono_deemph_output = &demod->prev_mono_deemph_output;
  float *prev_stereo_deemph_input = &demod->prev_stereo_deemph_input;
  float *prev_stereo_deemph_output = &demod->prev_stereo_deemph_output;

  bool running = true;
  while (running) {
    iq_data_t *iq_data;
    iq_data_t filtered_iq_data;
    real_data_t demodulated_data;
    float *mono_audio = NULL;
    size_t num_mono_samples = 0;
    float *delta_audio = NULL;
    size_t num_delta_samples = 0;
    float *left_audio = NULL;
    float *right_audio = NULL;
    size_t num_interleaved_stereo_audio_samples = 0;
    float *interleaved_stereo_audio = NULL;
    int16_t *mono_audio_16 = NULL;
    int16_t *stereo_audio_16 = NULL;

    running = worker_read_input(worker, (void **)&iq_data);

    if (!running) {
      break;
    }

    if (iq_data->sample_rate_Hz != 250000) {
      fprintf(
          stderr,
          "Source IQ data sample rate is expected to be 250kHz, not %uHz!",
          iq_data->sample_rate_Hz); // TODO: Handle arbitrary input sample rates
    }

    // Filter the source IQ data using a LPF with a 100kHz cut-off
    filtered_iq_data.sample_rate_Hz = iq_data->sample_rate_Hz;
    apply_filter_c(input_filter, 1, iq_data->samples, iq_data->num_samples,
                   &filtered_iq_data.samples, &filtered_iq_data.num_samples);

    // Demodulate the FM signal
    demodulated_data.sample_rate_Hz = filtered_iq_data.sample_rate_Hz;
    polar_discriminant(filtered_iq_data.samples, filtered_iq_data.num_samples,
                       &demodulated_data.samples, &demodulated_data.num_samples,
                       polar_discrim_prev_sample);

    // write_data_to_file(demodulated_data.samples, demodulated_data.num_samples
    // * sizeof(*demodulated_data.samples), "demodulated_data.bin");
    demodulate_bpsk(bpsk_demod, &demodulated_data);

    // Extract the mono audio data
    extract_audio(&demodulated_data, mono_audio_resampler, &mono_audio,
                  &num_mono_samples, prev_mono_deemph_input,
                  prev_mono_deemph_output);

    // Extract the L-R stereo channel audio data
    extract_stereo_delta_audio(&demodulated_data, stereo_delta_channel_filter,
                               stereo_audio_resampler, &delta_audio,
                               &num_delta_samples, prev_stereo_deemph_input,
                               prev_stereo_deemph_output);

    // Compute the left channel stereo audio data
    left_audio = (float *)malloc(num_delta_samples * sizeof(float));
    for (size_t i = 0; i < num_delta_samples; ++i) {
      left_audio[i] = mono_audio[i] + delta_audio[i];
    }
    // normalise(left_audio, num_delta_samples);

    // Compute the right channel stereo audio data
    right_audio = (float *)malloc(num_delta_samples * sizeof(float));
    for (size_t i = 0; i < num_delta_samples; ++i) {
      right_audio[i] = mono_audio[i] - delta_audio[i];
    }
    // normalise(right_audio, num_delta_samples);

    // Interleave the left and right audio channel samples
    num_interleaved_stereo_audio_samples = 2 * num_delta_samples;
    interleaved_stereo_audio =
        (float *)malloc(num_interleaved_stereo_audio_samples * sizeof(float));
    for (size_t i = 0; i < num_delta_samples; ++i) {
      interleaved_stereo_audio[i * 2] = left_audio[i];
      interleaved_stereo_audio[i * 2 + 1] = right_audio[i];
    }

    // Convert the mono audio data from floats to 16bit integers
    mono_audio_16 = (int16_t *)malloc(num_mono_samples * sizeof(int16_t));
    convert_data_ftos(mono_audio, mono_audio_16, num_mono_samples);

    // Convert the stereo audio data from floats to 16bit integers
    stereo_audio_16 = (int16_t *)malloc(num_interleaved_stereo_audio_samples *
                                        sizeof(int16_t));
    convert_data_ftos(interleaved_stereo_audio, stereo_audio_16,
                      num_interleaved_stereo_audio_samples);

    // Write the mono and stereo audio data to separate output files
    write_data_to_file(mono_audio_16, num_mono_samples * sizeof(int16_t),
                       "mono_audio.bin");
    write_data_to_file(stereo_audio_16,
                       num_interleaved_stereo_audio_samples * sizeof(int16_t),
                       "stereo_audio.bin");

    free(stereo_audio_16);
    free(mono_audio_16);
    free(interleaved_stereo_audio);
    free(right_audio);
    free(left_audio);
    free(delta_audio);
    free(mono_audio);
    destroy_real_data(&demodulated_data);
    destroy_iq_data(&filtered_iq_data);
    destroy_iq_data(iq_data);
    free(iq_data);
  }

  return NULL;
}

void init_fm_demod(fm_demod_t *demod, interconnect_t *output) {
  demod->polar_discrim_prev_sample = 0.0f + 0.0f * I;
  demod->prev_mono_deemph_input = 0.0f;
  demod->prev_mono_deemph_output = 0.0f;
  demod->prev_stereo_deemph_input = 0.0f;
  demod->prev_stereo_deemph_output = 0.0f;

  init_filter_c(&demod->input_filter, FIR_FILT_250kFS_100kPA_105kST,
                sizeof(FIR_FILT_250kFS_100kPA_105kST) /
                    sizeof(*FIR_FILT_250kFS_100kPA_105kST));
  init_filter_r(&demod->stereo_delta_channel_filter,
                FIR_FILT_250kFS_21kST_38kPA_55kST,
                sizeof(FIR_FILT_250kFS_21kST_38kPA_55kST) /
                    sizeof(*FIR_FILT_250kFS_21kST_38kPA_55kST));

  demod->stereo_delta_channel_filter.delay_line_num_samples =
      demod->stereo_delta_channel_filter.num_taps - 1;

  init_resampler_r(&demod->mono_audio_resampler, 3, 17,
                   FIR_FILT_250kFS_15kPA_19kST,
                   sizeof(FIR_FILT_250kFS_15kPA_19kST) /
                       sizeof(*FIR_FILT_250kFS_15kPA_19kST));
  init_resampler_r(&demod->stereo_audio_resampler, 3, 17,
                   FIR_FILT_250kFS_15kPA_19kST,
                   sizeof(FIR_FILT_250kFS_15kPA_19kST) /
                       sizeof(*FIR_FILT_250kFS_15kPA_19kST));

  init_bpsk_demod(&demod->bpsk_demod);

  init_worker(&demod->worker, output, demodulate_fm, demod);
}

void destroy_fm_demod(fm_demod_t *demod) {
  destroy_worker(&demod->worker);

  destroy_bpsk_demod(&demod->bpsk_demod);

  destroy_filter_c(&demod->input_filter);
  destroy_filter_r(&demod->stereo_delta_channel_filter);
  destroy_resampler_r(&demod->mono_audio_resampler);
  destroy_resampler_r(&demod->stereo_audio_resampler);
}
