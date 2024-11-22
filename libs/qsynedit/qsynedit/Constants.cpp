/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "Constants.h"

namespace QSynedit {
const QSet<QChar> WordBreakChars{'.', ',', ';', ':',
      '"', '\'', '!', '?', '[', ']', '(', ')', '{', '}', '^', '-', '=', '+',
      '-', '*', '/', '\\', '|'};
const QChar TabGlyph(0x2192);
const QChar SpaceGlyph('.');
const QChar LineBreakGlyph(0x2193);
const QChar SoftBreakGlyph(0x2193);
}
