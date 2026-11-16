// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor_data.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "colors.hpp"
#include "debug.hpp"
#include "item_att_property.hpp"
#include "item_data.hpp"
#include "misc.hpp"
#include "paths.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "saving.hpp"
#include "xml.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
template <typename K, typename V>
static std::vector<std::string> get_keys_sorted(
    const std::unordered_map<K, V>& m)
{
    std::vector<std::string> keys;
    keys.reserve(m.size());

    for (const auto& it : m) {
        keys.push_back(it.first);
    }

    std::sort(std::begin(keys), std::end(keys));

    return keys;
}

static const std::unordered_map<std::string, MonShockLvl> str_to_shock_lvl_map = {
    {"MONSHOCK_NONE", MonShockLvl::none},
    {"MONSHOCK_UNSETTLING", MonShockLvl::unsettling},
    {"MONSHOCK_FRIGHTENING", MonShockLvl::frightening},
    {"MONSHOCK_TERRIFYING", MonShockLvl::terrifying},
    {"MONSHOCK_MIND_SHATTERING", MonShockLvl::mind_shattering}};

static const std::unordered_map<std::string, actor::Speed> str_to_speed_map = {
    {"MONSPEED_SLOW", actor::Speed::slow},
    {"MONSPEED_NORMAL", actor::Speed::normal},
    {"MONSPEED_FAST", actor::Speed::fast},
    {"MONSPEED_VERY_FAST", actor::Speed::very_fast}};

static const std::unordered_map<std::string, actor::MonGroupSize> s_str_to_group_size_map = {
    {"MONGROUPSIZE_ALONE", actor::MonGroupSize::alone},
    {"MONGROUPSIZE_FEW", actor::MonGroupSize::few},
    {"MONGROUPSIZE_PACK", actor::MonGroupSize::pack},
    {"MONGROUPSIZE_SWARM", actor::MonGroupSize::swarm}};

static const std::unordered_map<std::string, actor::Size> s_str_to_actor_size_map = {
    {"MONSIZE_FLOOR", actor::Size::floor},
    {"MONSIZE_HUMANOID", actor::Size::humanoid},
    {"MONSIZE_GIANT", actor::Size::giant}};

static const std::unordered_map<actor::AiId, std::string> s_ai_id_to_tag_name_map = {
    {actor::AiId::looks, "looks"},
    {actor::AiId::avoids_blocking_friend, "avoids_blocking_friend"},
    {actor::AiId::attacks, "attacks"},
    {actor::AiId::paths_to_target_when_aware, "paths_to_target_when_aware"},
    {actor::AiId::moves_to_target_when_los, "moves_to_target_when_los"},
    {actor::AiId::moves_to_lair, "moves_to_lair"},
    {actor::AiId::moves_to_leader, "moves_to_leader"},
    {actor::AiId::moves_randomly_when_unaware, "moves_randomly_when_unaware"}};

