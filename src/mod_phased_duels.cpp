#include "mod_phased_duels.h"

// Add command script for duel ranking reset
class DuelRankingCommands : public CommandScript
{
public:
    DuelRankingCommands() : CommandScript("DuelRankingCommands") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> duelCommandTable =
        {
            { "reset",      SEC_ADMINISTRATOR, false, &HandleDuelRankingResetCommand, "" },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "duelranking", SEC_ADMINISTRATOR, false, nullptr, "", duelCommandTable },
        };

        return commandTable;
    }

    static bool HandleDuelRankingResetCommand(ChatHandler* handler, const char* /*args*/)
    {
        // Check if ranking system is enabled
        if (!sConfigMgr->GetOption<bool>("DuelRanking.Enable", true))
        {
            handler->SendSysMessage("Duel ranking system is disabled.");
            return true;
        }

        // Reset all rankings to 0
        CharacterDatabase.Execute("UPDATE duel_statistics SET rating = 0, total_duels = 0, wins = 0, losses = 0");
        
        // Announce the reset to all players
        std::string message = "|cffFF0000[ADMIN RESET]|r All duel rankings have been reset by an administrator!";
        sWorld->SendServerMessage(SERVER_MSG_STRING, message);
        
        // Confirm to the admin
        handler->SendSysMessage("All duel rankings have been successfully reset!");
        
        // Log the action
        LOG_WARN("module.phasedduels", "Duel rankings manually reset by admin: {}", handler->GetSession()->GetPlayerName());
        
        return true;
    }
};

