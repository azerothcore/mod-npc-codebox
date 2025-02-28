/*

# Codebox NPC #

#### A module for AzerothCore by [StygianTheBest](https://github.com/StygianTheBest/AzerothCore-Content/tree/master/Modules)
------------------------------------------------------------------------------------------------------------------


### Description ###
------------------------------------------------------------------------------------------------------------------
Meet Retdream, the Keeper of Codes. She's a codebox NPC that emotes and speaks. This NPC takes codes from the player
and checks them against custom database tables to determine the loot. You can set charges for items to allow players
to use the code once or a specific number of times. It also supports unique codes that can only be used once by any
player.


### Features ###
------------------------------------------------------------------------------------------------------------------
- Creates a CodeBox NPC with emotes
- GM can view a menu of codes in the database in-game
- GM can create, read, update, and delete codes in-game
- Characters input codes to change name, race, or faction
- Gives items and/or gold to players if they enter the correct code
- Reads/Writes code data from the database
- Supports alpha-numeric codes
- Allows unique codes to be given out only once
- Checks for already redeemed codes
- Codes have charges that can prevent the code from being used more than X times
- Supports multiple items per code
- I check the db for # of codes the player has obtained so..
  - Charges for each item in a multi-code must be equal to: (charges X # of items)
  - ex) A 3 item code with 1 charge must have 9 charges for each item

### Sample Codes

- These codes will add the following items to the player's inventory:
  - **artifact**: A one-charge code from a questgiver or GM.
  - **ballroom**: A multi-charge code giving three items for a tuxedo outfit.
  - **threebags**: A limited multi-charge code giving 3 Netherweave bags to the player.
  - **lockpick**: A limited multi-charge code giving 5 skeleton keys to the player.

- These codes will initiate a character customization:
  - **race**: A customization code that queues the player to change their race.
  - **name**: A customization code that queues the player to change their name.
  - **faction**: A customization code that queues the player to change their faction.


### To-Do ###
------------------------------------------------------------------------------------------------------------------
- If possible, create a way to prevent players from trading codes
- Refactor the charges code


### Data ###
------------------------------------------------------------------------------------------------------------------
- Type: NPC
- Script: codebox_npc
- Config: Yes
    - Enable Module Announce
- SQL: Yes
    - NPC ID: 601021
    - Add Table: lootcode_items (codes, items, charges, etc.)
    - Add Table: lootcode_player (tracks redeemed codes)


### Version ###
------------------------------------------------------------------------------------------------------------------
- v2019.03.11 - Add GM Code CRUD & Character Customization by WiiZZy
- v2019.02.09 - Update AI, Config
- v2017.08.10 - Release


### Credits ###
------------------------------------------------------------------------------------------------------------------
- [WiiZZy](https://github.com/wizzymore)
- [Blizzard Entertainment](http://blizzard.com)
- [TrinityCore](https://github.com/TrinityCore/TrinityCore/blob/3.3.5/THANKS)
- [SunwellCore](http://www.azerothcore.org/pages/sunwell.pl/)
- [AzerothCore](https://github.com/AzerothCore/azerothcore-wotlk/graphs/contributors)
- [AzerothCore Discord](https://discord.gg/gkt4y2x)
- [EMUDevs](https://youtube.com/user/EmuDevs)
- [AC-Web](http://ac-web.org/)
- [ModCraft.io](http://modcraft.io/)
- [OwnedCore](http://ownedcore.com/)
- [OregonCore](https://wiki.oregon-core.net/)
- [Wowhead.com](http://wowhead.com)
- [AoWoW](https://wotlk.evowow.com/)


### License ###
------------------------------------------------------------------------------------------------------------------
- This code and content is released under the [GNU AGPL v3](https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3).

*/
#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"

enum Customization {
    CUSTOMIZE_FACTION = 1,
    CUSTOMIZE_RACE,
    CUSTOMIZE_RENAME
};

struct LootElements
{
    char loot[32] = "";
    uint32 itemId = 0;
    char name[255] = "";
    uint32 quantity = 1;
    uint32 gold = 0;
    uint32 customize = 0;
    uint32 charges = 1;
    uint32 unique = 0;
};

struct ShowLootElements
{
    char loot[32] = "";
    uint32 itemId = 0;
    char name[255] = "";
    uint32 quantity = 1;
    uint32 gold = 0;
    uint32 customize = 0;
    uint32 charges = 1;
    uint32 unique = 0;
};

struct DeleteLootElements
{
    char loot[32] = "";
};