static void dump_text(xml::Element* const e, actor::ActorData& data)
{
    if (xml::has_child(e, "name_a")) {
        data.name_a = xml::get_str(xml::child(e, "name_a"));
    }
    if (xml::has_child(e, "name_the")) {
        data.name_the = xml::get_str(xml::child(e, "name_the"));
    }
    if (xml::has_child(e, "corpse_name_a")) {
        data.corpse_name_a = xml::get_str(xml::child(e, "corpse_name_a"));
    }
    if (xml::has_child(e, "corpse_name_the")) {
        data.corpse_name_the = xml::get_str(xml::child(e, "corpse_name_the"));
    }
    if (xml::has_child(e, "description")) {
        data.descr = xml::get_str(xml::child(e, "description"));
    }
    if (xml::has_child(e, "allow_wield_description")) {
        data.allow_wielded_wpn_descr = xml::get_bool(xml::child(e, "allow_wield_description"));
    }
    if (xml::has_child(e, "allow_speed_description")) {
        data.allow_speed_descr = xml::get_bool(xml::child(e, "allow_speed_description"));
    }
    if (xml::has_child(e, "smell_message")) {
        data.smell_msg = xml::get_str(xml::child(e, "smell_message"));
    }
    if (xml::has_child(e, "wary_message")) {
        data.wary_msg = xml::get_str(xml::child(e, "wary_message"));
    }
    if (xml::has_child(e, "aware_message_seen")) {
        auto* aware_msg_seen_e = xml::child(e, "aware_message_seen");
        data.aware_msg_mon_seen = xml::get_str(aware_msg_seen_e);

        if (xml::has_attribute_bool(aware_msg_seen_e, "use_cultist_messages")) {
            data.use_cultist_aware_msg_mon_seen =
                xml::get_attribute_bool(aware_msg_seen_e, "use_cultist_messages");
        }
    }
    if (xml::has_child(e, "aware_message_hidden")) {
        auto* aware_msg_hidden_e = xml::child(e, "aware_message_hidden");
        data.aware_msg_mon_hidden = xml::get_str(aware_msg_hidden_e);

        if (xml::has_attribute_bool(aware_msg_hidden_e, "use_cultist_messages")) {
            data.use_cultist_aware_msg_mon_hidden =
                xml::get_attribute_bool(aware_msg_hidden_e, "use_cultist_messages");
        }
    }
    if (xml::has_child(e, "spell_message_sound")) {
        data.spell_msg_sound = xml::get_str(xml::child(e, "spell_message_sound"));
    }
    if (xml::has_child(e, "spell_message_visual")) {
        data.spell_msg_visual = xml::get_str(xml::child(e, "spell_message_visual"));
    }
    if (xml::has_child(e, "death_message_override")) {
        data.death_msg_override = xml::get_str(xml::child(e, "death_message_override"));
    }
}

static void dump_gfx(xml::Element* const e, actor::ActorData& data)
{
    if (xml::has_child(e, "tile")) {
        data.tile = gfx::str_to_tile_id(xml::get_str(xml::child(e, "tile")));
    }

    if (xml::has_child(e, "character")) {
        const std::string char_str = xml::get_str(xml::child(e, "character"));

        if (char_str.empty()) {
            data.character = 0;
        }
        else {
            ASSERT(char_str.length() == 1);

            data.character = char_str[0];
        }
    }

    if (xml::has_child(e, "color")) {
        std::string color_str = xml::get_str(xml::child(e, "color"));

        if (!color_str.empty()) {
            data.color = colors::name_to_color(color_str);
        }
    }
}

static void dump_audio(xml::Element* const e, actor::ActorData& data)
{
    if (xml::has_child(e, "aware_sfx_seen")) {
        data.aware_sfx_mon_seen =
            audio::str_to_sfx_id(xml::get_str(xml::child(e, "aware_sfx_seen")));
    }
    if (xml::has_child(e, "aware_sfx_hidden")) {
        data.aware_sfx_mon_hidden =
            audio::str_to_sfx_id(xml::get_str(xml::child(e, "aware_sfx_hidden")));
    }
}

