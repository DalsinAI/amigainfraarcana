// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ATTACK_HPP
#define ATTACK_HPP

#include "global.hpp"
#include "query.hpp"

namespace item
{
class Wpn;
}  // namespace item

namespace actor
{
class Actor;
}  // namespace actor

struct P;

namespace attack
{
inline constexpr int g_pct_chance_eldritch_undoing_double_dmg = 25;

enum class AttackSource
{
    normal,
    // Attack does not spend time, status effects such as terrified cannot prevent melee attacks.
    magical,
};

void melee(
    actor::Actor* attacker,
    const P& origin,
    const P& aim_pos,
    item::Wpn& wpn,
    AttackSource attack_source = AttackSource::normal);

DidAction ranged(
    actor::Actor* attacker,
    const P& origin,
    const P& aim_pos,
    item::Wpn& wpn);

void ranged_hit_chance(
    const actor::Actor& attacker,
    const actor::Actor& defender,
    const item::Wpn& wpn);

BinaryAnswer query_player_attack_mon_with_ranged_wpn(
    const item::Wpn& wpn,
    const actor::Actor& mon);

// Do pure damage against "unnatural" creatures vulnerable to some effects targeting them.
void dmg_unnatural_creature(actor::Actor& actor, actor::Actor* attacker, int dmg);

bool allow_apply_eldritch_undoing_bon(
    const actor::Actor& actor,
    const actor::Actor* attacker);

}  // namespace attack

#endif  // ATTACK_HPP
