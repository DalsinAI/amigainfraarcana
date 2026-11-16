// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "actor.hpp"
#include "actor_factory.hpp"
#include "actor_move.hpp"
#include "attack_internal.hpp"
#include "catch.hpp"
#include "game.hpp"
#include "game_time.hpp"
#include "global.hpp"
#include "item_att_property.hpp"
#include "item_data.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "property.hpp"
#include "property_data.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "terrain.hpp"
#include "terrain_factory.hpp"
#include "test_utils.hpp"

static bool starts_with_any_of(
    const std::string& str,
    const std::vector<std::string>& sub_strings)
{
    const auto& pred = [str](const auto& sub_str) {
        return str.find(sub_str, 0) == 0;
    };

    return (
        std::any_of(
            std::begin(sub_strings),
            std::end(sub_strings),
            pred));
}

static void clear_log_immediately_and_send_messages_to_history()
{
    // Hack to immediately cler the log without fading.
    msg_log::more_prompt();
}

static int nr_game_history_entries_matching(const std::string& search_str)
{
    const std::vector<HistoryEvent>& history = game::history();

    return (
        std::count_if(
            history.begin(),
            history.end(),
            [&search_str](const HistoryEvent& event) {
                return event.msg.find(search_str) != std::string::npos;
            }));
}

// Applies "gripped" on the player the same way as a monster melee attack does.
static void grip_player(const actor::Actor& gripper)
{
    ItemAttackProp att_prop(prop::make(prop::Id::gripped));
    att_prop.prop->set_indefinite();

    attack::try_apply_attack_property_on_actor(
        att_prop,
        *map::g_player,
        DmgType::blunt,
        &gripper);
}

static const prop::Gripped* player_gripped_prop()
{
    return static_cast<const prop::Gripped*>(
        map::g_player->m_properties.prop(prop::Id::gripped));
}

TEST_CASE("Infection triggers disease")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    properties.apply(prop::make(prop::Id::infected));

    REQUIRE(properties.has(prop::Id::infected));
    REQUIRE(!properties.has(prop::Id::diseased));

    // Tick the infected property enough to no longer exist (could use while-true loop, but this
    // could cause a failing test to get stuck).
    for (int i = 0; i < 100000; ++i) {
        properties.on_turn_begin();
    }

    REQUIRE(!properties.has(prop::Id::infected));
    REQUIRE(properties.has(prop::Id::diseased));

    test_utils::cleanup_all();
}

TEST_CASE("Number turns active")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const blind = prop::make(prop::Id::blind);
    blind->set_duration(500);

    properties.apply(blind);

    REQUIRE(properties.prop(prop::Id::blind)->nr_turns_active() == 0);

    properties.on_turn_begin();
    properties.on_turn_begin();
    properties.on_turn_begin();

    REQUIRE(properties.prop(prop::Id::blind)->nr_turns_active() == 3);

    test_utils::cleanup_all();
}

TEST_CASE("Frenzy allows moving away from monster if LOS blocked")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    properties.apply(prop::make(prop::Id::frenzied));

    map::g_player->m_pos.set(10, 10);

    actor::Actor* const mon = actor::make("MON_ZOMBIE", {14, 10});

    map::update_vision();

    // Try moving away from a seen monster - this should NOT be allowed.
    actor::do_move_action(*map::g_player, Dir::left);

    REQUIRE(map::g_player->m_pos == P(10, 10));

    map::update_terrain(terrain::make(terrain::Id::wall, {12, 10}));

    map::update_vision();

    mon->m_mon_aware_state.player_aware_of_me_counter = 1;

    // Try moving away from a known monster to the right, but with the LOS to the monster
    // blocked - this SHOULD be allowed.
    actor::do_move_action(*map::g_player, Dir::left);

    REQUIRE(map::g_player->m_pos == P(9, 10));

    test_utils::cleanup_all();
}

