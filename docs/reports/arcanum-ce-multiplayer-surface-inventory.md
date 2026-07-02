<!-- markdownlint-disable MD013 -->

# Arcanum CE Multiplayer Surface Inventory

Date: 2026-07-02

Scope: Phase 0 documentation only. This report maps the dormant multiplayer and
network compatibility surface currently present in the repository. It does not
implement multiplayer, does not edit `src/net_compat.h`, and does not change
engine source.

Do not implement yet:

- no multiplayer transport
- no `net_compat.h` edits
- no `tig/net.c` replacement
- no config gate for multiplayer
- no CE overlay or DAT work
- no QoL work

## Source Files Reviewed

- `src/net_compat.h`
- `src/game/multiplayer.c`
- `src/game/multiplayer.h`
- `src/game/mp_utils.c`
- `src/game/mp_utils.h`
- call sites found under `src/game/` and `src/ui/`

## Current Compile-Time Boundary

`src/net_compat.h` is a compatibility shim. In the current default build,
`tig_net_is_active()` is a constant `0`, `tig_net_is_host()` is a constant `0`,
most send/register calls are empty macros, and server options read as `0`.

That means the multiplayer branches remain compiled but are dormant in default
single-player execution. This default must remain unchanged until a later
multiplayer phase is explicitly opened.

Do not implement yet: keep `tig_net_is_active()` constant false in default
builds.

## `net_compat.h` Macro Call-Site Map

Definitions are in `src/net_compat.h`. Call sites exclude the defining header.

### `TIG_NET_SERVER_PLAYER_KILLING`

- `src\game\multiplayer.c`: 2904

### `TIG_NET_SERVER_FRIENDLY_FIRE`

- `src\game\multiplayer.c`: 2931

### `TIG_NET_SERVER_AUTO_EQUIP`

- `src\game\multiplayer.c`: 2516

### `TIG_NET_SERVER_KEY_SHARING`

- `src\game\item.c`: 1724

### `tig_net_is_active`

- `src\game\anim.c`: 2806, 2825, 3788, 4000, 4211, 4273, 4286, 4626, 4658, 4722, 4814, 4872, 4935, 4997, 5021, 5395, 5885, 5967, 6172, 6573, 6588, 6592, 6598, 6616, 6620, 6626, 7333, 7884, 8041, 8104, 8134, 8164, 8194, 8204, 8235, 8268, 8281, 9826, 9887, 11120, 11188, 11325, 11356, 11414, 11522, 11586, 13004, 13069, 13305, 13360, 13417, 13483, 13535, 13656, 13735, 14009, 14172, 14214, 14259, 14518, 14872, 15035, 15090, 15123
- `src\game\item.c`: 623, 718, 1723, 1772, 2096, 2193, 2481, 3141, 3927, 3975, 4127, 4156
- `src\game\magictech.c`: 1543, 1863, 1942, 1995, 2310, 2498, 2508, 2635, 2667, 2717, 2725, 3846, 4592, 4670, 4822, 4847, 4870, 5112, 5266, 5272, 5299
- `src\game\mp_utils.c`: 32, 47, 91, 111, 135, 143, 180, 215, 241, 255, 281, 410, 432, 450, 467, 517, 547, 552, 579
- `src\game\multiplayer.c`: 492, 540, 1131, 1155, 1175, 1342, 2259, 2298
- `src\game\party.c`: 95, 123, 152, 176, 228, 267, 282
- `src\game\script.c`: 292, 2406, 2445, 2973
- `src\game\target.c`: 676, 708, 773, 805, 1548
- `src\game\timeevent.c`: 825, 903
- `src\ui\intgame.c`: 2293, 3558, 3576, 3642, 3649
- `src\ui\inven_ui.c`: 482, 2553, 4080, 4152, 4180, 4210, 4240, 4248, 4271, 4295, 4328, 4346
- `src\ui\tb_ui.c`: 345
- `src\ui\wmap_ui.c`: 1254, 2197, 4821

### `tig_net_is_host`