// Helper function to update duel statistics
void UpdateDuelStatistics(Player* winner, Player* loser)
{
    if (!sConfigMgr->GetOption<bool>("DuelRanking.Enable", true))
        return;

    uint32 winnerGuid = winner->GetGUID().GetCounter();
    uint32 loserGuid = loser->GetGUID().GetCounter();

    // Get current stats for winner
    QueryResult winnerResult = CharacterDatabase.Query("SELECT total_duels, wins, losses, rating FROM duel_statistics WHERE player_guid = {}", winnerGuid);
    uint32 winnerDuels = 0, winnerWins = 0, winnerLosses = 0, winnerRating = 1500;
    
    if (winnerResult)
    {
        Field* fields = winnerResult->Fetch();
        winnerDuels = fields[0].Get<uint32>();
        winnerWins = fields[1].Get<uint32>();
        winnerLosses = fields[2].Get<uint32>();
        winnerRating = fields[3].Get<uint32>();
    }

    // Get current stats for loser
    QueryResult loserResult = CharacterDatabase.Query("SELECT total_duels, wins, losses, rating FROM duel_statistics WHERE player_guid = {}", loserGuid);
    uint32 loserDuels = 0, loserWins = 0, loserLosses = 0, loserRating = 1500;
    
    if (loserResult)
    {
        Field* fields = loserResult->Fetch();
        loserDuels = fields[0].Get<uint32>();
        loserWins = fields[1].Get<uint32>();
        loserLosses = fields[2].Get<uint32>();
        loserRating = fields[3].Get<uint32>();
    }

    // Calculate new ratings (ELO system with configurable K-factors)
    float expectedWinner = 1.0f / (1.0f + pow(10.0f, (loserRating - winnerRating) / 400.0f));
    float expectedLoser = 1.0f / (1.0f + pow(10.0f, (winnerRating - loserRating) / 400.0f));
    
    uint32 kFactorGain = sConfigMgr->GetOption<uint32>("DuelRanking.PointsGain", 32);
    uint32 kFactorLoss = sConfigMgr->GetOption<uint32>("DuelRanking.PointsLoss", 32);
    
    uint32 newWinnerRating = winnerRating + kFactorGain * (1 - expectedWinner);
    uint32 newLoserRating = loserRating + kFactorLoss * (0 - expectedLoser);

    // Update winner stats
    CharacterDatabase.Execute("INSERT INTO duel_statistics (player_guid, total_duels, wins, losses, rating) "
                             "VALUES ({}, {}, {}, {}, {}) "
                             "ON DUPLICATE KEY UPDATE "
                             "total_duels = {}, wins = {}, losses = {}, rating = {}, last_duel_time = NOW()",
                             winnerGuid, winnerDuels + 1, winnerWins + 1, winnerLosses, newWinnerRating,
                             winnerDuels + 1, winnerWins + 1, winnerLosses, newWinnerRating);

    // Update loser stats
    CharacterDatabase.Execute("INSERT INTO duel_statistics (player_guid, total_duels, wins, losses, rating) "
                             "VALUES ({}, {}, {}, {}, {}) "
                             "ON DUPLICATE KEY UPDATE "
                             "total_duels = {}, wins = {}, losses = {}, rating = {}, last_duel_time = NOW()",
                             loserGuid, loserDuels + 1, loserWins, loserLosses + 1, newLoserRating,
                             loserDuels + 1, loserWins, loserLosses + 1, newLoserRating);

    // Check for ranking changes in top 3 before announcing
    if (sConfigMgr->GetOption<bool>("DuelRanking.Announce", true))
    {
        // Get current top 3 rankings BEFORE the duel update
        QueryResult oldTop3Result = CharacterDatabase.Query("SELECT player_guid FROM duel_statistics ORDER BY rating DESC LIMIT 3");
        std::vector<uint32> oldTop3;
        
        if (oldTop3Result)
        {
            do
            {
                Field* fields = oldTop3Result->Fetch();
                oldTop3.push_back(fields[0].Get<uint32>());
            } while (oldTop3Result->NextRow());
        }

        // Update database first (this was done above)
        
        // Get NEW top 3 rankings AFTER the duel update
        QueryResult newTop3Result = CharacterDatabase.Query("SELECT player_guid FROM duel_statistics ORDER BY rating DESC LIMIT 3");
        std::vector<uint32> newTop3;
        
        if (newTop3Result)
        {
            do
            {
                Field* fields = newTop3Result->Fetch();
                newTop3.push_back(fields[0].Get<uint32>());
            } while (newTop3Result->NextRow());
        }

        // Compare old vs new top 3 and announce changes
        for (size_t i = 0; i < 3 && i < newTop3.size(); ++i)
        {
            bool rankChanged = false;
            std::string rankName = (i == 0) ? "First" : (i == 1) ? "Second" : "Third";
            std::string rankNumber = std::to_string(i + 1);

            // Check if this position changed
            if (i >= oldTop3.size() || oldTop3[i] != newTop3[i])
            {
                rankChanged = true;
            }

            if (rankChanged)
            {
                // Get player names
                std::string newPlayerName = "";
                std::string oldPlayerName = "";

                // Get new player name
                if (newTop3[i] == winnerGuid)
                {
                    newPlayerName = winner->GetName();
                }
                else if (newTop3[i] == loserGuid)
                {
                    newPlayerName = loser->GetName();
                }
                else
                {
                    QueryResult nameResult = CharacterDatabase.Query("SELECT name FROM characters WHERE guid = {}", newTop3[i]);
                    if (nameResult)
                        newPlayerName = nameResult->Fetch()[0].Get<std::string>();
                }

                // Get old player name (if there was one)
                if (i < oldTop3.size())
                {
                    if (oldTop3[i] == winnerGuid)
                    {
                        oldPlayerName = winner->GetName();
                    }
                    else if (oldTop3[i] == loserGuid)
                    {
                        oldPlayerName = loser->GetName();
                    }
                    else
                    {
                        QueryResult nameResult = CharacterDatabase.Query("SELECT name FROM characters WHERE guid = {}", oldTop3[i]);
                        if (nameResult)
                            oldPlayerName = nameResult->Fetch()[0].Get<std::string>();
                    }
                }

                // Create announcement message
                std::string message;
                if (!oldPlayerName.empty())
                {
                    message = "|cffFFD700[RANKING CHANGE]|r " + newPlayerName + " is now rank #" + rankNumber + 
                             " and took " + oldPlayerName + "'s position!";
                }
                else
                {
                    message = "|cffFFD700[RANKING CHANGE]|r " + newPlayerName + " is now rank #" + rankNumber + "!";
                }

                sWorld->SendServerMessage(SERVER_MSG_STRING, message);
            }
        }
    }
}

uint32 PhasedDueling::getNormalPhase(Player* player) const
{
    if (player->IsGameMaster())
        return uint32(PHASEMASK_ANYWHERE);

    // GetPhaseMaskForSpawn copypaste
    uint32 phase = PHASEMASK_NORMAL;
    Player::AuraEffectList const& phases = player->GetAuraEffectsByType(SPELL_AURA_PHASE);
    if (!phases.empty())
        phase = phases.front()->GetMiscValue();
    if (uint32 n_phase = phase & ~PHASEMASK_NORMAL)
        return n_phase;

    return PHASEMASK_NORMAL;
}

void PhasedDueling::OnPlayerLogin(Player* player)
{
    if (sConfigMgr->GetOption<bool>("PhasedDuelsAnnounce.Enable", true))
        ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00PhasedDuels |rmodule.");
}

