/*
Copyright 2012 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TIME_H
#define TIME_H

void time_add_1_min( time_t *time );
void time_add_30_min( time_t *time );
void time_add_1_min_approx( time_t *time );
void time_add_5_min_approx( time_t *time );

#endif