static void dump_attributes(xml::Element* const e, actor::ActorData& data)
{
    if (xml::has_child(e, "hit_points")) {
        data.hp = xml::get_int(xml::child(e, "hit_points"));
    }
    if (xml::has_child(e, "spirit")) {
        data.spi = xml::get_int(xml::child(e, "spirit"));
    }
    if (xml::has_child(e, "speed")) {
        data.speed = str_to_speed_map.at(xml::get_str(xml::child(e, "speed")));
    }
    if (xml::has_child(e, "shock_level")) {
        data.mon_shock_lvl = str_to_shock_lvl_map.at(xml::get_str(xml::child(e, "shock_level")));
    }
    if (xml::has_child(e, "melee")) {
        data.ability_values.set(AbilityId::melee, xml::get_int(xml::child(e, "melee")));
    }
    if (xml::has_child(e, "ranged")) {
        data.ability_values.set(AbilityId::ranged, xml::get_int(xml::child(e, "ranged")));
    }
    if (xml::has_child(e, "dodging")) {
        data.ability_values.set(AbilityId::dodging, xml::get_int(xml::child(e, "dodging")));
    }
    if (xml::has_child(e, "stealth")) {
        data.ability_values.set(AbilityId::stealth, xml::get_int(xml::child(e, "stealth")));
    }
    if (xml::has_child(e, "searching")) {
        data.ability_values.set(AbilityId::searching, xml::get_int(xml::child(e, "searching")));
    }
    if (xml::has_child(e, "can_open_doors")) {
        data.can_open_doors = xml::get_bool(xml::child(e, "can_open_doors"));
    }
    if (xml::has_child(e, "can_bash_doors")) {
        data.can_bash_doors = xml::get_bool(xml::child(e, "can_bash_doors"));
    }
    if (xml::has_child(e, "size")) {
        data.actor_size = s_str_to_actor_size_map.at(xml::get_str(xml::child(e, "size")));
    }
    if (xml::has_child(e, "always_prevent_knockback")) {
        data.prevent_knockback = xml::get_bool(xml::child(e, "always_prevent_knockback"));
    }
    if (xml::has_child(e, "humanoid")) {
        data.is_humanoid = true;
    }
    if (xml::has_child(e, "rat")) {
        data.is_rat = true;
    }
    if (xml::has_child(e, "canine")) {
        data.is_canine = true;
    }
    if (xml::has_child(e, "spider")) {
        data.is_spider = true;
    }
    if (xml::has_child(e, "ghost")) {
        data.is_ghost = true;
    }
    if (xml::has_child(e, "ghoul")) {
        data.is_ghoul = true;
    }
    if (xml::has_child(e, "snake")) {
        data.is_snake = true;
    }
    if (xml::has_child(e, "reptile")) {
        data.is_reptile = true;
    }
    if (xml::has_child(e, "amphibian")) {
        data.is_amphibian = true;
    }
    if (xml::has_child(e, "can_bleed")) {
        data.can_bleed = xml::get_bool(xml::child(e, "can_bleed"));
    }
    if (xml::has_child(e, "can_leave_corpse")) {
        data.can_leave_corpse = xml::get_bool(xml::child(e, "can_leave_corpse"));
    }
    if (xml::has_child(e, "prioritize_destroying_corpse")) {
        data.prio_corpse_bash = xml::get_bool(xml::child(e, "prioritize_destroying_corpse"));
    }
}

static void dump_intr_attack_property(xml::Element* const e, actor::IntrAttData& attack_data)
{
    const auto prop_id = prop::str_to_prop_id(xml::get_str(e));

    attack_data.prop_applied.prop.reset(prop::make(prop_id));

    if (xml::has_attribute_int(e, "percent_chance")) {
        attack_data.prop_applied.pct_chance_to_apply = xml::get_attribute_int(e, "percent_chance");
    }

    if ((attack_data.prop_applied.pct_chance_to_apply <= 0) ||
        (attack_data.prop_applied.pct_chance_to_apply > 100)) {
        TRACE_ERROR_RELEASE
            << "Invalid attack property chance: "
            << attack_data.prop_applied.pct_chance_to_apply
            << "\n";

        PANIC;
    }

    if (xml::has_attribute_int(e, "duration")) {
        attack_data.prop_applied.prop->set_duration(xml::get_attribute_int(e, "duration"));
    }
    // Duration not specified as integer, check if string instead ("indefinite").
    else if (xml::has_attribute_str(e, "duration")) {
        if (xml::get_attribute_str(e, "duration") == "indefinite") {
            attack_data.prop_applied.prop->set_indefinite();
        }
    }
}

