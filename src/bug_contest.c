// bug_contest.c
#include "global.h"
#include "script.h"
#include "event_data.h"
#include "fieldmap.h"
#include "overworld.h"
#include "bug_contest.h"
#include "constants/flags.h"
#include "constants/maps.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "pokemon.h"
#include "constants/species.h"
#include "item.h"
#include "battle.h"
#include "field_screen_effect.h"
#include "pokemon.h"
#include "string_util.h"
extern const struct SpeciesInfo gSpeciesInfo[];
static bool32 IsPlayerDefeated(u32 battleOutcome);
static u32 sBugContestStartTime;
static bool8 sBugContestTimerActive;


bool32 GetBugContestFlag(void)
{
    return FlagGet(FLAG_SYS_BUG_CONTEST_MODE);
}


void BugContestRetirePrompt(void)
{
    ScriptContext_SetupScript(BugContest_EventScript_TimesUp);
}

void EnterBugContestMode(void)
{
    FlagSet(FLAG_SYS_BUG_CONTEST_MODE);
    sBugContestStartTime = gMain.vblankCounter1;
    sBugContestTimerActive = TRUE;
}

void ExitBugContestMode(void)
{
    FlagClear(FLAG_SYS_BUG_CONTEST_MODE);
    sBugContestTimerActive = FALSE;
}

bool8 BugContestCheckTimeLimit(void)
{
    if (!FlagGet(FLAG_SYS_BUG_CONTEST_MODE) || !sBugContestTimerActive)
        return FALSE;

    u32 elapsed = gMain.vblankCounter1 - sBugContestStartTime;
    if (elapsed >= BUG_CONTEST_TIME_LIMIT_FRAMES) // 5 minutes by default
    {
        sBugContestTimerActive = FALSE;
        ScriptContext_SetupScript(BugContest_EventScript_TimesUp);
        return TRUE;
    }

    return FALSE;
}



bool8 TransferBugContestMon(void)
{
    u8 monIndex = VarGet(VAR_0x8004);
    struct Pokemon *mon = &gPlayerParty[monIndex];
    struct BoxPokemon *boxMon = &mon->box;
    u8 boxId, boxPos;

    if (StorePokemonInBox(boxMon, &boxId, &boxPos) != -1)
    {
        ZeroMonData(mon);  // Remove mon from party after storing
        CompactPartySlots();
        gSpecialVar_Result = MON_GIVEN_TO_PC;
    }
    else
    {
        gSpecialVar_Result = MON_CANT_GIVE;
    }

    return FALSE;
}



bool8 JudgeBugContestMon(void)
{
    //Scyther max hp: 53
        //low 48
    //Pinsir max hp: 51
        //low 46
    //Butterfree min hp:45
    //Beedrill min hp:44
    //Venonat min hp:40
    //metapod min hp:49
    //paras min hp:33

    u16 monIndex = VarGet(VAR_0x8004);
    u16 species = GetMonData(&gPlayerParty[monIndex], MON_DATA_SPECIES);
    u8 maxHP = GetMonData(&gPlayerParty[monIndex], MON_DATA_MAX_HP); //change to MON_DATA_HP for a more authentic johto experience
    u16 rand = Random() % 100;
    u16 placement;

    if (maxHP < 41)
    {
        gSpecialVar_Result = 3;
        placement = gSpecialVar_Result;
    }
    else if (maxHP <= 46)
    {
        gSpecialVar_Result = (rand < 50) ? 2 : 3;
        placement = gSpecialVar_Result;
    }
    else if (maxHP <= 47)
    {
        gSpecialVar_Result = (rand < 75) ? 1 : 2;
        placement = gSpecialVar_Result;
    }
    else // baseHP >= 48
    {
        gSpecialVar_Result = 1;
        placement = gSpecialVar_Result;
    }

        // Reward tables
    static const u16 sFirstPlaceRewards[]  = { ITEM_MOON_STONE, ITEM_SUN_STONE, ITEM_LEAF_STONE};
    static const u16 sSecondPlaceRewards[] = { ITEM_FIRE_STONE, ITEM_THUNDER_STONE, ITEM_WATER_STONE };
    static const u16 sThirdPlaceRewards[]  = {
        ITEM_ORAN_BERRY, ITEM_CHERI_BERRY, ITEM_PERSIM_BERRY,
        ITEM_PECHA_BERRY, ITEM_RAWST_BERRY, ITEM_ASPEAR_BERRY, ITEM_CHESTO_BERRY
    };

    switch (placement)
    {
    case 1:
        VarSet(VAR_0x8005, sFirstPlaceRewards[Random() % ARRAY_COUNT(sFirstPlaceRewards)]);
        break;
    case 2:
        VarSet(VAR_0x8005, sSecondPlaceRewards[Random() % ARRAY_COUNT(sSecondPlaceRewards)]);
        break;
    case 3:
        VarSet(VAR_0x8005, sThirdPlaceRewards[Random() % ARRAY_COUNT(sThirdPlaceRewards)]);
        break;
    default:
        VarSet(VAR_0x8005, ITEM_NONE);
        break;
    }

    return FALSE;
}


