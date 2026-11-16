// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef ITEM_ATT_PROPERTY_HPP
#define ITEM_ATT_PROPERTY_HPP

#include <memory>

#include "property.hpp"

struct ItemAttackProp
{
    ItemAttackProp() = default;

    ItemAttackProp(prop::Prop* const property) :
        prop(property) {}

    ~ItemAttackProp() = default;

    std::shared_ptr<prop::Prop> prop {nullptr};
    int pct_chance_to_apply {100};
};

#endif  // ITEM_ATT_PROPERTY_HPP
