/*
Copyright 2012 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#define TIME_1_MINUTE 60
#define TIME_2_MINUTES 120
#define TIME_3_MINUTES 180
#define TIME_4_MINUTES 240
#define TIME_5_MINUTES 300
#define TIME_15_MINUTES 900

time_t time_add_1_min(void);
time_t time_add_15_min(void);
time_t time_add_2_min_approx(void);
time_t time_add_5_min_approx(void);
