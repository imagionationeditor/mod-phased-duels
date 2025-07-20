# Phased Duels Module

A comprehensive PvP enhancement module for AzerothCore that provides phased dueling, advanced ranking system, and post-duel improvements.

## Features

### 🎯 **Phased Dueling System**
- Automatically phases players during duels to prevent interference
- Smart phase allocation to avoid conflicts with other players
- Supports pets and maintains proper visibility
- Works in all outdoor areas (dungeons excluded for safety)

### 🏆 **Advanced Ranking System**
- **ELO-based rating system** for competitive dueling
- **Comprehensive statistics tracking**: wins, losses, total duels, ratings
- **Top 3 server announcements** when rankings change
- **Configurable point system** for gains and losses
- **Database persistence** across server restarts

### ✨ **Post-Duel Enhancements**
- **Full HP restoration** after duels
- **Complete cooldown reset** for fair subsequent matches
- **Mana/Energy restoration** (configurable for Warriors/Rogues)
- **Pet revival and healing** for Hunter/Warlock classes
- **Instant recovery** for continuous PvP action

### ⚙️ **Admin Tools**
- **Manual ranking reset** command for administrators
- **Comprehensive configuration options** for all features
- **Server-wide announcements** for ranking changes
- **Detailed logging** for debugging and monitoring

## Installation

1. Place the module files in your AzerothCore modules directory:
   ```
   modules/mod-phased-duels/
   ```

2. Run the database SQL files:
   ```sql
   SOURCE modules/mod-phased-duels/data/sql/db-characters/base/duel_statistics.sql;
   ```

3. Rebuild your server with the new module included

4. Configure the module settings in `mod_phased_duels.conf`

## Configuration

### Core Settings
```ini
# Enable the phased dueling system
PhasedDuels.Enable = 1

# Enable ranking system with statistics
DuelRanking.Enable = 1

# Announce top 3 ranking changes
DuelRanking.Announce = 1
```

### Ranking System
```ini
# Points gained for winning (ELO K-factor)
DuelRanking.PointsGain = 32

# Points lost for losing (ELO K-factor)  
DuelRanking.PointsLoss = 32
```

### Post-Duel Enhancements
```ini
# Restore full HP after duels
SetMaxHP.Enable = 1

# Reset all cooldowns
ResetCoolDowns.Enable = 1

# Restore mana/energy
RestorePower.Enable = 1

# Include Warriors/Rogues in power restoration
RetorePowerForRogueOrWarrior.Enable = 1

# Revive and heal pets
ReviveOrRestorPetHealth.Enable = 1
```

## Commands

### Administrator Commands
```bash
.duelranking reset
```
Manually resets all duel rankings to 0 and announces to all players. Requires Administrator access level.

## Database Schema

### `duel_statistics` Table
Stores comprehensive player dueling statistics:

| Column | Type | Description |
|--------|------|-------------|
| `player_guid` | INT | Unique player identifier (Primary Key) |
| `total_duels` | INT | Total number of duels participated |
| `wins` | INT | Number of victories |
| `losses` | INT | Number of defeats |
| `rating` | INT | Current ELO rating (default: 1500) |
| `last_duel_time` | TIMESTAMP | Last duel participation time |

## How It Works

### Phased Dueling Process
1. **Duel Initiation**: When players start a duel, the system scans for nearby players
2. **Phase Allocation**: Finds an unused phase mask to isolate the duel
3. **Player Phasing**: Both duelists and their pets are moved to the private phase
4. **Duel Flag Phasing**: The duel flag is also phased for complete isolation
5. **Post-Duel Restoration**: Players return to normal phase after duel completion

### Ranking System Process
1. **Statistics Collection**: Every duel result is recorded in the database
2. **ELO Calculation**: Uses standard ELO rating system with configurable K-factors
3. **Rating Updates**: Both winner and loser ratings are updated based on expected outcomes
4. **Ranking Announcements**: Top 3 changes are announced server-wide
5. **Leaderboard Tracking**: Maintains persistent rankings across sessions

### Post-Duel Enhancement Process
1. **HP Restoration**: Sets both players to maximum health
2. **Cooldown Reset**: Clears all spell and ability cooldowns
3. **Power Restoration**: Restores mana, energy, or rage based on class
4. **Pet Management**: Revives dead pets and restores their health and happiness
5. **Immediate Readiness**: Players are instantly ready for the next duel

## Technical Details

### Phase Management
- **Dynamic Phase Allocation**: Automatically finds unused phases (powers of 2)
- **Conflict Prevention**: Scans 100-yard radius to avoid phase conflicts
- **GM Immunity**: Game Masters can see all phases and are never phased
- **Pet Compatibility**: Ensures pets follow their owners into phased areas

### ELO Rating System
- **Standard Implementation**: Uses proven ELO algorithm for fair matchmaking
- **Expected Outcome Calculation**: Considers rating differences for point allocation
- **Configurable K-Factors**: Separate settings for gains and losses (10-50 range)
- **Anti-Inflation Measures**: Balanced point system prevents rating inflation

### Performance Optimization
- **Efficient Database Queries**: Optimized SELECT and UPDATE operations
- **Memory Management**: Minimal memory footprint with smart caching
- **Event-Driven Architecture**: Only processes during actual duel events
- **Scalable Design**: Handles multiple simultaneous duels efficiently

## Compatibility

### AzerothCore Versions
- **Tested on**: AzerothCore 3.3.5a (WotLK)
- **Minimum Requirements**: AzerothCore master branch
- **Database**: MySQL 5.7+ or MariaDB 10.3+

### Module Compatibility
- **Fully Compatible**: Works alongside other AzerothCore modules
- **No Conflicts**: Does not interfere with core duel mechanics
- **Safe Integration**: Can be enabled/disabled without affecting existing duels

## Troubleshooting

### Common Issues

**Issue**: "There are no free phases" message during duels
- **Cause**: Too many simultaneous duels in the same area
- **Solution**: Move to a different location or wait for other duels to finish

**Issue**: Ranking statistics not updating
- **Cause**: DuelRanking.Enable might be disabled
- **Solution**: Check configuration file and ensure setting is set to 1

**Issue**: Players not returning to normal phase after duel
- **Cause**: Rare edge case with phase restoration
- **Solution**: Players can relog or move to different areas to restore normal phase

### Debug Information
Enable detailed logging by setting log level to DEBUG in your AzerothCore configuration:
```ini
Logger.modules.level = 3
```

## Development

### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Code Style
- Follow AzerothCore coding standards
- Use meaningful variable names
- Comment complex algorithms
- Maintain backward compatibility

## License

This module is released under the GNU General Public License v3.0, same as AzerothCore.

## Credits

**Developer**: Mojispectre  
**Based on**: AzerothCore framework  
**Special Thanks**: AzerothCore development team for the excellent foundation

## Changelog

### Version 1.0.0
- Initial release with phased dueling system
- Advanced ELO ranking system implementation
- Post-duel enhancement features
- Comprehensive configuration options
- Administrator tools and commands

---

**For support, issues, or feature requests, please contact the developer or create an issue in the repository.**# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

## mod-phased-duels

### Phased Duels module for AzerothCore

- Latest build status with azerothcore:

[![Build Status](https://github.com/azerothcore/mod-phased-duels/workflows/core-build/badge.svg)](https://github.com/azerothcore/mod-phased-duels)

Phased Duels module create a separated phase for dueling players.

![phasedduels](https://github.com/azerothcore/mod-phased-duels/assets/2810187/3f56e1cd-f7df-4db8-91a7-ddcec3c1f9f6)

## Credits

- Rochet2
- Tommy
