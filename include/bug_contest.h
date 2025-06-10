#ifndef GUARD_BUG_CONTEST_H
#define GUARD_BUG_CONTEST_H

#include "global.h"

#define BUG_CONTEST_TIME_LIMIT_FRAMES (60 * 60 * 1) // 8 minutes at 60 FPS

void EnterBugContestMode(void);
void ExitBugContestMode(void);
bool8 BugContestCheckTimeLimit(void);
bool32 GetBugContestFlag(void);
bool8 JudgeBugContestMon(void);
bool8 TransferBugContestMon(void);

void BugContestRetirePrompt(void);

extern const u8 BugContest_EventScript_TimesUp[];

#endif // GUARD_BUG_CONTEST_H
