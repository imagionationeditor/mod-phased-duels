# Phased Duels Module - Bug Fixes Summary

This document summarizes all critical and major bugs that were fixed in the AzerothCore Phased Duels module.

## 🔴 Critical Bugs Fixed

### 1. Server Crash on Console Command Execution
**File:** `src/mod_phased_duels.cpp` (Line 44)

**Problem:** When admin executed `.duelranking reset` from server console (not in-game), `handler->GetSession()` returned `nullptr`, causing a crash when calling `->GetPlayerName()`.

**Fix:**
```cpp
std::string adminName = handler->GetSession() ? handler->GetSession()->GetPlayerName() : "Console";
LOG_WARN("module.phasedduels", "Duel rankings manually reset by admin: {}", adminName);
```

---

### 2. Rating Underflow (uint32 Wrap-around)
**File:** `src/mod_phased_duels.cpp` (Lines 92-93)

**Problem:** ELO calculation could produce negative values, but since `newWinnerRating` and `newLoserRating` were `uint32`, negative results would wrap around to ~4 billion, giving losing players astronomically high ratings instead of lowering them.

**Fix:**
```cpp
// Use int32 to prevent underflow
int32 newWinnerRating = static_cast<int32>(winnerRating) + static_cast<int32>(kFactorGain * (1.0f - expectedWinner));
int32 newLoserRating = static_cast<int32>(loserRating) + static_cast<int32>(kFactorLoss * (0.0f - expectedLoser));

// Clamp to minimum 0
newWinnerRating = std::max(0, newWinnerRating);
newLoserRating = std::max(0, newLoserRating);
```

---

### 3. Top 3 Ranking Logic Completely Broken
**File:** `src/mod_phased_duels.cpp` (Lines 114-140)

**Problem:** The code was querying `oldTop3` **after** updating the database, so both `oldTop3` and `newTop3` contained the same data. This made the ranking change detection useless - no announcements were ever sent.

**Fix:** Moved the `oldTop3` query to **before** the database UPDATE operations:
```cpp
// 1. Get old top 3 BEFORE update
QueryResult oldTop3Result = CharacterDatabase.Query("SELECT ...");

// 2. Update database
CharacterDatabase.Execute("INSERT ... ON DUPLICATE KEY UPDATE ...");

// 3. Get new top 3 AFTER update
QueryResult newTop3Result = CharacterDatabase.Query("SELECT ...");

// 4. Compare and announce
```

---

### 4. Early Return Preventing Pet Healing
**File:** `src/mod_phased_duels.cpp` (Lines 321-328)

**Problem:** When `RetorePowerForRogueOrWarrior.Enable = 0` and one player was Rogue/Warrior, the `return` statement exited the **entire** `OnPlayerDuelEnd` function, preventing pet revival/healing for BOTH players.

**Fix:** Replaced early returns with a flag-based skip:
```cpp
bool skipRogueWarrior = !sConfigMgr->GetOption<bool>("RetorePowerForRogueOrWarrior.Enable", true) &&
    (firstplayer->getClass() == CLASS_ROGUE || firstplayer->getClass() == CLASS_WARRIOR ||
     secondplayer->getClass() == CLASS_ROGUE || secondplayer->getClass() == CLASS_WARRIOR);

if (!skipRogueWarrior)
{
    // Restore power for both players
}
// Pet healing code continues to execute normally
```

---

### 5. Pet Healing Only Works When BOTH Have Pets
**File:** `src/mod_phased_duels.cpp` (Lines 339-340)

**Problem:** Condition `if (!pet1 || !pet2) return;` meant if only ONE player had a pet (e.g., Hunter vs Mage), NO pets were healed at all.

**Fix:** Handle each pet independently:
```cpp
if (pet1)
{
    // Revive/heal pet1
}
if (pet2)
{
    // Revive/heal pet2
}
```

---

## 🟠 Major Issues Fixed

### 6. Config Key Mismatch
**File:** `src/mod_phased_duels.cpp` (Line 298)

**Problem:** Code checked `"PhasedDueling.Enable"` but config file defined `"PhasedDuels.Enable"`. This meant the end-duel logic always ran with default `true` even if admin disabled it.

