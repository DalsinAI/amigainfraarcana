// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#include "attack.hpp"

#include <memory>
#include <string>

#include "actor.hpp"
#include "actor_hit.hpp"
#include "actor_see.hpp"
#include "attack_internal.hpp"
#include "common_text.hpp"
#include "game_time.hpp"
#include "item.hpp"
#include "item_att_property.hpp"
#include "item_weapon.hpp"
#include "map.hpp"
#include "msg_log.hpp"
#include "player_bon.hpp"
#include "property.hpp"
#include "property_factory.hpp"
#include "property_handler.hpp"
#include "random.hpp"
#include "terrain_mob.hpp"
#include "text_format.hpp"

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------
const std::vector<prop::Id> s_eldritch_undoing_vulnerable_props = {
    prop::Id::outer_being,
    prop::Id::undead,
};

// -----------------------------------------------------------------------------
// attack
// -----------------------------------------------------------------------------
namespace attack
{
HitSize relative_hit_size(const int dmg)
{
    const int threshold_medium = g_min_dmg_to_wound;
    const int threshold_major = g_min_dmg_to_wound + 3;

    if (dmg >= threshold_major) {
        return HitSize::major;
    }

    if (dmg >= threshold_medium) {
        return HitSize::medium;
    }

    return HitSize::minor;
}

std::string hit_size_punctuation_str(const HitSize hit_size)
{
    switch (hit_size) {
    case HitSize::minor:
        return ".";

    case HitSize::medium:
        return "!";

    case HitSize::major:
        return "!!!";
    }

    return "";
}

void try_apply_attack_property_on_actor(
    const ItemAttackProp& att_prop,
    actor::Actor& actor,
    const DmgType dmg_type,
    const actor::Actor* attacker)
{
    // HACK: Do not allow applying "gripped" on monsters (too much trouble to support).
    if ((att_prop.prop->id() == prop::Id::gripped) && !actor::is_player(&actor)) {
        return;
    }

    if (!rnd::percent(att_prop.pct_chance_to_apply)) {
        return;
    }

    const bool is_resisting_dmg = actor.m_properties.is_resisting_dmg(dmg_type, Verbose::no);

    if (is_resisting_dmg) {
        return;
    }

    prop::Prop* const prop_cpy = prop::make(att_prop.prop->id());

    const prop::PropDurationMode duration_mode = att_prop.prop->duration_mode();

    if (duration_mode == prop::PropDurationMode::specific) {
        prop_cpy->set_duration(att_prop.prop->nr_turns_left());
    }
    else if (duration_mode == prop::PropDurationMode::indefinite) {
        prop_cpy->set_indefinite();
    }

    actor.m_properties.apply(prop_cpy);

    // HACK: Add gripping actor if "gripped" is applied.
    if (prop_cpy->id() == prop::Id::gripped) {
        // NOTE: We retrieve the gripped property again from the actor and add the actor to this
        // property - the actor cannot be added to the applied property. The reason is that the
        // gripped property stores a list of all creatures that is gripping, and there may already
        // be creatures on an existing property currently gripping.
        auto* const gripped =
            static_cast<prop::Gripped*>(
                actor.m_properties.prop(prop::Id::gripped));

        gripped->add_actor_gripping(*attacker);
    }
}

BinaryAnswer query_player_attack_mon_with_ranged_wpn(
    const item::Wpn& wpn,
    const actor::Actor& mon)
{
    const std::string wpn_name = wpn.name(ItemNameType::a);

    const bool can_see_mon = can_player_see_actor(mon);

    const std::string mon_name = can_see_mon ? actor::name_the(mon) : "it";

    const std::string msg =
        "Attack " +
        mon_name +
        " with " +
        wpn_name +
        "? " +
        common_text::g_yes_or_no_hint;

    msg_log::add(
        msg,
        colors::light_white(),
        MsgInterruptPlayer::no,
        MorePromptOnMsg::no,
        CopyToMsgHistory::no);

    const BinaryAnswer answer = query::yes_or_no();

    return answer;
}

void dmg_unnatural_creature(actor::Actor& actor, actor::Actor* const attacker, const int dmg)
{
    Color msg_clr;

    if (actor::is_player(&actor)) {
        msg_clr = colors::msg_bad();
    }
    else if (map::g_player->is_leader_of(&actor)) {
        msg_clr = colors::white();
    }
    else {
        msg_clr = colors::msg_good();
    }

    if (actor::can_player_see_actor(actor)) {
        const std::string name = text_format::first_to_upper(actor::name_the(actor));

        msg_log::add(name + " unravels.", msg_clr);
    }

    actor::hit(actor, dmg, DmgType::pure, attacker);
}

bool allow_apply_eldritch_undoing_bon(
    const actor::Actor& actor,
    const actor::Actor* const attacker)
{
    return (
        actor::is_player(attacker) &&
        player_bon::has_trait(TraitId::eldritch_undoing) &&
        actor::is_alive(actor) &&
        actor::is_alive(*map::g_player) &&
        actor.m_properties.has_any(s_eldritch_undoing_vulnerable_props));
}

}  // namespace attack