static void dump_items(xml::Element* const e, actor::ActorData& data)
{
    for (auto* item_set_e = xml::child(e); item_set_e; item_set_e = xml::next(item_set_e)) {
        actor::ActorItemSetData item_set;

        const std::string id_str = xml::get_str(item_set_e);

        item_set.item_set_id = item::str_to_item_set_id(id_str);

        if (xml::has_attribute_int(item_set_e, "percent_chance")) {
            item_set.pct_chance_to_spawn = xml::get_attribute_int(item_set_e, "percent_chance");
        }
        if (xml::has_attribute_int(item_set_e, "min")) {
            item_set.nr_spawned_range.min = xml::get_attribute_int(item_set_e, "min");
        }
        if (xml::has_attribute_int(item_set_e, "max")) {
            item_set.nr_spawned_range.max = xml::get_attribute_int(item_set_e, "max");
        }

        data.item_sets.push_back(item_set);
    }
}

static void dump_intr_attacks(xml::Element* const e, actor::ActorData& data)
{
    for (auto* attack_e = xml::child(e); attack_e; attack_e = xml::next(attack_e)) {
        auto attack_data = std::make_unique<actor::IntrAttData>();

        const std::string id_str = xml::get_attribute_str(attack_e, "id");

        attack_data->item_id = item::str_to_intr_item_id(id_str);

        auto* dmg_e = xml::child(attack_e);

        attack_data->dmg = xml::get_int(dmg_e);

        // Properties applied
        for (auto* prop_e = xml::next(dmg_e); prop_e; prop_e = xml::next(prop_e)) {
            dump_intr_attack_property(prop_e, *attack_data);
        }

        data.intr_attacks.push_back(std::move(attack_data));
    }
}

static void dump_spells(xml::Element* const e, actor::ActorData& data)
{
    for (auto* spell_e = xml::child(e); spell_e; spell_e = xml::next(spell_e)) {
        actor::ActorSpellData spell_data;

        const std::string id_str = xml::get_str(spell_e);

        spell_data.spell_id = spells::str_to_spell_id(id_str);

        const std::string skill_str = xml::get_attribute_str(spell_e, "skill");

        spell_data.spell_skill = spells::str_to_spell_skill_id(skill_str);

        if (xml::has_attribute_int(spell_e, "percent_chance")) {
            spell_data.pct_chance_to_know = xml::get_attribute_int(spell_e, "percent_chance");
        }

        data.spells.push_back(spell_data);
    }
}

static void dump_properties(xml::Element* const e, actor::ActorData& data)
{
    for (auto* prop_e = xml::child(e); prop_e; prop_e = xml::next(prop_e)) {
        const prop::Id prop_id = prop::str_to_prop_id(xml::get_str(prop_e));

        data.natural_props[(size_t)prop_id] = true;
    }
}

static void dump_ai(xml::Element* const e, actor::ActorData& data)
{
    if (xml::child(e, "erratic_move_percent")) {
        data.erratic_move_pct = xml::get_int(xml::child(e, "erratic_move_percent"));
    }
    if (xml::child(e, "turns_aware")) {
        data.nr_turns_aware = xml::get_int(xml::child(e, "turns_aware"));
    }
    if (xml::child(e, "ranged_cooldown_turns")) {
        data.ranged_cooldown_turns = xml::get_int(xml::child(e, "ranged_cooldown_turns"));
    }
    if (xml::child(e, "is_pausing_on_player_seen")) {
        data.is_pausing_on_player_seen = xml::get_bool(xml::child(e, "is_pausing_on_player_seen"));
    }

    for (size_t i = 0; i < (size_t)actor::AiId::END; ++i) {
        const std::string ai_id_str = s_ai_id_to_tag_name_map.at((actor::AiId)i);

        if (xml::child(e, ai_id_str)) {
            data.ai[i] = xml::get_bool(xml::child(e, ai_id_str));
        }
    }
}