TEST_CASE("Frenzy allows moving away from unseen known monster")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    properties.apply(prop::make(prop::Id::frenzied));

    map::g_player->m_pos.set(10, 10);

    actor::Actor* const mon = actor::make("MON_ZOMBIE", {14, 10});

    map::update_vision();

    // Try moving away from a seen monster - this should NOT be allowed.
    actor::do_move_action(*map::g_player, Dir::left);

    REQUIRE(map::g_player->m_pos == P(10, 10));

    mon->m_properties.apply(prop::make(prop::Id::invis));

    map::update_vision();

    mon->m_mon_aware_state.player_aware_of_me_counter = 1;
    mon->m_mon_aware_state.aware_counter = 1;

    // Try moving away from an unseen known monster - this SHOULD be allowed.
    actor::do_move_action(*map::g_player, Dir::left);

    REQUIRE(map::g_player->m_pos == P(9, 10));

    test_utils::cleanup_all();
}

TEST_CASE("Frenzy allows attacking adjacent unseen known monster")
{
    test_utils::init_all();

    map::g_player->m_inv.put_in_slot(
        SlotId::wpn,
        item::make(item::Id::dagger),
        Verbose::no);

    prop::PropHandler& properties = map::g_player->m_properties;

    properties.apply(prop::make(prop::Id::frenzied));

    map::g_player->m_pos.set(10, 10);

    actor::make("MON_ZOMBIE", {14, 10});

    map::update_vision();

    // Try moving away from a seen monster - this should NOT be allowed.
    actor::do_move_action(*map::g_player, Dir::down);

    REQUIRE(map::g_player->m_pos == P(10, 10));

    actor::Actor* const mon_2 = actor::make("MON_ZOMBIE", {10, 11});

    mon_2->m_properties.apply(prop::make(prop::Id::invis));

    map::update_vision();

    // Reset/clear the message history
    msg_log::init();

    mon_2->m_mon_aware_state.player_aware_of_me_counter = 1;
    mon_2->m_mon_aware_state.aware_counter = 1;

    // Try attacking an unseen known monster directly below the player, with
    // a seen monster to the right - this SHOULD be allowed.
    actor::do_move_action(*map::g_player, Dir::down);

    clear_log_immediately_and_send_messages_to_history();

    const auto& history = msg_log::history();

    REQUIRE(!history.empty());

    const std::vector<std::string> possible_messages = {"I STAB", "I MISS"};

    REQUIRE(starts_with_any_of(history[0].text(), possible_messages));

    REQUIRE(map::g_player->m_pos == P(10, 10));

    test_utils::cleanup_all();
}

TEST_CASE("Poison damage")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const poison = prop::make(prop::Id::poisoned);
    poison->set_duration(10000);
    properties.apply(poison);

    map::g_player->m_hp = 11;
    map::g_player->m_base_max_hp = 11;

    for (int i = 0; i < 10000; ++i) {
        properties.on_turn_begin();
    }

    REQUIRE(map::g_player->m_hp == 5);
    REQUIRE(actor::is_alive(*map::g_player));

    map::g_player->m_hp = 10;
    map::g_player->m_base_max_hp = 10;

    for (int i = 0; i < 10000; ++i) {
        properties.on_turn_begin();
    }

    REQUIRE(map::g_player->m_hp == 5);
    REQUIRE(actor::is_alive(*map::g_player));

    map::g_player->m_hp = 2;
    map::g_player->m_base_max_hp = 2;

    for (int i = 0; i < 10000; ++i) {
        properties.on_turn_begin();
    }

    REQUIRE(map::g_player->m_hp == 1);
    REQUIRE(actor::is_alive(*map::g_player));

    map::g_player->m_hp = 1;
    map::g_player->m_base_max_hp = 1;

    for (int i = 0; i < 10000; ++i) {
        properties.on_turn_begin();
    }

    REQUIRE(map::g_player->m_hp == 1);
    REQUIRE(actor::is_alive(*map::g_player));

    test_utils::cleanup_all();
}

