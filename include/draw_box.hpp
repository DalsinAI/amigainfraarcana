// =============================================================================
// Copyright Martin Törnqvist
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

#ifndef DRAW_BOX_HPP
#define DRAW_BOX_HPP

#include "colors.hpp"

struct R;

void draw_box(R border, const Color& color = colors::dark_gray());

#endif  // DRAW_BOX_HPP
