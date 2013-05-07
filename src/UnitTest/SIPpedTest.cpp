// SIPpedTest.cpp : Defines the entry point for the console application.
//


/*
int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}
*/

#include "gtest/gtest.h"


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv); 
  RUN_ALL_TESTS(); 
  std::getchar(); 
}