void PhasedDueling::OnPlayerDuelStart(Player* firstplayer, Player* secondplayer)
{
    if (sConfigMgr->GetOption<bool>("PhasedDuels.Enable", true))
    {
        Map* map = firstplayer->GetMap();
        if (map->IsDungeon())
            return;

        // Duel flag is used as duel center point
        GameObject* go = map->GetGameObject(firstplayer->GetGuidValue(PLAYER_DUEL_ARBITER));
        if (!go)
            return;

        // Get players from 100 yard radius ( duel radius is 40-50 yd )
        std::list<Player*> playerList;
        Acore::AnyPlayerInObjectRangeCheck checker(go, 100.0f);
        Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(go, playerList, checker);
        Cell::VisitWorldObjects(go, searcher, 100.0f);

        // insert players' phases to used phases, ignore GMs
        uint32 usedPhases = 0;
        if (!playerList.empty())
            for (std::list<Player*>::const_iterator it = playerList.begin(); it != playerList.end(); ++it)
                if (!(*it)->IsGameMaster())
                    usedPhases |= (*it)->GetPhaseMask();

        // loop all unique phases
        for (uint32 phase = 2; phase <= UINT_MAX / 2; phase *= 2)
        {
            // If phase in use, skip
            if (usedPhases & phase)
                continue;

            // Phase players & pets, dont update visibility yet
            firstplayer->SetPhaseMask(phase, false);
            secondplayer->SetPhaseMask(phase, false);
            // Phase duel flag
            go->SetPhaseMask(phase, true);
            // Update visibility here so pets will be phased and wont despawn
            firstplayer->UpdateObjectVisibility();
            secondplayer->UpdateObjectVisibility();
            return;
        }

        // Couldnt find free unique phase
        ChatHandler(firstplayer->GetSession()).SendNotification("There are no free phases");
        ChatHandler(secondplayer->GetSession()).SendNotification("There are no free phases");
    }
}

void PhasedDueling::OnPlayerDuelEnd(Player* firstplayer, Player* secondplayer, DuelCompleteType type)
{
    // Update duel statistics for ranking system
    if (type == DUEL_WON)
    {
        UpdateDuelStatistics(firstplayer, secondplayer); // firstplayer won
    }
    else if (type == DUEL_FLED)
    {
        UpdateDuelStatistics(secondplayer, firstplayer); // secondplayer won (firstplayer fled)
    }

    if (sConfigMgr->GetOption<bool>("PhasedDueling.Enable", true))
    {
        // Phase players, dont update visibility yet
        firstplayer->SetPhaseMask(getNormalPhase(firstplayer), false);
        secondplayer->SetPhaseMask(getNormalPhase(secondplayer), false);
        // Update visibility here so pets will be phased and wont despawn
        firstplayer->UpdateObjectVisibility();
        secondplayer->UpdateObjectVisibility();

        if (sConfigMgr->GetOption<bool>("SetMaxHP.Enable", true))
        {
            firstplayer->SetHealth(firstplayer->GetMaxHealth());
            secondplayer->SetHealth(secondplayer->GetMaxHealth());
        }

        if (sConfigMgr->GetOption<bool>("ResetCoolDowns.Enable", true))
        {
            firstplayer->RemoveAllSpellCooldown();
            secondplayer->RemoveAllSpellCooldown();
        }

        if (sConfigMgr->GetOption<bool>("RestorePower.Enable", true))
        {
            if (!sConfigMgr->GetOption<bool>("RetorePowerForRogueOrWarrior.Enable", true))
            {
                if (firstplayer->getClass() == CLASS_ROGUE || firstplayer->getClass() == CLASS_WARRIOR)
                    return;

                if (secondplayer->getClass() == CLASS_ROGUE || secondplayer->getClass() == CLASS_WARRIOR)
                    return;
            }

            firstplayer->SetPower(firstplayer->getPowerType(), firstplayer->GetMaxPower(firstplayer->getPowerType()));
            secondplayer->SetPower(secondplayer->getPowerType(), secondplayer->GetMaxPower(secondplayer->getPowerType()));
        }

        if (sConfigMgr->GetOption<bool>("ReviveOrRestorPetHealth.Enable", true))
        {
            Pet* pet1 = firstplayer->GetPet();
            Pet* pet2 = secondplayer->GetPet();

            if (!pet1 || !pet2)
                return;

            if (!pet1->IsAlive() || !pet2->IsAlive())
            {
                if (firstplayer->getClass() == CLASS_HUNTER || secondplayer->getClass() == CLASS_HUNTER)
                {
                    pet1->SetPower(POWER_HAPPINESS, pet1->GetMaxPower(POWER_HAPPINESS));
                    pet2->SetPower(POWER_HAPPINESS, pet2->GetMaxPower(POWER_HAPPINESS));
                }
                pet1->setDeathState(DeathState::Alive);
                pet2->setDeathState(DeathState::Alive);
            }

            pet1->SetHealth(pet1->GetMaxHealth());
            pet2->SetHealth(pet2->GetMaxHealth());
        }
    }
}

// Add scripts
void AddPhasedDuelsScripts()
{
    new PhasedDueling();
    new DuelRankingCommands();
}