uint32 CodeboxNPCID = 601021;
uint32 CodeboxMessageTimer;
uint32 max_loot_results = 9;
bool CodeboxAnnounceModule;
typedef std::unordered_map<uint32, ShowLootElements> ShowLoot_Pair;

std::unordered_map<ObjectGuid, DeleteLootElements>DelLoot;
std::unordered_map<ObjectGuid, LootElements>AddLoot;
std::unordered_map<ObjectGuid, ShowLoot_Pair> ShowLoot;
std::unordered_map<ObjectGuid, uint32> editid;
std::unordered_map<ObjectGuid, uint32> lootid;

class CodeboxConfig : public WorldScript
{
public:
    CodeboxConfig() : WorldScript("CodeboxConfig", {
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD
    }) { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
            CodeboxAnnounceModule = sConfigMgr->GetOption<bool>("Codebox.Announce", true);
            CodeboxMessageTimer = sConfigMgr->GetOption<int>("Codebox.MessageTimer", 60000);

            // Enforce Min/Max Time
            if (CodeboxMessageTimer != 0)
            {
                if (CodeboxMessageTimer < 60000 || CodeboxMessageTimer > 300000)
                    CodeboxMessageTimer = 60000;
            }
        }
    }
};

class CodeboxAnnounce : public PlayerScript
{

public:

    CodeboxAnnounce() : PlayerScript("CodeboxAnnounce", {
        PLAYERHOOK_ON_LOGIN
    }) {}

    void OnPlayerLogin(Player* player) override
    {
        // Announce Module
        if (CodeboxAnnounceModule)
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00CodeboxNPC |rmodule.");
    }
};

class codebox_npc : public CreatureScript
{

public:

    codebox_npc() : CreatureScript("codebox_npc") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        showInitialMenu(player, creature);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        if (action == 1) {
            if (!code)
                code = "";

            // Determine Loot
            getLoot(player, creature, code);
        }

        ObjectGuid guid = player->GetGUID();

        if (action == 20)
        {
            if (!code || strcmp(code, " ") != -1)
            {
                code = "";
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", that is not a valid code name.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                SendGossipMenuFor(player, CodeboxNPCID, creature->GetGUID());
                return false;
            }

            // Check for a valid code
            QueryResult checkCode = WorldDatabase.Query("SELECT code, itemId, quantity, gold, customize, charges, isUnique FROM lootcode_items WHERE code = '{}'", (code));

            if (checkCode)
            {
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", this code already exists.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                SendGossipMenuFor(player, CodeboxNPCID, creature->GetGUID());
            }
            else
            {
                snprintf(AddLoot[guid].loot, sizeof(AddLoot[guid].loot), "%s", code);
                showLootMenu(player, creature);
            }
        }

        if (action == 21)
        {
            AddLoot[guid].itemId = charptouint(code);
            showLootMenu(player, creature);
        }

        if (action == 22)
        {
            snprintf(AddLoot[guid].name, sizeof(AddLoot[guid].name), "%s", code);
            showLootMenu(player, creature);
        }

        if (action == 23)
        {
            AddLoot[guid].quantity = charptouint(code);
            showLootMenu(player, creature);
        }