- `src\game\anim.c`: 3789, 4006, 4212, 4270, 4274, 4287, 4627, 4659, 4723, 4815, 4936, 5022, 5396, 5886, 5968, 6173, 6574, 6589, 6599, 6617, 6627, 7334, 7885, 8042, 8105, 8135, 8165, 8195, 8205, 8236, 8269, 8282, 11415, 11523, 11587, 13005, 13028, 13070, 13306, 13368, 13387, 13418, 13484, 13543, 13569, 13664, 13684, 13743, 13763, 14873, 15036, 15054, 15126, 15144
- `src\game\item.c`: 624, 719, 2097, 2482, 3142, 3928, 3976, 4127
- `src\game\magictech.c`: 1546, 1864, 1943, 1995, 2311, 2333, 2499, 2636, 3846, 3992, 4593, 4671, 4822, 4847, 4870, 5113, 5266, 5300
- `src\game\mp_utils.c`: 33, 48, 92, 95, 112, 136, 144, 160, 181, 215, 242, 255, 282, 369, 378, 387, 396, 432, 518, 535, 548, 553, 580
- `src\game\multiplayer.c`: 1003, 1061, 1079, 1350, 1531
- `src\game\party.c`: 268, 283
- `src\game\script.c`: 293
- `src\game\target.c`: 305, 370
- `src\game\timeevent.c`: 826, 904
- `src\ui\inven_ui.c`: 482, 4153, 4180, 4210, 4240, 4248, 4271, 4295, 4328, 4346
- `src\ui\wmap_ui.c`: 2198

### `tig_net_send_app_all`

- `src\game\anim.c`: 4014, 4023, 4311, 8146, 8216, 8293, 13037, 13325, 13391, 13432, 13498, 13573, 13688, 13767, 15058, 15148
- `src\game\item.c`: 631, 725, 3934, 4135
- `src\game\magictech.c`: 1555, 1879, 2316, 2345, 2516, 2647, 2894, 3854, 3963, 4004, 4596, 4681, 4693, 5280
- `src\game\mp_utils.c`: 39, 58, 99, 115, 151, 186, 222, 247, 262, 287, 382, 416, 458, 474, 526, 558, 583, 609
- `src\game\multiplayer.c`: 881, 1011, 1031, 1064, 1082, 1377, 1556, 2272, 2319, 2399, 2467, 2480
- `src\game\party.c`: 273
- `src\game\timeevent.c`: 913
- `src\ui\inven_ui.c`: 491, 4154

### `tig_net_send_app`

- `src\game\mp_utils.c`: 82, 202, 373, 392, 440

### `tig_net_send_app_except`

- none

### `tig_net_local_client_set_name`

- `src\game\multiplayer.c`: 378, 458

### `tig_net_local_server_set_max_players`

- `src\game\multiplayer.c`: 382, 459

### `tig_net_local_server_set_description`

- `src\game\multiplayer.c`: 386, 460, 733

### `tig_net_local_server_set_name`

- `src\game\multiplayer.c`: 457

### `tig_net_start_client`

- `src\game\multiplayer.c`: 618

### `tig_net_on_message`

- `src\game\multiplayer.c`: 623, 637

### `tig_net_on_message_validation`

- `src\game\multiplayer.c`: 624

### `tig_net_on_network_event`

- `src\game\multiplayer.c`: 625

### `sub_5280F0`

- `src\game\multiplayer.c`: 638

### `tig_net_start_server`

- `src\game\multiplayer.c`: 647

### `sub_52A940`

- `src\game\multiplayer.c`: 757, 944, 1404

### `sub_52A950`

- `src\game\multiplayer.c`: 760, 959, 1415

### `sub_52B210`

- `src\game\multiplayer.c`: 777

### `sub_5286E0`

- `src\game\multiplayer.c`: 779

### `tig_net_xfer_count`

- `src\game\multiplayer.c`: 865, 884

### `tig_net_client_is_active`

- `src\game\multiplayer.c`: 891, 1105, 1120, 1160, 1180, 1408

### `tig_net_client_is_waiting`

- `src\game\multiplayer.c`: 892

### `tig_net_client_is_loading`

- `src\game\multiplayer.c`: 893

### `sub_52A9E0`

- `src\game\multiplayer.c`: 919

### `tig_net_local_server_get_max_players`

- `src\game\multiplayer.c`: 1006

### `tig_net_xfer_send_as`

- `src\game\multiplayer.c`: 1445, 1513

### `tig_net_xfer_send`

- `src\game\multiplayer.c`: 1479

### `sub_529520`

- `src\game\multiplayer.c`: 1501, 1553, 1554
- `src\ui\inven_ui.c`: 4078

