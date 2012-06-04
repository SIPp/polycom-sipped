/*
  unit test screen.cpp

  */


#include "gtest/gtest.h"
#include "screen.hpp"

using namespace std;

// access screen.cpp secret unit test helper routines
string get_screen_exename();


TEST(screentest, methods){
  char teststr[] = "richard was here";
  screen_set_exename(teststr);
  EXPECT_STREQ(teststr, get_screen_exename().c_str());
}