        if (action == 24)
        {
            AddLoot[guid].gold = charptouint(code);
            showLootMenu(player, creature);
        }
        if (action == 26)
        {
            AddLoot[guid].charges = charptouint(code);
            showLootMenu(player, creature);
        }
        if (action == 27)
        {
            AddLoot[guid].unique = charptouint(code);
            showLootMenu(player, creature);
        }
        if (action == 40)
        {
            // Check for a valid code
            QueryResult checkCode = WorldDatabase.Query("SELECT code, itemId, quantity, gold, customize, charges, isUnique FROM lootcode_items WHERE code = '{}'", (code));
            if (!checkCode)
            {
                // No code match found in database
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", that is not a valid code.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                showInitialMenu(player, creature);
                return true;
            }
            else
            {
                snprintf(DelLoot[guid].loot, sizeof(DelLoot[guid].loot), "%s", code);
                ClearGossipMenuFor(player);
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Delete Loot Code", GOSSIP_SENDER_MAIN, 41);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Cancel", GOSSIP_SENDER_MAIN, 42);

                SendGossipMenuFor(player, 661040, creature->GetGUID());
            }
        }

        if (action == 60)
        {
            if (!code || strcmp(code, " ") != -1)
            {
                code = "";
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", that is not a valid code name.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                SendGossipMenuFor(player, CodeboxNPCID, creature->GetGUID());
                return false;
            }
            // Check for a valid code
            QueryResult checkCode = WorldDatabase.Query("SELECT code, itemId, quantity, gold, customize, charges, isUnique FROM lootcode_items WHERE code = '{}'", (code));

            if (checkCode)
            {
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", this code already exists.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                SendGossipMenuFor(player, CodeboxNPCID, creature->GetGUID());
            }
            else
            {
                snprintf(ShowLoot[guid][editid[guid]].loot, sizeof(ShowLoot[guid][editid[guid]].loot), "%s", code);
                showEditMenu(player, creature, editid[guid]);
            }
        }

        if (action == 61)
        {
            ShowLoot[guid][editid[guid]].itemId = charptouint(code);
            showEditMenu(player, creature, editid[guid]);
        }
        if (action == 62)
        {
            snprintf(ShowLoot[guid][editid[guid]].name, sizeof(ShowLoot[guid][editid[guid]].name), "%s", code);
            showEditMenu(player, creature, editid[guid]);
        }
        if (action == 63)
        {
            ShowLoot[guid][editid[guid]].quantity = charptouint(code);
            showEditMenu(player, creature, editid[guid]);
        }
        if (action == 64)
        {
            ShowLoot[guid][editid[guid]].gold = charptouint(code);
            showEditMenu(player, creature, editid[guid]);
        }
        if (action == 66)
        {
            ShowLoot[guid][editid[guid]].charges = charptouint(code);
            showEditMenu(player, creature, editid[guid]);
        }
        if (action == 67)
        {
            ShowLoot[guid][editid[guid]].unique = charptouint(code);
            showEditMenu(player, creature, editid[guid]);
        }

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        ObjectGuid guid = player->GetGUID();

        if (action == 25)
        {
            ClearGossipMenuFor(player);

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Faction", GOSSIP_SENDER_MAIN, 250);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Race", GOSSIP_SENDER_MAIN, 251);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Name", GOSSIP_SENDER_MAIN, 252);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back..", GOSSIP_SENDER_MAIN, 253);

            SendGossipMenuFor(player, 661025, creature->GetGUID());
        }
        if (action == 250)
        {
            // Change Faction
            AddLoot[guid].customize = CUSTOMIZE_FACTION;
            showLootMenu(player, creature);
        }
        if (action == 251)
        {
            // Change Race
            AddLoot[guid].customize = CUSTOMIZE_RACE;
            showLootMenu(player, creature);
        }
        if (action == 252)
        {
            // Rename Character
            AddLoot[guid].customize = CUSTOMIZE_RENAME;
            showLootMenu(player, creature);
        }
        if (action == 253)
        {
            showLootMenu(player, creature);
        }
        if (action == 27)
        {
            // Edit Unique
            if (AddLoot[guid].unique == 1)
                AddLoot[guid].unique = 0;
            else
                AddLoot[guid].unique = 1;
            showLootMenu(player, creature);
        }
        if (action == 28)
        {
            // Add the entry to the database
            WorldDatabase.Query("INSERT INTO lootcode_items (code, itemId, name, quantity, gold, customize, charges, isUnique) VALUES ('{}', {}, '{}', {}, {}, {}, {}, {});", AddLoot[guid].loot, AddLoot[guid].itemId, AddLoot[guid].name, AddLoot[guid].quantity, AddLoot[guid].gold, AddLoot[guid].customize, AddLoot[guid].charges, AddLoot[guid].unique);
            std::ostringstream messageCode;
            messageCode << "The lootcode " << AddLoot[guid].loot << " has been added in the database.";
            creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
            initializeAddLoot(guid);
            showInitialMenu(player, creature);
        }
        if (action == 29)
        {
            // Initialize Loot
            initializeAddLoot(guid);
            showLootMenu(player, creature);
        }
        if (action == 30)
        {
            // Show Menu
            showInitialMenu(player, creature);
        }
        if (action == 41)
        {
            // Delete the entry from the database
            WorldDatabase.Query("DELETE FROM lootcode_items WHERE  code = '{}';", DelLoot[guid].loot);
            std::ostringstream messageCode;
            messageCode << "The lootcode " << DelLoot[guid].loot << " has been deleted from the database.";
            //messageCode << "The lootcode " << AddLoot[guid].loot << " has been deleted from the database.";
            creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
            showInitialMenu(player, creature);
        }
        if (action == 42)
        {
            // Show Menu
            showInitialMenu(player, creature);
        }
        if (action == 50)
        {
            // Delete the entry from the database
            ClearGossipMenuFor(player);
            lootid[guid] = 0;

            QueryResult getLoot = WorldDatabase.Query("SELECT * FROM lootcode_items LIMIT {};", max_loot_results);
            QueryResult count_results = WorldDatabase.Query("SELECT COUNT(id) FROM lootcode_items");
            Field * fields_count = count_results->Fetch();
            uint32 total_results = fields_count[0].Get<uint32>();

            do
            {
                if (!getLoot)
                {
                    // No code match found in database
                    std::ostringstream messageCode;
                    messageCode << "Error: Code Not Found!";
                    player->PlayDirectSound(9638); // No
                    creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                    creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                    showInitialMenu(player, creature);
                    return true;
                }
                else
                {
                    Field * fields = getLoot->Fetch();
                    uint32 id = fields[0].Get<uint32>();
                    snprintf(ShowLoot[guid][id].loot, sizeof(ShowLoot[guid][id].loot), "%s", fields[1].Get<std::string>().c_str());
                    ShowLoot[guid][id].itemId = fields[2].Get<uint32>();
                    snprintf(ShowLoot[guid][id].name, sizeof(ShowLoot[guid][id].name), "%s", fields[3].Get<std::string>().c_str());
                    ShowLoot[guid][id].quantity = fields[4].Get<uint32>();
                    ShowLoot[guid][id].gold = fields[5].Get<uint32>();
                    ShowLoot[guid][id].customize = fields[6].Get<uint32>();
                    ShowLoot[guid][id].charges = fields[7].Get<uint32>();
                    ShowLoot[guid][id].unique = fields[8].Get<uint32>();

                    char message[1024] = "";
                    snprintf(message, 1024, "%u) [%s] %s", id, ShowLoot[guid][id].loot, ShowLoot[guid][id].name);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, message, GOSSIP_SENDER_MAIN, 100 + id);
                }
            } while (getLoot->NextRow());

            if (total_results > max_loot_results)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Next", GOSSIP_SENDER_MAIN, 80);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 82);
            SendGossipMenuFor(player, 601050, creature->GetGUID());
        }
        if (action == 65)
        {
            // Show Customize Options
            ClearGossipMenuFor(player);

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Faction", GOSSIP_SENDER_MAIN, 650);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Race", GOSSIP_SENDER_MAIN, 651);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Customize Character Name", GOSSIP_SENDER_MAIN, 652);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back..", GOSSIP_SENDER_MAIN, 653);

            SendGossipMenuFor(player, 661065, creature->GetGUID());
        }

        if (action == 650)
        {
            // Change Faction
            ShowLoot[guid][editid[guid]].customize = CUSTOMIZE_FACTION;
            showEditMenu(player, creature, editid[guid]);
        }

        if (action == 651)
        {
            // Change Race
            ShowLoot[guid][editid[guid]].customize = CUSTOMIZE_RACE;
            showEditMenu(player, creature, editid[guid]);
        }

        if (action == 652)
        {
            // Rename Character
            ShowLoot[guid][editid[guid]].customize = CUSTOMIZE_RENAME;
            showEditMenu(player, creature, editid[guid]);
        }

        if (action == 653)
        {
            // Edit Code
            showEditMenu(player, creature, editid[guid]);
        }

        if (action == 67)
        {
            // Unique Edit
            if (ShowLoot[guid][editid[guid]].unique == 1)
                ShowLoot[guid][editid[guid]].unique = 0;
            else
                ShowLoot[guid][editid[guid]].unique = 1;
            showLootMenu(player, creature);
        }

        if (action == 70)
        {
            // Add the entry to the database
            WorldDatabase.Query("UPDATE lootcode_items SET code='{}', itemId={}, name='{}', quantity={}, gold={}, customize={}, charges={}, isUnique={} WHERE  id={};", ShowLoot[guid][editid[guid]].loot, ShowLoot[guid][editid[guid]].itemId, ShowLoot[guid][editid[guid]].name, ShowLoot[guid][editid[guid]].quantity, ShowLoot[guid][editid[guid]].gold, ShowLoot[guid][editid[guid]].customize, ShowLoot[guid][editid[guid]].charges, ShowLoot[guid][editid[guid]].unique, editid[guid]);
            std::ostringstream messageCode;
            messageCode << "The lootcode " << ShowLoot[guid][editid[guid]].loot << " edited.";
            creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
            showInitialMenu(player, creature);
        }

        if (action == 71)
        {
            // Display the menu
            showInitialMenu(player, creature);
        }

        if (action == 80)
        {
            ClearGossipMenuFor(player);
            lootid[guid] += max_loot_results;
            QueryResult getLoot = WorldDatabase.Query("SELECT * FROM lootcode_items LIMIT {},{};", lootid[guid], max_loot_results);
            QueryResult count_results = WorldDatabase.Query("SELECT COUNT(id) FROM lootcode_items");
            Field * fields_count = count_results->Fetch();
            uint32 total_results = fields_count[0].Get<uint32>();

            do
            {
                if (!getLoot)
                {
                    // No code match found in database
                    std::ostringstream messageCode;
                    messageCode << "Error: Code Not Found!";
                    player->PlayDirectSound(9638); // No
                    creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                    creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                    showInitialMenu(player, creature);
                    return true;
                }
                else
                {
                    Field * fields = getLoot->Fetch();
                    uint32 id = fields[0].Get<uint32>();
                    snprintf(ShowLoot[guid][id].loot, sizeof(ShowLoot[guid][id].loot), "%s", fields[1].Get<std::string>().c_str());
                    ShowLoot[guid][id].itemId = fields[2].Get<uint32>();
                    snprintf(ShowLoot[guid][id].name, sizeof(ShowLoot[guid][id].name), "%s", fields[3].Get<std::string>().c_str());
                    ShowLoot[guid][id].quantity = fields[4].Get<uint32>();
                    ShowLoot[guid][id].gold = fields[5].Get<uint32>();
                    ShowLoot[guid][id].customize = fields[6].Get<uint32>();
                    ShowLoot[guid][id].charges = fields[7].Get<uint32>();
                    ShowLoot[guid][id].unique = fields[8].Get<uint32>();

                    char message[1024] = "";
                    snprintf(message, 1024, "%u) [%s] %s", id, ShowLoot[guid][id].loot, ShowLoot[guid][id].name);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, message, GOSSIP_SENDER_MAIN, 100 + id);
                }
            } while (getLoot->NextRow());

            if (total_results - lootid[guid] > max_loot_results)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Next", GOSSIP_SENDER_MAIN, 80);

            if (lootid[guid] >= max_loot_results)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 81);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 82);
            SendGossipMenuFor(player, 601050, creature->GetGUID());
        }

        if (action == 81)
        {
            ClearGossipMenuFor(player);
            lootid[guid] -= max_loot_results;
            QueryResult getLoot = WorldDatabase.Query("SELECT * FROM lootcode_items LIMIT {},{};", lootid[guid], max_loot_results);
            QueryResult count_results = WorldDatabase.Query("SELECT COUNT(id) FROM lootcode_items");
            Field * fields_count = count_results->Fetch();
            uint32 total_results = fields_count[0].Get<uint32>();

            do
            {
                if (!getLoot)
                {
                    // No code match found in database
                    std::ostringstream messageCode;
                    messageCode << "Error: Code Not Found!";
                    player->PlayDirectSound(9638); // No
                    creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                    creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                    showInitialMenu(player, creature);
                    return true;
                }
                else
                {
                    Field * fields = getLoot->Fetch();
                    uint32 id = fields[0].Get<uint32>();
                    snprintf(ShowLoot[guid][id].loot, sizeof(ShowLoot[guid][id].loot), "%s", fields[1].Get<std::string>().c_str());
                    ShowLoot[guid][id].itemId = fields[2].Get<uint32>();
                    snprintf(ShowLoot[guid][id].name, sizeof(ShowLoot[guid][id].name), "%s", fields[3].Get<std::string>().c_str());
                    ShowLoot[guid][id].quantity = fields[4].Get<uint32>();
                    ShowLoot[guid][id].gold = fields[5].Get<uint32>();
                    ShowLoot[guid][id].customize = fields[6].Get<uint32>();
                    ShowLoot[guid][id].charges = fields[7].Get<uint32>();
                    ShowLoot[guid][id].unique = fields[8].Get<uint32>();

                    char message[1024] = "";
                    snprintf(message, 1024, "%u) [%s] %s", id, ShowLoot[guid][id].loot, ShowLoot[guid][id].name);
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, message, GOSSIP_SENDER_MAIN, 100 + id);
                }
            } while (getLoot->NextRow());

            if (total_results - lootid[guid] > max_loot_results)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Next", GOSSIP_SENDER_MAIN, 80);

            if (lootid[guid] >= max_loot_results)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back", GOSSIP_SENDER_MAIN, 81);

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Close", GOSSIP_SENDER_MAIN, 82);
            SendGossipMenuFor(player, 601050, creature->GetGUID());
        }

        if (action == 82)
            showInitialMenu(player, creature);

        if (action >= 100)
        {
            ClearGossipMenuFor(player);

            editid[guid] = action - 100;
            std::string add_loot_text = "Enter Loot Code";
            char message[1024];

            snprintf(message, 1024, "Loot Code: %s", ShowLoot[guid][editid[guid]].loot);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 60, "", 0, true);

            snprintf(message, 1024, "Item ID: %u", ShowLoot[guid][editid[guid]].itemId);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 61, "", 0, true);

            snprintf(message, 1024, "Description: %s", ShowLoot[guid][editid[guid]].name);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 62, "", 0, true);

            snprintf(message, 1024, "Quantity: %u", ShowLoot[guid][editid[guid]].quantity);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 63, "", 0, true);

            snprintf(message, 1024, "Gold: %u", ShowLoot[guid][editid[guid]].gold);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 64, "", 0, true);

            snprintf(message, 1024, "Customize ID: %u", ShowLoot[guid][editid[guid]].customize);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 65);

            snprintf(message, 1024, "Charges: %u", ShowLoot[guid][editid[guid]].charges);
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 66, "", 0, true);

            snprintf(message, 1024, "Unique: %u", ShowLoot[guid][editid[guid]].unique);

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 67);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Confirm", GOSSIP_SENDER_MAIN, 70);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back..", GOSSIP_SENDER_MAIN, 71);

            SendGossipMenuFor(player, 601022, creature->GetGUID());
        }
        return true;
    }

    // Convert Charater Pointer to Int
    uint32 charptouint(const char* code)
    {
        std::stringstream strValue;
        strValue << code;

        uint32 intValue;
        strValue >> intValue;
        return intValue;
    }

    void showEditMenu(Player* player, Creature* creature, uint32 id)
    {
        ClearGossipMenuFor(player);

        ObjectGuid guid = player->GetGUID();
        std::string add_loot_text = "Enter Loot Code";
        char message[1024];

        snprintf(message, 1024, "Loot Code: %s", ShowLoot[guid][id].loot);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 60, "", 0, true);

        snprintf(message, 1024, "Item ID: %u", ShowLoot[guid][id].itemId);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 61, "", 0, true);

        snprintf(message, 1024, "Description: %s", ShowLoot[guid][id].name);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 62, "", 0, true);

        snprintf(message, 1024, "Quantity: %u", ShowLoot[guid][id].quantity);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 63, "", 0, true);

        snprintf(message, 1024, "Gold: %u", ShowLoot[guid][id].gold);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 64, "", 0, true);

        snprintf(message, 1024, "Customize ID: %u", ShowLoot[guid][id].customize);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 65);

        snprintf(message, 1024, "Charges: %u", ShowLoot[guid][id].charges);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 66, "", 0, true);

        snprintf(message, 1024, "Unique: %u", ShowLoot[guid][id].unique);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 67);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Confirm", GOSSIP_SENDER_MAIN, 70);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back..", GOSSIP_SENDER_MAIN, 71);

        SendGossipMenuFor(player, 601022, creature->GetGUID());
    }

    void showLootMenu(Player* player, Creature* creature)
    {

        ClearGossipMenuFor(player);

        ObjectGuid guid = player->GetGUID();
        std::string add_loot_text = "Enter Loot Code";
        char message[1024];

        snprintf(message, 1024, "Loot Code: %s", AddLoot[guid].loot);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 20, "", 0, true);

        snprintf(message, 1024, "Item ID: %u", AddLoot[guid].itemId);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 21, "", 0, true);

        snprintf(message, 1024, "Description: %s", AddLoot[guid].name);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 22, "", 0, true);

        snprintf(message, 1024, "Quantity: %u", AddLoot[guid].quantity);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 23, "", 0, true);

        snprintf(message, 1024, "Gold: %u", AddLoot[guid].gold);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 24, "", 0, true);

        snprintf(message, 1024, "Customize ID: %u", AddLoot[guid].customize);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 25);

        snprintf(message, 1024, "Charges: %u", AddLoot[guid].charges);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 26, "", 0, true);

        snprintf(message, 1024, "Unique: %u", AddLoot[guid].unique);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, message, GOSSIP_SENDER_MAIN, 27);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Create Loot Code", GOSSIP_SENDER_MAIN, 28);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Reset", GOSSIP_SENDER_MAIN, 29);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Back..", GOSSIP_SENDER_MAIN, 30);

        SendGossipMenuFor(player, 601022, creature->GetGUID());
    }

    void showInitialMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);

        // If GameMaster
        if (player->IsGameMaster())
            initializeAddLoot(player->GetGUID());

        std::string text = "Enter Loot Code then Press Accept";
        std::string add_loot_text = "Add A Loot Code?";
        std::string remove_loot_text = "Remove A Loot Code?";
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "I'd like to enter my loot code.", GOSSIP_SENDER_MAIN, 1, text, 0, true);

        // For Gamemasters
        if (player->IsGameMaster())
        {
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "[GM] Add Loot Code", GOSSIP_SENDER_MAIN, 20, add_loot_text, 0, true);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "[GM] Remove Loot Code", GOSSIP_SENDER_MAIN, 40, remove_loot_text, 0, true);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "[GM] View Loot Codes", GOSSIP_SENDER_MAIN, 50);
        }
        SendGossipMenuFor(player, CodeboxNPCID, creature->GetGUID());
    }

    void initializeAddLoot(ObjectGuid guid)
    {
        lootid[guid] = 0;
        AddLoot[guid].itemId = 0;
        snprintf(AddLoot[guid].name, sizeof(AddLoot[guid].name), " ");
        AddLoot[guid].quantity = 1;
        AddLoot[guid].gold = 0;
        AddLoot[guid].customize = 0;
        AddLoot[guid].charges = 1;
        AddLoot[guid].unique = 0;
    }

    void getLoot(Player* player, Creature* creature, const char* code)
    {
        // Check for a valid code
        QueryResult checkCode = WorldDatabase.Query("SELECT code, itemId, quantity, gold, customize, charges, isUnique FROM lootcode_items WHERE code = '{}'", (code));

        // Check if player has redeemed the code
        QueryResult getLoot = WorldDatabase.Query("SELECT playerGUID, count(code) AS chargesUsed FROM lootcode_player WHERE playerGUID like {} AND code = '{}'", player->GetGUID().GetCounter(), (code));

        do
        {
            if (!checkCode)
            {
                // No code match found in database
                std::ostringstream messageCode;
                messageCode << "Sorry " << player->GetName() << ", that is not a valid code.";
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                showInitialMenu(player, creature);
                return;
            }
            else {

                // Get checkCode fields
                Field * fields = checkCode->Fetch();
                std::string const code = fields[0].Get<std::string>();
                uint32 itemId = fields[1].Get<uint32>();
                uint32 quantity = fields[2].Get<uint32>();
                uint32 gold = fields[3].Get<uint32>();
                uint32 customize = fields[4].Get<uint32>();
                uint32 charges = fields[5].Get<uint32>();
                uint32 isUnique = fields[6].Get<uint32>();

                // Get getLoot fields
                Field * fields2 = getLoot->Fetch();
                uint32 chargesUsed = fields2[1].Get<uint32>();

                // If the code is unqiue, check to see if anyone has already used it.
                if (isUnique == 1)
                {
                    // Query for the unique code
                    QueryResult uniqueRedeemed = WorldDatabase.Query("SELECT playerGUID, isUnique FROM lootcode_player WHERE code = '{}' AND isUnique = 1", (code.c_str()));

                    // If any player has redeemed this unique code, deny the code
                    if (uniqueRedeemed)
                    {
                        std::ostringstream messageCode;
                        messageCode << "Sorry " << player->GetName() << ", This unique code has already been redeemed.";
                        player->PlayDirectSound(9638); // No
                        creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                        creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                        showInitialMenu(player, creature);
                        return;
                    }
                }

                // Check the # of charges used by the player for this code
                if (chargesUsed < charges)
                {
                    // Add the entry to the database
                    WorldDatabase.Query("INSERT INTO lootcode_player (code, playerGUID, playerName, isUnique) VALUES ('{}', {}, '{}', {});", (code.c_str()), player->GetGUID().GetCounter(), player->GetName().c_str(), isUnique);

                    // Add Item to player inventory
                    if (itemId != 0)
                        player->AddItem(itemId, quantity);

                    // Add Gold to player inventory
                    if (gold != 0)
                        player->ModifyMoney(gold * 10000);

                    // Customize Character
                    if (customize != 0)
                    {
                        // Faction
                        if (customize == CUSTOMIZE_FACTION)
                        {
                            Player* target = player;
                            ObjectGuid targetGuid = target->GetGUID();
                            std::string targetName = target->GetName();

                            auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);

                            stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_FACTION));
                            if (target)
                            {
                                target->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                                stmt->setUInt32(1, target->GetGUID().GetCounter());
                            }
                            else
                                stmt->setUInt32(1, targetGuid.GetCounter());

                            std::ostringstream messageCode;
                            messageCode << "You can change your faction on your next login " << target->GetName() << ".";
                            creature->MonsterWhisper(messageCode.str().c_str(), target);
                            CharacterDatabase.Execute(stmt);
                        }

                        // Race
                        if (customize == CUSTOMIZE_RACE)
                        {
                            Player* target = player;
                            ObjectGuid targetGuid = target->GetGUID();
                            std::string targetName = target->GetName();

                            auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);

                            stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_RACE));
                            if (target)
                            {
                                target->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                                stmt->setUInt32(1, target->GetGUID().GetCounter());
                            }
                            else
                                stmt->setUInt32(1, targetGuid.GetCounter());

                            std::ostringstream messageCode;
                            messageCode << "You can change your race on your next login " << target->GetName() << ".";
                            creature->MonsterWhisper(messageCode.str().c_str(), target);
                            CharacterDatabase.Execute(stmt);
                        }

                        // Name
                        if (customize == CUSTOMIZE_RENAME)
                        {
                            Player* target = player;
                            ObjectGuid targetGuid = target->GetGUID();
                            std::string targetName = target->GetName();

                            if (target)
                                target->SetAtLoginFlag(AT_LOGIN_RENAME);
                            else
                            {
                                auto* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
                                stmt->setUInt16(0, uint16(AT_LOGIN_RENAME));
                                stmt->setUInt32(1, targetGuid.GetCounter());
                                CharacterDatabase.Execute(stmt);
                            }

                            std::ostringstream messageCode;
                            messageCode << "You can rename your character on your next login " << target->GetName() << ".";
                            creature->MonsterWhisper(messageCode.str().c_str(), target);
                        }
                    }
                }
                else
                {
                    // Code charges exceeded
                    std::ostringstream messageCode;
                    messageCode << "Sorry, " << player->GetName() << ". you've reached the limit on this code.";
                    player->PlayDirectSound(9638); // No
                    creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                    creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                    showInitialMenu(player, creature);
                    return;
                }
            }

        } while (checkCode->NextRow());

        // Code successfully redeemed
        std::ostringstream messageCode;
        messageCode << "Your code has been redeemed " << player->GetName() << ". Have a nice day!";
        creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
        creature->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
        CloseGossipMenuFor(player);
    }

    // Passive Emotes
    struct Codebox_PassiveAI : public ScriptedAI
    {
        Codebox_PassiveAI(Creature * creature) : ScriptedAI(creature) { }

        uint32 Choice;
        uint32 MessageTimer;

        // Called once when client is loaded
        void Reset() override
        {
            MessageTimer = urand(CodeboxMessageTimer, 300000); // 1-5 minutes
        }

        // Called at World update tick
        void UpdateAI(const uint32 diff) override
        {
            // If Enabled
            if (CodeboxMessageTimer != 0)
            {
                if (MessageTimer <= diff)
                {

                    // Make a random message choice
                    Choice = urand(1, 3);

                    switch (Choice)
                    {
                    case 1:
                    {
                        me->MonsterSay("Do you have a code to redeem? Step right up!", LANG_UNIVERSAL, NULL);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                        MessageTimer = urand(CodeboxMessageTimer, 180000);
                        break;
                    }
                    case 2:
                    {
                        me->MonsterSay("Did you receive a loot code in the mail? I can help.", LANG_UNIVERSAL, NULL);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                        MessageTimer = urand(CodeboxMessageTimer, 180000);
                        break;
                    }
                    case 3:
                    {
                        me->MonsterSay("Did you find a secret code? I can help.", LANG_UNIVERSAL, NULL);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                        MessageTimer = urand(CodeboxMessageTimer, 180000);
                        break;
                    }
                    default:
                    {
                        me->MonsterSay("Do you have a code to redeem? Step right up!", LANG_UNIVERSAL, NULL);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                        MessageTimer = urand(CodeboxMessageTimer, 180000);
                        break;
                    }
                    }
                }
                else
                    MessageTimer -= diff;
            }
        };
    };

    // CREATURE AI
    CreatureAI * GetAI(Creature * creature) const override
    {
        return new Codebox_PassiveAI(creature);
    }
};

void AddNPCCodeboxScripts()
{
    new CodeboxConfig();
    new CodeboxAnnounce();
    new codebox_npc();
}
