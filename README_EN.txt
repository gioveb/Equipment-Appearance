EQUIPMENT APPEARANCE 0.2.1
Dragon Age: Origins (Windows)

Helmet visibility, HEELS, Full Body robes and optional weapon scaling.

REQUIREMENTS
- Dragon Age: Origins 1.05 for Windows.
- DAOLib with compatible plugin ABI 2 and matching Address Library resources.
  Development baseline: DAOLib 0.5.0. DAOLib 0.11.0 was checked statically:
  ABI and address files remain compatible; a full in-game test with that loader
  is not yet recorded. Follow DAOLib's own installation instructions.
- Extended Variations is optional and separate; retain it for extended-ID items.
- "No Helmet Hack" is not required.

INSTALLATION (MANUAL ZIP, NOT DAZIP)
Close DAO and the Toolset first.
1. Back up your existing equipment_appearance plugin folder outside
   bin_ship/daolib/plugins, especially your INI and custom scale-table edits.
2. Remove old standalone heels_probe and robe_fullbody plugin folders from
   bin_ship/daolib/plugins. Do not run them alongside this unified plugin.
3. Copy the CONTENTS of GameFiles into your DAO installation directory.
   Merge bin_ship with the existing bin_ship. The final plugin folder is:
   <game>/bin_ship/daolib/plugins/equipment_appearance/
   It contains the DLL, INI, JSON scale table and JSON schema.
   Do not put the DLL in Documents override.
4. DocumentsFiles contains support resources. Copy its CONTENTS into your
   active Documents/BioWare/Dragon Age directory as applicable:
   a) packages/core/override/Extended_Variations/var_item.gda
      Full Toolset variable table: 13 native entries plus EV_MODEL_ID (INT)
      and HEELS (FLOAT). Keep ONE authoritative copy. If another mod expands
      this table, merge its entries instead of overwriting them. Check older
      plugin-local overrides for duplicate copies. Restart the Toolset.
      The DLL reads existing item variables directly; this table exposes them
      for authoring new items in the Toolset.
   b) packages/core/override/zz_Templar_BaseItem22_Fix/gen_im_arm_cht_mas_tmp.uti
      Compatibility fix for the vanilla Templar outfit with Full Body enabled.
      Changes BaseItem 12 to 22, retaining variation 8 and separate glove models.
      Other mods editing this UTI need the same change merged into their file.
      This does not patch the _a/_b variants and is optional if Full Body is off.
GameFiles and DocumentsFiles are destination labels, not folders to copy intact.

UPDATING FROM 0.1.0 / 0.2.0 / TEST BUILDS
Replace the DLL and supplied scale table/schema in the same plugin folder.
Use the new commented INI and reapply your preferred values; preserve and merge
any personal JSON entries. Keep only one Equipment Appearance DLL.
Unchanged matching support resources need not be reinstalled. No save conversion
is required. Save after changing a character's helmet visibility preference.
If migrating from "No Helmet Hack", restore helmets with its book before removing
that mod: Equipment Appearance does not recover items in another equipment slot.

FEATURES AND DEFAULTS
HELMETS: click the eye beside the selected character's name above the quickbar.
The choice belongs to that character and does not unequip the helmet or alter
its stats. ShowInDialogues=1 respects that choice in supported dialogue/cutscene
scene filtering. Version 0.2.1 fixes transition gaps and actors outside the
current hero/speaker pair. Inventory always shows equipped helmets.
PartyPickerVisibility=1 makes companion selection follow the character choice;
0 leaves the game's native helmet behaviour in that room.
SaveCharacterPreference=1 saves EA_HIDE_HELMET as a character-local INT.
No var_creature.gda is required. Set it to 0 for session-only choices.

HEELS: supported boots and rob-type torso items may contain HEELS, a FLOAT in
centimetres (5.0 means 5 cm). The mesh must already be authored for heels, with
its sole below the original ground plane by the intended lift. The plugin only
raises the visual model; it does not reshape footwear or move gameplay position.
Rob torso height takes priority where applicable; heights are not added.
World, supported dialogues and party selection are supported. Dialogue camera
compensation is enabled. Inventory lift defaults OFF to avoid preview jumps.

FULL BODY: eligible rob-type variations in armour BaseItems 9/10/11/12 suppress
separate glove/boot visuals when the torso supplies a complete outfit. Those
items remain equipped with their stats. Extended IDs use Extended Variations
when available. The plugin does not convert an ordinary mesh into a full outfit.

WEAPON SCALING: table and creature factors are independent.
TableEnabled=1 (default) uses dalls_weapon_scales.json, version 1.3.3, 99 entries.
CreatureEnabled=0 (default); enable to use configurable HF/HM/EF/EM/DF/DM/QF/QM
factors. Each factor accepts 0.05 to 10.0. Known darkspawn models ignore the
creature factor by default; their table factor still applies.
UnknownWeaponScale=1.0 supplies a table-side fallback for resolved unlisted or
unknown-hash meshes. Disabled entries and unresolved names are excluded.
Scaling preserves the model's local origin and affects visuals only, not damage,
reach or collision. Reference hashes distinguish supported mesh versions.
See USAGE_NOTES_EN.txt for examples and custom table details.

All main feature switches default ON; creature scaling and inventory lift OFF.
Weapon diagnostics default ON as in the tested configuration; helmet diagnostics
OFF. The INI explains every shipped parameter. Close/restart DAO after edits.

VALIDATION / LIMITS
The user confirmed the Captured! transition sequence with helmets visible and
hidden, plus preference persistence after save/load. Earlier tests confirmed
HEELS, Full Body, party-selection helmets, stable weapon scaling and I/II swaps,
darkspawn exclusion and UnknownWeaponScale.
GOG/Steam executable layouts were compared; this is not testing every store
build in game. EA gameplay with the unified DLL remains unverified.
Not every scripted scene, weapon/projectile or rendered LOD is certified.
DAOLib, DAFix, game files, weapon meshes and custom outfits are not bundled.
Light Caps Scalp Fix and custom Morrigan robe fixes are separate resource mods.

LOGS / TROUBLESHOOTING
Documents/BioWare/Dragon Age/daolib/equipment_appearance.log
Previous session: equipment_appearance.previous.log
Also check DAOLib's loader.log for successful plugin loading.
For helmet reports, set [HelmetDiagnostics] Enabled=1, restart, reproduce the
issue, and include both logs, your INI, game build and relevant item/mod names.

UNINSTALL
Close DAO and remove bin_ship/daolib/plugins/equipment_appearance.
Restore backups of support resources as appropriate. Do not remove a shared
var_item.gda still required by another mod. Saved preferences are inert without
the plugin. Keep DAOLib and Extended Variations if other mods require them.

CREDITS / SOURCE
DAOLib and DAOAddressLibrary: Whocares66-Dev. Weapon comparison reference: Dalls
Rescaled Weapons. Table factors are nominal estimates from geometry comparison,
not recovered author settings or a byte-exact recreation. Custom factors are
identified separately. No Dalls model files are distributed here.
Source is supplied separately with dependency sources and licence texts.
Plugin source headers carry GPL-3.0-or-later notices; third-party components
retain their own licences. See LICENSES and the source package.
