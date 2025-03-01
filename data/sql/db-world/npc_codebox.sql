-- ######################################################--
--	NPC CODEBOX TABLES
-- ######################################################--

SET FOREIGN_KEY_CHECKS=0;

-- --------------------------------------------------------------------------------------
--	CODEBOX - 601021
-- --------------------------------------------------------------------------------------
SET
@Entry 		:= 601021,
@Model 		:= 16804, -- Elven Jeweler
@Name 		:= "Retdream",
@Title 		:= "Keeper of Codes",
@Icon 		:= "Buy",
@GossipMenu := 0,
@MinLevel 	:= 80,
@MaxLevel 	:= 80,
@Faction 	:= 35,
@NPCFlag 	:= 1,
@Scale		:= 1.0,
@Rank		:= 0,
@Type 		:= 7,
@TypeFlags 	:= 138936390,
@FlagsExtra := 2,
@AIName		:= "",
@Script 	:= "codebox_npc";

-- NPC
DELETE FROM creature_template WHERE entry = @Entry;
INSERT INTO creature_template (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `unit_class`, `unit_flags`, `type`, `type_flags`, `RegenHealth`, `flags_extra`, `AiName`, `ScriptName`) VALUES
(@Entry, @Name, @Title, @Icon, @GossipMenu, @MinLevel, @MaxLevel, @Faction, @NPCFlag, 1, 1.14286, @Scale, @Rank, 1, 2, @Type, @TypeFlags, 1, @FlagsExtra, @AIName, @Script);

-- NPC MODEL
DELETE FROM `creature_template_model` WHERE `CreatureID` = @Entry;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(@Entry, 0, @Model, 1, 1, 0);

-- NPC Text
DELETE FROM `npc_text` WHERE `ID`=@Entry;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@Entry, 'Greetings $N. Do you have a loot code to redeem?');
DELETE FROM `npc_text` WHERE `ID`=@Entry+1;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@Entry+1, '[GM] Add Loot Code');
DELETE FROM `npc_text` WHERE `ID`=@Entry+18;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@Entry+18, '[GM] Delete Loot Code');
DELETE FROM `npc_text` WHERE `ID`=@Entry+28;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@Entry+28, '[GM] View Loot Codes');

-- --------------------------------------------------------------------------------------
-- Table structure for lootcode_items
-- --------------------------------------------------------------------------------------
DROP TABLE IF EXISTS `lootcode_items`;
CREATE TABLE `lootcode_items` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `code` varchar(32) DEFAULT NULL,
  `itemId` int(7) unsigned DEFAULT NULL,
  `name` varchar(255) DEFAULT NULL,
  `quantity` int(14) NOT NULL DEFAULT '1',
  `gold` int(14) unsigned DEFAULT NULL,
  `customize` int(14) unsigned DEFAULT '0',
  `charges` tinyint(5) DEFAULT '1',
  `isUnique` tinyint(1) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;

-- --------------------------------------------------------------------------------------
-- Default Records of lootcode_items
-- Q 	= Quantity
-- G 	= Gold
-- CU	= Customize
-- C	= Charges
-- U	= Is Unique Item
-- -------------------------------------------------------------------------------------------------------------------
--                                    ID     CODE        ITEMID   NAME             Q     G   CU    C    U
-- -------------------------------------------------------------------------------------------------------------------
INSERT INTO `lootcode_items` VALUES ('1', 'threebags', '21841', 'Netherweave Bag', '3', '0', '0', '1', '0');
INSERT INTO `lootcode_items` VALUES ('2', 'artifact', '4696', 'Lapidis Tankard of Tidesippe', '1', '0', '0','1', '1');
INSERT INTO `lootcode_items` VALUES ('3', 'ballroom', '3419', 'Red Rose', '1', '0', '0', '3', '0');
INSERT INTO `lootcode_items` VALUES ('4', 'ballroom', '6833', 'Tuxedo Shirt', '1', '0', '0', '3', '0');
INSERT INTO `lootcode_items` VALUES ('5', 'ballroom', '6835', 'Tuxedo Pants', '1', '0', '0', '3', '0');
INSERT INTO `lootcode_items` VALUES ('6', 'lockpick', '15869', 'Skeleton Key', '5', '0', '0', '3', '0');
INSERT INTO `lootcode_items` VALUES ('7', 'faction', '0', 'Faction Change', '1', '0', '1', '1', '0');
INSERT INTO `lootcode_items` VALUES ('8', 'race', '0', 'Race Change', '1', '0', '2', '1', '0');
INSERT INTO `lootcode_items` VALUES ('9', 'rename', '0', 'Rename Character', '1', '0', '3', '1', '0');

-- --------------------------------------------------------------------------------------
-- Table structure for lootcode_player
-- Comment this to avoid overwriting existing 'lootcore_player' data.
-- --------------------------------------------------------------------------------------
DROP TABLE IF EXISTS `lootcode_player`;
CREATE TABLE `lootcode_player` (
  `id` int(7) unsigned NOT NULL AUTO_INCREMENT,
  `code` varchar(255) DEFAULT NULL,
  `playerGUID` int(7) unsigned DEFAULT NULL,
  `playerName` varchar(100) DEFAULT NULL,
  `isUnique` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `redeemed` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
