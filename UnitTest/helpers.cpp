/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author : Edward Estabrook - 13 December 2011
 *           From Polyom Inc.
 *
 */

#include "gtest/gtest.h"
#include <string>
#include "helpers.hpp"


TEST(helpers, get_long){
  EXPECT_EQ(get_long("0", "0"), 0);
  EXPECT_EQ(get_long("1", "1"), 1);
  EXPECT_EQ(get_long("001", "001"), 1);
  EXPECT_EQ(get_long("08", "z08"), 8);
  EXPECT_EQ(get_long("1234", "1234"), 1234);
  EXPECT_EQ(get_long("0x1", "0x1"), 1);
  EXPECT_EQ(get_long("0xa", "0xa"), 10);
}

