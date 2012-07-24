/*
  unit test screen.cpp
  20120611
    all tests work but are commented out for convience.
    screen clears prevents viewing of prior test results.
    intentional calls to exit prevent subsequent tests from running

    note reqd additional compiler flag _USE_32BIT_TIME_T to
      avoid bug in 64 bit time code that causes crash when
      REPORT_ERROR, WARNING etc make calls to time stamps.

  */


#include "gtest/gtest.h"
#include "screen.hpp"
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>  //Sleep
#define SLEEP Sleep(2000);
#else
#include <unistd.h>  //sleep
#define SLEEP sleep(2000);
#endif



using namespace std;
char flag1[256];
char flag2[256];
const char* expected1 = "first method called";
const char* expected2 = "second method called";

void dothisonexit(){
  strncpy(flag1,expected1,strlen(expected1)+1);
}

void dothisonexit2(){
  strncpy(flag2,expected2,strlen(expected2)+1);

}

// access methods and members to verify function
string      get_screen_exename();  //screen.cpp secret unit test helper
extern int  screen_inited;         //screen.cpp local
extern char screen_last_error[];   //sipp_globals.cpp

// verify screen_set_exename and exercise screen_init, screen_exit.
TEST(screentest, methods){
  char teststr[] = "richard was here";
  screen_set_exename(teststr);
  EXPECT_STREQ(teststr, get_screen_exename().c_str());
/*********works but disabled to prevent screen clears 
  strncpy(flag1,"INITILIAZED", strlen("INITILIAZED")+1);
  strncpy(flag2,"INITILIAZED", strlen("INITILIAZED")+1);
  screen_init(dothisonexit, dothisonexit2);
  EXPECT_EQ(1,screen_inited);
  screen_exit(EXIT_SCREEN_UNITTEST);
  // verify that both function pointers passed in are called.
  EXPECT_STREQ(expected1,flag1) << "function pointers in screen_init were not properly executed on screen_exit";
  EXPECT_STREQ(expected2,flag2) << "function pointers in screen_init were not properly executed on screen_exit";
*******/
}

// exercise readkey and screenclear
/**********works but disabled so as not to have to hit key and loose screen 
TEST(screentest, clearscreen){
  printf("testing screen_clear...press any key to continue");
  screen_readkey();
  screen_clear();
}
*******/

extern void REPORT_ERROR_nonfatal(const char *fmt, ...);
extern unsigned long screen_errors;
TEST(screentest, errorhandling){
    char* warning = "tHIS IS A warning";
    WARNING(warning);
    char* result = strstr(screen_last_error,warning);
    EXPECT_TRUE(result!=NULL) << "warning message not added to screen_last_error";

//REPORT_ERROR, WARNING et al all failed with:
//assertion failed *ptime <= _MAX_time64_t     (calling seq: _screen_error->_set_last_msg->CStat::FormatTime->localtime->localtime64)
//100% of the time, it would fail with same error
//http://securityvulns.com/advisories/year3000.asp
//compiler flag in both sipp and unit test _USE_32BIT_TIME_T   :resolved issue 
//REPORT_ERROR("%s","Reporting Error");
//  above method calls exit() and prevents remaining tests from running

//calls screen_exit : works but commented out to allow testing to go beyond
//same as above but customized not to trigger screen_exit(), 
//still clears screen though so commented out.
/********* works but clears screen of test results *********
  unsigned long local_screenerror = screen_errors;
  REPORT_ERROR_nonfatal("%s","Reporting Error"); 
  EXPECT_EQ(local_screenerror+1, screen_errors)<< "REPORT_ERROR not updating screen_errors counter";
*****/
  
}