TEST_CASE("Apply same property, old property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* terrified = prop::make(prop::Id::terrified);
    terrified->set_duration(10);

    properties.apply(terrified);

    REQUIRE(properties.has(prop::Id::terrified));
    REQUIRE(properties.prop(prop::Id::terrified)->nr_turns_left() == 10);

    terrified = prop::make(prop::Id::terrified);
    terrified->set_duration(9);

    properties.apply(terrified);

    REQUIRE(properties.has(prop::Id::terrified));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::terrified)->nr_turns_left() == 10);
}

TEST_CASE("Apply same property, new property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* terrified = prop::make(prop::Id::terrified);
    terrified->set_duration(10);

    properties.apply(terrified);

    REQUIRE(properties.has(prop::Id::terrified));
    REQUIRE(properties.prop(prop::Id::terrified)->nr_turns_left() == 10);

    terrified = prop::make(prop::Id::terrified);
    terrified->set_duration(11);

    properties.apply(terrified);

    REQUIRE(properties.has(prop::Id::terrified));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::terrified)->nr_turns_left() == 11);
}

TEST_CASE("Apply same property, old property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* blind = prop::make(prop::Id::blind);
    blind->set_indefinite();

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));
    REQUIRE(
        properties.prop(prop::Id::blind)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 1);

    blind = prop::make(prop::Id::blind);
    blind->set_duration(9);

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::blind)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 1);
}

TEST_CASE("Apply same property, new property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* blind = prop::make(prop::Id::blind);
    blind->set_duration(10);

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));
    REQUIRE(properties.prop(prop::Id::blind)->nr_turns_left() == 10);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 0);

    blind = prop::make(prop::Id::blind);
    blind->set_indefinite();

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::blind)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 1);
}

TEST_CASE("Apply same property, old and new property are indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* blind = prop::make(prop::Id::blind);
    blind->set_indefinite();

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));
    REQUIRE(
        properties.prop(prop::Id::blind)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 1);

    blind = prop::make(prop::Id::blind);
    blind->set_indefinite();

    properties.apply(blind);

    REQUIRE(properties.has(prop::Id::blind));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::blind)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("permanently blind") == 1);
}

TEST_CASE("Apply upgrade of existing property, old property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(10);

    properties.apply(cursed);

    REQUIRE(properties.has(prop::Id::cursed));
    REQUIRE(properties.prop(prop::Id::cursed)->nr_turns_left() == 10);

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(9);

    properties.apply(doomed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 10);
}

TEST_CASE("Apply upgrade of existing property, new property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(10);

    properties.apply(cursed);

    REQUIRE(properties.has(prop::Id::cursed));
    REQUIRE(properties.prop(prop::Id::cursed)->nr_turns_left() == 10);

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(11);

    properties.apply(doomed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 11);
}

TEST_CASE("Apply upgrade of existing property, old property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_indefinite();

    properties.apply(cursed);

    REQUIRE(properties.has(prop::Id::cursed));
    REQUIRE(
        properties.prop(prop::Id::cursed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 1);

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(9);

    properties.apply(doomed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::doomed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 1);
    REQUIRE(nr_game_history_entries_matching("My doom was written") == 1);
}

TEST_CASE("Apply upgrade of existing property, new property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(10);

    properties.apply(cursed);

    REQUIRE(properties.has(prop::Id::cursed));
    REQUIRE(properties.prop(prop::Id::cursed)->nr_turns_left() == 10);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 0);

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_indefinite();

    properties.apply(doomed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::doomed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 0);
    REQUIRE(nr_game_history_entries_matching("My doom was written") == 1);
}

TEST_CASE("Apply downgrade of existing property, old property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(10);

    properties.apply(doomed);

    REQUIRE(properties.has(prop::Id::doomed));
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 10);

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(9);

    properties.apply(cursed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 10);
}