### `tig_net_reset_connection`

- `src\game\multiplayer.c`: 1545

### `tig_net_local_server_get_options`

- `src\game\item.c`: 1724
- `src\game\multiplayer.c`: 2516, 2904, 2931

### `sub_52A530`

- `src\game\mp_utils.c`: 373, 392

### `sub_52A900`

- `src\game\timeevent.c`: 767

### `tig_net_client_info_get_name`

- `src\game\multiplayer.c`: 837

## Multiplayer Systems Still Present

### Lifecycle and State

- `multiplayer_init`, `multiplayer_exit`, and `multiplayer_reset` still manage
  player directories, multiplayer tables, message resources, player slots, and
  default local client/server names.
- `multiplayer_save` and `mutliplayer_load` still serialize multiplayer-only
  state behind `tig_net_is_active()`.
- `multiplayer_mod_load` is currently trivial. `multiplayer_mod_unload` still
  locks and clears temporary multiplayer objects.
- `multiplayer_start` still registers message, validation, and network-event
  callbacks through the shim.
- `multiplayer_end` still unregisters message handling and resets lock state.

Do not implement yet: the registration calls are inventory only while the shim
macros are no-ops.

### Module, Map, and Session Startup

- `sub_49CC50` calls the server-start shim.
- `sub_49CC70` still contains module DAT probing, module load fallback,
  multiplayer description setup, start-map loading, initial player creation, and
  player portrait/profile file staging into `data\temp`.
- `multiplayer_start_play` still creates or loads player data, normalizes PC
  runtime state, applies auto-equip logic, and schedules teleport events.
- `multiplayer_map_open_by_name` still adds a map DAT repository if present,
  flushes and opens maps, and validates the object system before and after map
  load.

Do not implement yet: do not wire this to a real transport until the later
multiplayer phase.

### Message Dispatch and Validation

- `multiplayer_start` registers:
  - `multiplayer_handle_message`
  - `multiplayer_validate_message`
  - `multiplayer_handle_network_event`
- `multiplayer_validate_message` accepts only integer packet types greater than
  `0` and less than `131`.
- `multiplayer_handle_message` is still marked incomplete.
- `multiplayer_handle_network_event` is still marked incomplete.

Do not implement yet: the handler gaps are findings for a future design doc,
not permission to add dispatch code in Phase 0.

### Player Slots and Max-Player Handling

- `NUM_PLAYERS` is hard-coded to `8` in `multiplayer.c`.
- Slot arrays use `NUM_PLAYERS`: `stru_5E8AD0`, `qword_5E8D78`,
  `multiplayer_level_scheme_tbl`, `off_5F0BC8`, and `qword_5F0E20`.
- `tig_net_local_server_set_max_players(NUM_PLAYERS)` is called during init and
  reset.
- `tig_net_local_server_get_max_players()` currently returns `8` from the shim
  and controls player-list packet population.
- Several loops still use literal `8` instead of `NUM_PLAYERS`, notably
  transfer and active-client loops.

Do not implement yet: any future player-limit change needs a separate design
and compatibility pass.

### File Transfer and Save Handoff

- Multiplayer time events still wait for transfer counts, send current map
  metadata, copy player files, and reset multiplayer state.
- `sub_4A33F0` writes `xferdata.mp`, calls registered save callbacks, sends map
  files, and schedules transfer completion events.
- `sub_4A3660` sends staged player files from `data\temp\Players`.
- `sub_4A3780` copies the local player's `.mpc`, `.bmp`, and `_b.bmp` files and
  sends them as transfer payloads.
- `save_char`, `load_char`, `sub_4A4320`, and `sub_4A6470` still implement
  multiplayer character-file persistence surfaces.

### Object, Item, and Inventory Locking

- `sub_4A2EC0`, `sub_4A3030`, `sub_4A3080`, `sub_4A30D0`, and `sub_4A3170`
  still model multiplayer item/object locks with `OF_MULTIPLAYER_LOCK`.
- `sub_4A3230` still wraps item lock requests with success/failure callbacks
  and a packet path.
- `sub_4A50D0`, `sub_4A51C0`, `sub_4A52C0`, and `sub_4A5320` still hide and
  unhide items per client using `OIF_NO_DISPLAY`.
- `sub_4A53B0` and `sub_4A5460` still track object-to-client relationships.

### Player Metadata and Level Schemes

