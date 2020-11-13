# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore
## mod-npc-codebox
### This is a module for [AzerothCore](http://www.azerothcore.org)
- Latest build status with azerothcore: [![Build Status](https://github.com/azerothcore/mod-npc-codebox/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-npc-codebox)


## Description

Allows a player to transfer emblems (frost, triump, conquest) from one character to another **within the same account**. A configurable penalty can be applied (default: 10%) to the transfered amount.


## How to use ingame
1. Enter an item and code in the new database table `lootcode_items`
1. Spawn the npc and interact with it. (.npc add 601021)
2. Enter LootCode you added to the lootcode table in the world database after running the sql's.


## Requirements

- AzerothCore v1.0.1+


## Installation

```
1) Simply place the module under the `modules` directory of your AzerothCore source. 
2) Import the SQL manually to the right Database (auth, world or characters).
3) Re-run cmake and launch a clean build of AzerothCore.
```

## Edit module configuration (optional)

If you need to change the module configuration, go to your server configuration folder (where your `worldserver` or `worldserver.exe` is), copy `emblem_transfer.conf.dist` to `emblem_transfer.conf` and edit that new file.


## Credits

- [Stygianisthebest](http://stygianthebest.github.io)  

- AzerothCore: [repository](https://github.com/azerothcore) - [website](http://azerothcore.org/) - [discord chat community](https://discord.gg/PaqQRkd)
                              
    
    #############################################################################################################
    
     Modules, Scripts, and other resources for use with AzerothCore and the World of Warcraft v3.3.5a game client
    
    #############################################################################################################