TEST_CASE("Apply downgrade of existing property, new property has longer duration")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(10);

    properties.apply(doomed);

    REQUIRE(properties.has(prop::Id::doomed));
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 10);

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(11);

    properties.apply(cursed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 11);
}

TEST_CASE("Apply downgrade of existing property, old property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_indefinite();

    properties.apply(doomed);

    REQUIRE(properties.has(prop::Id::doomed));
    REQUIRE(
        properties.prop(prop::Id::doomed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("My doom was written") == 1);

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_duration(9);

    properties.apply(cursed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::doomed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 0);
    REQUIRE(nr_game_history_entries_matching("My doom was written") == 1);
}

TEST_CASE("Apply downgrade of existing property, new property is indefinite")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    prop::Prop* const doomed = prop::make(prop::Id::doomed);
    doomed->set_duration(10);

    properties.apply(doomed);

    REQUIRE(properties.has(prop::Id::doomed));
    REQUIRE(properties.prop(prop::Id::doomed)->nr_turns_left() == 10);

    REQUIRE(nr_game_history_entries_matching("My doom was written") == 0);

    prop::Prop* const cursed = prop::make(prop::Id::cursed);
    cursed->set_indefinite();

    properties.apply(cursed);

    // Only doomed shall exist now.
    REQUIRE(!properties.has(prop::Id::cursed));
    REQUIRE(properties.has(prop::Id::doomed));

    // Longest duration shall be used.
    REQUIRE(
        properties.prop(prop::Id::doomed)->duration_mode() ==
        prop::PropDurationMode::indefinite);

    REQUIRE(nr_game_history_entries_matching("A perpetual curse") == 0);
    REQUIRE(nr_game_history_entries_matching("My doom was written") == 1);
}

TEST_CASE("Gripped is not applied on monsters")
{
    test_utils::init_all();

    map::g_player->m_pos.set(10, 10);

    actor::Actor* const gripper = actor::make("MON_WRITHING_MASS", {11, 10});
    actor::Actor* const victim = actor::make("MON_ZOMBIE", {12, 10});

    ItemAttackProp att_prop(prop::make(prop::Id::gripped));
    att_prop.prop->set_indefinite();

    attack::try_apply_attack_property_on_actor(
        att_prop,
        *victim,
        DmgType::blunt,
        gripper);

    REQUIRE(!victim->m_properties.has(prop::Id::gripped));

    test_utils::cleanup_all();
}

TEST_CASE("Player gripped by multiple creatures is released when all grippers stop gripping")
{
    test_utils::init_all();

    map::g_player->m_pos.set(10, 10);

    const actor::Actor* const mon_1 = actor::make("MON_WRITHING_MASS", {11, 10});
    const actor::Actor* const mon_2 = actor::make("MON_WRITHING_MASS", {9, 10});
    const actor::Actor* const mon_3 = actor::make("MON_WRITHING_MASS", {10, 11});

    grip_player(*mon_1);
    grip_player(*mon_2);

    // The same creature gripping again shall not be registered twice.
    grip_player(*mon_2);

    REQUIRE(map::g_player->m_properties.has(prop::Id::gripped));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_1));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_2));
    REQUIRE(!player_gripped_prop()->is_actor_gripping(*mon_3));

    // A creature that is not gripping stops gripping - no effect.
    prop::stop_gripping_player(*mon_3);

    REQUIRE(map::g_player->m_properties.has(prop::Id::gripped));

    prop::stop_gripping_player(*mon_1);

    REQUIRE(map::g_player->m_properties.has(prop::Id::gripped));
    REQUIRE(!player_gripped_prop()->is_actor_gripping(*mon_1));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_2));

    // The last gripper stops gripping (only once, despite gripping twice).
    prop::stop_gripping_player(*mon_2);

    REQUIRE(!map::g_player->m_properties.has(prop::Id::gripped));

    test_utils::cleanup_all();
}