static void dump_group_size(xml::Element* const e, actor::ActorData& data)
{
    actor::MonGroupSpawnRule rule;

    rule.group_size = s_str_to_group_size_map.at(xml::get_str(e));

    if (xml::has_attribute_int(e, "weight")) {
        rule.weight = xml::get_attribute_int(e, "weight");
    }

    if (xml::has_attribute_str(e, "min_dungeon_level")) {
        std::string required_dlvl_str = xml::get_attribute_str(e, "min_dungeon_level");

        if (required_dlvl_str[0] == '+') {
            // Relative to minimum spawn dungeon level.
            const int relative_dlvl = to_int(required_dlvl_str.substr(1));

            rule.required_dlvl = data.spawn_min_dlvl + relative_dlvl;
        }
        else {
            // Absolute value.
            rule.required_dlvl = to_int(required_dlvl_str);
        }
    }

    data.group_sizes.push_back(rule);
}

static void dump_native_room(xml::Element* const e, actor::ActorData& data)
{
    const auto room_type = room::str_to_room_type(xml::get_str(e));

    data.native_rooms.push_back(room_type);
}

static void dump_spawning(xml::Element* const e, actor::ActorData& data)
{
    if (xml::child(e, "min_dungeon_level")) {
        data.spawn_min_dlvl = xml::get_int(xml::child(e, "min_dungeon_level"));
    }
    if (xml::child(e, "max_dungeon_level")) {
        data.spawn_max_dlvl = xml::get_int(xml::child(e, "max_dungeon_level"));
    }
    if (xml::child(e, "spawn_weight")) {
        data.spawn_weight = xml::get_int(xml::child(e, "spawn_weight"));
    }
    if (xml::child(e, "auto_spawn")) {
        data.is_auto_spawn_allowed = xml::get_bool(xml::child(e, "auto_spawn"));
    }
    if (xml::child(e, "can_be_summoned_by_monster")) {
        data.can_be_summoned_by_mon = xml::get_bool(xml::child(e, "can_be_summoned_by_monster"));
    }
    if (xml::child(e, "can_spawn_from_tomb")) {
        data.can_spawn_from_tomb = xml::get_bool(xml::child(e, "can_spawn_from_tomb"));
    }
    if (xml::child(e, "can_be_shapeshifted_into")) {
        data.can_be_shapeshifted_into = xml::get_bool(xml::child(e, "can_be_shapeshifted_into"));
    }
    if (xml::child(e, "unique")) {
        data.is_unique = true;
    }
    if (xml::child(e, "nr_allowed_to_spawn")) {
        data.nr_left_allowed_to_spawn = xml::get_int(xml::child(e, "nr_allowed_to_spawn"));
    }

    for (xml::Element* group_size_e = xml::child(e, "group_size");
         group_size_e;
         group_size_e = xml::next(group_size_e, "group_size")) {
        dump_group_size(group_size_e, data);
    }

    for (xml::Element* native_room_e = xml::child(e, "native_room");
         native_room_e;
         native_room_e = xml::next(native_room_e, "native_room")) {
        dump_native_room(native_room_e, data);
    }
}

static void dump_starting_allies(xml::Element* const e, actor::ActorData& data)
{
    for (auto* ally_e = xml::child(e); ally_e; ally_e = xml::next(ally_e)) {
        const std::string id = xml::get_attribute_str(ally_e, "id");

        actor::StartingAllyEntry starting_ally;

        starting_ally.id = id;

        if (xml::has_attribute_int(ally_e, "min")) {
            starting_ally.nr.min = xml::get_attribute_int(ally_e, "min");
        }
        if (xml::has_attribute_int(ally_e, "max")) {
            starting_ally.nr.max = xml::get_attribute_int(ally_e, "max");
        }

        data.starting_allies.push_back(starting_ally);
    }
}