**Fix:** Changed to match config file:
```cpp
if (sConfigMgr->GetOption<bool>("PhasedDuels.Enable", true))
```

---

### 7. Missing `<cmath>` Include
**File:** `src/mod_phased_duels.cpp` (Line 1)

**Problem:** Used `pow()` function without explicitly including `<cmath>`. Worked only due to transitive includes, but not safe/portable.

**Fix:**
```cpp
#include <cmath>
```

---

## 🟡 Race Condition Fixed

### 8. Race Condition Between Execute and Query
**File:** `src/mod_phased_duels.cpp` (Lines 116-129)

**Problem:** `CharacterDatabase.Execute()` may be asynchronous in some AzerothCore builds, adding queries to a queue. When `CharacterDatabase.Query()` immediately runs afterward to get `newTop3`, the UPDATE might not have committed yet, causing stale data to be read. This resulted in ranking change announcements being delayed or incorrect under high concurrency.

**Fix:** Changed from `Execute()` to `DirectExecute()` for synchronous execution:
```cpp
// Update winner stats (using DirectExecute to ensure synchronous execution)
CharacterDatabase.DirectExecute("INSERT INTO duel_statistics ...");

// Update loser stats (using DirectExecute to ensure synchronous execution)
CharacterDatabase.DirectExecute("INSERT INTO duel_statistics ...");
```

This guarantees that database updates are fully committed before querying the new top 3 rankings.

---

## 🛠️ Additional Fixes

### 9. Corrupted .gitignore File
**File:** `.gitignore`

**Problem:** The file contained markdown backticks (```) as actual content, likely from copying code blocks. It also had irrelevant entries for Python/Node.js projects (`venv/`, `__pycache__/`, `.pytest_cache/`, `node_modules/`) instead of C++/AzerothCore-specific patterns.

**Fix:** 
- Removed markdown backticks from beginning and end
- Replaced with proper C++/AzerothCore gitignore patterns:
  - Build directories (`build/`, `target/`, `bin/`, `lib/`)
  - CMake files (`CMakeCache.txt`, `CMakeFiles/`, `Makefile`)
  - AzerothCore module build path (`modules/mod_phased_duels/build/`)
  - Patch files (`*.orig`, `*.rej`)
  - Kept relevant IDE and system files

---

## 📋 Configuration Note

**Typo in Config Key:** `RetorePowerForRogueOrWarrior.Enable` (should be "Restore")

This typo exists in both code and config. We kept it for backward compatibility but added a note in the config file. If you're setting up fresh, be aware of this spelling.

---

## ✅ Testing Recommendations

After applying these fixes, test the following scenarios:

1. **Console Command:** Run `.duelranking reset` from server console (not in-game) - should NOT crash
2. **Low Rating Loss:** Have a player with rating < 32 lose a duel - rating should go to 0, not wrap to billions
3. **Ranking Announcements:** Win/lose duels to change top 3 - announcements should appear
4. **Rogue/Warrior Power:** Disable `RetorePowerForRogueOrWarrior.Enable`, duel as Rogue vs Hunter - Hunter's pet should still be healed
5. **Single Pet:** Hunter (with pet) vs Mage (no pet) - Hunter's pet should be healed/revived
6. **Config Disable:** Set `PhasedDuels.Enable = 0` - phased dueling should be fully disabled
7. **Race Condition:** Run multiple simultaneous duels under high load - ranking announcements should be accurate and immediate
8. **.gitignore Verification:** Run `git status` - should not show build artifacts or irrelevant files

---

## Files Modified

- `/workspace/src/mod_phased_duels.cpp` - All bug fixes including race condition fix
- `/workspace/conf/mod_phased_duels.conf.dist` - Added documentation note about typo
- `/workspace/.gitignore` - Fixed corrupted file with proper C++/AzerothCore patterns
- `/workspace/BUGFIXES_SUMMARY.md` - This comprehensive documentation

---

**Date:** 2025
**Module Version:** AzerothCore Phased Duels + ELO Ranking