TEST_CASE("Gripping creatures release the player when not adjacent or dead")
{
    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    map::g_player->m_pos.set(10, 10);

    actor::Actor* const mon_1 = actor::make("MON_WRITHING_MASS", {11, 10});
    actor::Actor* const mon_2 = actor::make("MON_WRITHING_MASS", {9, 9});

    grip_player(*mon_1);
    grip_player(*mon_2);

    // Both grippers are adjacent and alive.
    properties.on_turn_begin();

    REQUIRE(properties.has(prop::Id::gripped));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_1));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_2));

    // One gripper is no longer adjacent.
    mon_1->m_pos.set(12, 10);

    properties.on_turn_begin();

    REQUIRE(properties.has(prop::Id::gripped));
    REQUIRE(!player_gripped_prop()->is_actor_gripping(*mon_1));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_2));

    // Moving back does not make the creature grip again.
    mon_1->m_pos.set(11, 10);

    properties.on_turn_begin();

    REQUIRE(!player_gripped_prop()->is_actor_gripping(*mon_1));

    // The last gripper dies.
    mon_2->m_state = ActorState::corpse;

    properties.on_turn_begin();

    REQUIRE(!properties.has(prop::Id::gripped));

    test_utils::cleanup_all();
}

TEST_CASE("Gripping creature releases the player when affected by certain properties")
{
    const auto prop_id =
        GENERATE(
            prop::Id::burning,
            prop::Id::terrified,
            prop::Id::confused,
            prop::Id::weakened,
            prop::Id::paralyzed);

    test_utils::init_all();

    prop::PropHandler& properties = map::g_player->m_properties;

    map::g_player->m_pos.set(10, 10);

    actor::Actor* const mon_1 = actor::make("MON_WRITHING_MASS", {11, 10});
    actor::Actor* const mon_2 = actor::make("MON_WRITHING_MASS", {9, 10});

    grip_player(*mon_1);
    grip_player(*mon_2);

    prop::Prop* const prop = prop::make(prop_id);
    prop->set_indefinite();

    // Force the effect, the gripping creature may resist some properties.
    mon_1->m_properties.apply(prop, prop::PropSrc::intr, true, Verbose::no);

    REQUIRE(mon_1->m_properties.has(prop_id));

    properties.on_turn_begin();

    REQUIRE(properties.has(prop::Id::gripped));
    REQUIRE(!player_gripped_prop()->is_actor_gripping(*mon_1));
    REQUIRE(player_gripped_prop()->is_actor_gripping(*mon_2));

    test_utils::cleanup_all();
}

TEST_CASE("Gripped player cannot move, but eventually tears free")
{
    test_utils::init_all();

    const P start_pos(10, 10);

    map::g_player->m_pos = start_pos;

    const actor::Actor* const mon = actor::make("MON_WRITHING_MASS", {11, 10});

    grip_player(*mon);

    REQUIRE(map::g_player->m_properties.has(prop::Id::gripped));

    // Try to move away from the gripping creature until tearing free (bounded loop, the chance to
    // tear free is 50% per attempt).
    for (int i = 0; i < 1000; ++i) {
        if (!map::g_player->m_properties.has(prop::Id::gripped)) {
            break;
        }

        game_time::g_allow_tick = true;
        actor::do_move_action(*map::g_player, Dir::left);

        // Struggling to tear free never moves the player, even on the attempt that succeeds.
        REQUIRE(map::g_player->m_pos == start_pos);
    }

    REQUIRE(!map::g_player->m_properties.has(prop::Id::gripped));

    // Now the player can move.
    game_time::g_allow_tick = true;
    actor::do_move_action(*map::g_player, Dir::left);

    REQUIRE(map::g_player->m_pos == start_pos.with_x_offset(-1));

    test_utils::cleanup_all();
}