static bool32 IsPlayerDefeated(u32 battleOutcome)
{
    switch (battleOutcome)
    {
    case B_OUTCOME_LOST:
    case B_OUTCOME_DREW:
        return TRUE;
    case B_OUTCOME_WON:
    case B_OUTCOME_RAN:
    case B_OUTCOME_PLAYER_TELEPORTED:
    case B_OUTCOME_MON_FLED:
    case B_OUTCOME_CAUGHT:
        return FALSE;
    default:
        return FALSE;
    }
}

void CB2_EndBugContestBattle(void)
{
    CpuFill16(0, (void *)(BG_PLTT), BG_PLTT_SIZE);
    ResetOamRange(0, 128);
    u8 partyCount = 0;
    for (u8 i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
            partyCount++;
    }

    if (IsPlayerDefeated(gBattleOutcome) == TRUE)
    {
        SetMainCallback2(CB2_BugContestWhiteOut);
    }
    else if (gBattleOutcome == B_OUTCOME_CAUGHT)
    {

        if (partyCount == PARTY_SIZE)
        {
            ScriptContext_SetupScript(BugContest_EventScript_TimesUp); //change
        }
        SetMainCallback2(CB2_ReturnToField);
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
    }
    else
    {
        SetMainCallback2(CB2_ReturnToField);
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
    }
}

bool8 RemoveSafariBalls(void)
{
    u16 count = CountTotalItemQuantityInBag(ITEM_SAFARI_BALL);

    if (count > 0)
        RemoveBagItem(ITEM_SAFARI_BALL, count);

    return FALSE;
}

bool8 ShowBugContestChosenMon(void)
{
    u16 monIndex = VarGet(VAR_0x8004);
    u16 species = GetMonData(&gPlayerParty[monIndex], MON_DATA_SPECIES);

    // Set species name into STR_VAR_1 properly
    GetSpeciesName(gStringVar1, species);

    switch (species)
    {
    case SPECIES_CATERPIE:
        gSpecialVar_Result = 22;
        break;
    case SPECIES_WEEDLE:
        gSpecialVar_Result = 23;
        break;
    case SPECIES_METAPOD:
        gSpecialVar_Result = 24;
        break;
    case SPECIES_KAKUNA:
        gSpecialVar_Result = 25;
        break;
    case SPECIES_PARAS:
        gSpecialVar_Result = 26;
        break;
    case SPECIES_VENONAT:
        gSpecialVar_Result = 27;
        break;
    case SPECIES_BUTTERFREE:
        gSpecialVar_Result = 28;
        break;
    case SPECIES_BEEDRILL:
        gSpecialVar_Result = 29;
        break;
    case SPECIES_SCYTHER:
        gSpecialVar_Result = 30;
        break;
    case SPECIES_PINSIR:
        gSpecialVar_Result = 31;
        break;
    default:
        gSpecialVar_Result = 0; // fallback if invalid species
        break;
    }

    return FALSE;
}