static void dump_monster_data(xml::Element* const e, actor::ActorData& data)
{
    if (xml::has_child(e, "text")) {
        dump_text(xml::child(e, "text"), data);
    }
    if (xml::has_child(e, "graphics")) {
        dump_gfx(xml::child(e, "graphics"), data);
    }
    if (xml::has_child(e, "audio")) {
        dump_audio(xml::child(e, "audio"), data);
    }
    if (xml::has_child(e, "attributes")) {
        dump_attributes(xml::child(e, "attributes"), data);
    }
    if (xml::has_child(e, "items")) {
        dump_items(xml::child(e, "items"), data);
    }
    if (xml::has_child(e, "attacks")) {
        dump_intr_attacks(xml::child(e, "attacks"), data);
    }
    if (xml::has_child(e, "spells")) {
        dump_spells(xml::child(e, "spells"), data);
    }
    if (xml::has_child(e, "properties")) {
        dump_properties(xml::child(e, "properties"), data);
    }
    if (xml::has_child(e, "ai")) {
        dump_ai(xml::child(e, "ai"), data);
    }
    if (xml::has_child(e, "spawning")) {
        dump_spawning(xml::child(e, "spawning"), data);
    }
    if (xml::has_child(e, "starting_allies")) {
        dump_starting_allies(xml::child(e, "starting_allies"), data);
    }
}

static void read_actor_definitions_xml()
{
    TRACE_FUNC_BEGIN;

    xml::Doc types_doc;
    xml::Doc monsters_doc;
    xml::load_file(paths::data_dir() + "monster_types.xml", types_doc);
    xml::load_file(paths::data_dir() + "monsters.xml", monsters_doc);

    xml::Element* monsters_top_e = xml::child(monsters_doc);
    xml::Element* types_top_e = xml::child(types_doc);

    TRACE << "Reading monster types data" << "\n";

    std::unordered_map<std::string, actor::ActorData> mon_type_data;

    for (auto* mon_e = xml::child(types_top_e); mon_e; mon_e = xml::next(mon_e, "type")) {
        const std::string id = xml::get_attribute_str(mon_e, "id");

        TRACE << "Reading monster type data with ID: " << id << "\n";

        actor::ActorData& data = mon_type_data[id];

        data.reset();

        data.id = id;

        dump_monster_data(mon_e, data);
    }

    TRACE << "Reading monster data" << "\n";

    for (auto* mon_e = xml::child(monsters_top_e); mon_e; mon_e = xml::next(mon_e, "monster")) {
        const std::string id = xml::get_attribute_str(mon_e, "id");

        TRACE << "Reading monster data with ID: " << id << "\n";

        actor::ActorData& data = actor::g_data[id];

        xml::Element* mon_base_e = xml::child(mon_e, "base");

        if (mon_base_e) {
            const std::string base_id = xml::get_str(mon_base_e);

            TRACE << "Base monster type: " << base_id << std::endl;

            // Monster has a base monster type specified, reset to this data.
            data = mon_type_data.at(base_id);
        }
        else {
            // No base monster type specified, just do a full reset.
            data.reset();
        }

        data.id = id;

        dump_monster_data(mon_e, data);
    }

    TRACE_FUNC_END;

}  // read_actor_definitions_xml

// -----------------------------------------------------------------------------
// actor
// -----------------------------------------------------------------------------
namespace actor
{
// -----------------------------------------------------------------------------
// ActorData
// -----------------------------------------------------------------------------
void ActorData::reset()
{
    *this = ActorData {};
}

std::unordered_map<std::string, ActorData> g_data;

void init()
{
    TRACE_FUNC_BEGIN;

    read_actor_definitions_xml();

    TRACE_FUNC_END;
}

void save()
{
    const std::vector<std::string> keys = get_keys_sorted(g_data);

    for (const std::string& key : keys) {
        ActorData& d = g_data[key];

        saving::put_int(d.nr_left_allowed_to_spawn);
        saving::put_int(d.nr_kills);
        saving::put_bool(d.has_player_seen);
    }
}

void load()
{
    const std::vector<std::string> keys = get_keys_sorted(g_data);

    for (const std::string& key : keys) {
        ActorData& d = g_data[key];

        d.nr_left_allowed_to_spawn = saving::get_int();
        d.nr_kills = saving::get_int();
        d.has_player_seen = saving::get_bool();
    }
}

}  // namespace actor