- `multiplayer_level_scheme_set`, `multiplayer_level_scheme_get`,
  `multiplayer_notify_level_scheme_changed`,
  `multiplayer_level_scheme_rule`, and `multiplayer_level_scheme_name` still
  store and retrieve per-player auto-level scheme metadata.
- `multiplayer_portrait_path` still searches `Players`, `data\temp\Players`,
  and `Save\Current\Players` for player portrait files.

### Multiplayer Flags and Settings

- `MULTIPLAYER_ALWAYS_RUN`, `MULTIPLAYER_AUTO_ATTACK`,
  `MULTIPLAYER_AUTO_SWITCH_WEAPONS`, and `MULTIPLAYER_FOLLOWER_SKILLS` remain
  defined in `multiplayer.h`.
- `multiplayer_flags_set`, `multiplayer_flags_unset`, and
  `multiplayer_flags_get` still replicate high-byte client flags.
- `sub_4A5610` still reads current local settings for always-run,
  auto-attack, auto-switch weapons, and follower skills.

### Combat and Server Options

- `TIG_NET_SERVER_AUTO_EQUIP` gates auto-equip setup in `sub_4A5670`.
- `TIG_NET_SERVER_PLAYER_KILLING` and `TIG_NET_SERVER_FRIENDLY_FIRE` gate the
  friendly-fire/player-killing prevention helper `sub_4A6190`.
- `TIG_NET_SERVER_KEY_SHARING` is checked in `item.c`.
- Because `tig_net_local_server_get_options()` currently returns `0`, default
  builds behave as if all server options are disabled.

### `mp_utils.c` Helper Systems

- Object create and proto helper path: `mp_object_create`.
- Object script/action path: `sub_4ED6C0`.
- UI message and inventory update path: `sub_4EDA60`,
  `mp_ui_update_inven`, `mp_handle_ui_update_inven`.
- Object movement/origin path: `sub_4EDF20`, `sub_4EDFF0`.
- Item activation and written/inventory UI path: `mp_item_activate`,
  `mp_ui_written_start_type`, `mp_ui_show_inven_loot`, `sub_4EE3A0`,
  `mp_ui_show_inven_identify`, `sub_4EE4C0`, `sub_4EE5E0`.
- Sound and music path: `mp_gsound_play_sfx`, `sub_4EED00`,
  `mp_gsound_play_sfx_on_obj`, `mp_gsound_play_scheme`,
  `mp_handle_gsound_play_scheme`.
- Miscellaneous still-present helpers: `sub_4EF830`, `sub_4EF8B0`,
  `sub_4EFAE0`, `sub_4EFB50`, `sub_4EFBA0`, `sub_4EFC30`,
  `sub_4F0640`, and `sub_4F0690`.
- Incomplete placeholders remain: `sub_4EF320`, `sub_4EF3D0`,
  `sub_4EF540`, `sub_4EFBE0`, and `sub_4F05F0`.

## Message Types

Observed packet senders and packet structs:

| Type | Packet struct | Observed sender or handler surface |
| --- | --- | --- |
| 2 | `PacketGamePlayerList` | `multiplayer_send_player_list` |
| 3 | `PacketGameTime` | `timeevent.c` game/animation time broadcast |
| 4 | `Packet4` | `anim.c` animation/location sync, subtypes 0, 1, 2 |
| 5 | `Packet5` | defined, no observed sender in this pass |
| 6 | `Packet6` | `anim.c` magic/animation packet, subtype 0 observed |
| 7 | `Packet7` | defined, no observed sender in this pass |
| 8 | `Packet8` | `anim.c` animation modify/update sync |
| 9 | `Packet9` | `anim.c` follower/animation info sync |
| 10 | `Packet10` | `anim.c` object art/location update sync |
| unknown | `PacketCombatModeSet` | defined, no observed sender in this pass |
| 26 | `Packet26` | `item.c` item state packet |
| 27 | `Packet27` | `item.c` item location packet |
| 28 | `Packet28` | `item.c` item transfer/inventory packet |
| 29 | `Packet29` | `mp_utils.c` object action packet |
| 46 | `Packet46` | `multiplayer.c` disconnect/leave flow |
| 54 | `Packet54` | `magictech.c` magictech id packet |
| 57 | `PacketPlayerRequestCastSpell` | `magictech.c` player spell request |
| 58 | `PacketPlayerCastSpell` | `magictech.c` spell cast broadcast |
| 60 | `PacketPlayerSpellMaintainAdd` | `magictech.c` maintained spell add |
| 61 | `PacketPlayerSpellMaintainEnd` | `magictech.c` maintained spell end |
| 64 | `Packet64` | `multiplayer_timeevent_process` map/player transfer notification |
| 67 | `Packet67` | `sub_4A2040` small multiplayer notification |
| 68 | `PacketNotifyPlayerLagging` | `multiplayer_notify_player_lagging` |
| 69 | `PacketNotifyPlayerRecovered` | `multiplayer_notify_player_recovered` |
| 70 | `Packet70` | `mp_object_create`, `anim.c`; subtypes 0 and 5 observed |
| 71 | `PacketPartyUpdate` | `party.c` party update |
| 72 | `PacketObjectDestroy` | `magictech.c` object destroy |
| 73 | `PacketSummon` | `magictech.c` summon |
| 74 | `Packet74` | `magictech.c`; subtypes 0, 1, 2 observed |
| unassigned | `PacketMagicTechObjFlag` | `magictech.c` has a send path, but this pass did not find a `pkt.type` assignment in that block |
| 76 | `Packet76` | `magictech.c` cost/magictech packet |
| 77 | `PacketMagicTechEyeCandy` | `magictech.c` eye-candy packet, subtype 0 observed |
| 80 | `Packet80` | `sub_4A3230` item-lock callback request |
| 81 | `Packet81` | `inven_ui.c` inventory UI packet |
| 82 | `Packet82` | defined, no observed sender in this pass |
| 83 | `Packet83` | defined, no observed sender in this pass |
| 84 | `Packet84` | `sub_4EDA60` variable-length UI message |
| 92 | `Packet92` | `sub_4EDCE0` art-id update |
| 93 | `Packet93` | `sub_4A50D0`, `sub_4A51C0` item hide/unhide |
| 94 | `PacketUpdateInven` | `mp_ui_update_inven` |
| 97 | `Packet97` | `sub_4A53B0` object relation update |
| 98 | `PacketMultiplayerFlagsChange` | `multiplayer_flags_set`, `multiplayer_flags_unset` |
| 99 | `Packet99` | `sub_4EDF20` movement/origin update |
| 100 | `Packet100` | UI operations; subtypes 3, 5, 6, 7, 8, 10, 12 observed |
| 104 | `Packet104` | defined, no observed sender in this pass |
| 106 | `PacketPlaySound` | sound/music packet; subtypes 0, 1, 2 observed |
| 118 | `Packet118` | `sub_4EF830` object relation/action packet |
| 121 | `Packet121` | `sub_4EFAE0` object update packet |
| 122 | `Packet122` | `sub_4EFBA0` client-to-host request packet |
| 123 | `Packet123` | `sub_4EFC30` auto-level scheme text packet |
| 125 | `PacketPerformIdentifyService` | `item.c` identify-service packet |
| unknown | `PacketPerformRepairService` | defined, no observed sender in this pass |
| 128 | `Packet128` | defined, no observed sender in this pass |

Validation callback contract:

- The current validation callback checks only the first integer packet type.
- Accepted range is `1..130`.
- It does not validate packet size, sender authority, subtype, object handles,
  or payload-specific invariants yet.

Do not implement yet: future transport work must design validation before any
LAN milestone, but Phase 0 records only the current contract.

## Compile-Time Assumptions and Risks

- `NUM_PLAYERS` is fixed at `8`.
- `tig_net_local_server_get_max_players()` also returns `8`.
- `tig_net_is_active()` and `tig_net_is_host()` are constants in the current
  shim.
- Packet structs use `static_assert` sizes to preserve serialized layout.
- `PacketSummon` and `Packet84` have comments noting wrong x64 network size, so
  x64 transport work cannot treat the current structs as fully proven.
- Some systems still use literal `8` instead of `NUM_PLAYERS`.
- Message and network-event dispatch are explicitly incomplete.
- Several helpers still have decompiled names, so a future implementation needs
  a naming pass only after behavior is proven.

## Phase 0 Decision

This report is a map, not an implementation approval. The next multiplayer work
should be a Phase 6 design document after the earlier reliability/config/overlay
phases land. The near-term next implementation should stay in Phase 1 and avoid
multiplayer transport entirely.
