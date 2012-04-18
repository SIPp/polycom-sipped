/*
 * xp_parser_TEST.cpp
 *
 *  Created on: Feb 1, 2012
 *      Author: sipped
 */

#include "gtest/gtest.h"
#include "xp_parser.hpp"
#include "CompositeDocument.hpp"

#include <limits.h>

#ifndef WIN32
  #include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include <string>
#include <cstring>
#include <stdexcept>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
// windows variants of setenv, unsetenv 
#ifdef WIN32
  #include <stdlib.h>
  #include <string>
  

  char* get_env_var(string envname)
  {
      size_t len;
      char* pvalue;
      errno_t err = _dupenv_s(&pvalue, &len, envname.c_str());
      if (err) printf("failed to get environment variable %s, errno = (%d)\n",envname.c_str(),err);
      return pvalue;
  }
  
  int setenv(string envname,string envvalue,int overwrite)
  { 
    //char* env_value = get_env_var(envname.c_str());
    char* env_value = getenv(envname.c_str());
    if (overwrite||(env_value==NULL))
    {
      std::string envstring = (envname + "=" + envvalue);
      int result = _putenv(envstring.c_str());

      free(env_value);
      return result;
    }

    // overwrite not set and it exists. change nothing 
    free(env_value);
    return -1;
  }

  int unsetenv(string envname)
  {
    std::string envstring = (envname + "=");
    int result = _putenv(envname.c_str());
    return result;
  }

#endif
// import secret test routines.for xp_parser.cpp
string xp_get_xmlbuffer();
void set_xp_parser_verbose(int value);


int debug = 0;
int dumpxml = debug;  // echo  the xml data after its loaded into xp_file
CompositeDocument build_xp_file_metadata(string sippFile, int dumpxml);

//////////////////////////////////////////
//  Unit Test Helper routines
struct stat fileStat;

// get size of file in bytes which equals chars
int get_file_length(const string &fn)
{
  struct stat filestat;
  if (stat(fn.c_str(), &filestat) < 0)
    return -1;
  return filestat.st_size;
}
//windows/linux line ending strategy
//  on windows, when you read a TEXT file in, file with \r\n become \n
//    as seen by cout each char in hex.
//  on windows, when you read a TEXT file in, file with \n becomes \n
//  on linux, when you read a text file in, file with \n outputs \n
//  on linux, when you read a test file in, file with \r\n outputs \r\n
//
//  looks like shortest route to standard is to build a way to strip \r out
//  on any files read in so that all files only use \n
//
//  what if file being read into xml buffer has cr?
//  xp_parser eats \r and does not store into xp_file so stripping cr on all input is consistent
//  xp_parser.cpp:549 , xp_open_and_buffer_file
//      if(c == '\r') continue;
//      xp_file[(*index)++] = c;
//
string strip_cr_from_eol(string input)
{
  //need to remove cr not replace with blanks
  //input.replace(input.begin(), input.end(),'\r',' ');
  size_t pos = input.find('\r');
  while (pos!=string::npos)
  {
    input.erase(pos,1);
    pos = input.find('\r');
  }
  return input;
}

string stringFrNum(int value)
{
  const int MAXDIGS = 256;
  char buf[MAXDIGS];
  sprintf(buf, "%d",value);
  return buf;
}

// get file as a string a line at a time. line length limited to bufsize
string get_file_as_string(const string &fn)
{
  FILE* pFile;
  const int bufsize = 4096;
  char buf[bufsize];
  string result;

  pFile = fopen(fn.c_str(),"r");
  if (pFile==NULL)
  {
    throw runtime_error("Missing file:\'" + fn +"\'" );
  }
  while (!feof(pFile))
  {
    if (fgets(buf, bufsize, pFile) != NULL)
      result += buf;
  }
  fclose (pFile);
  return strip_cr_from_eol(result);
}


///////////////////////////////////////
//  verify loading of string into private variable xp_file
TEST(xp_parser, xp_set_xml_buffer_from_string){
  // ask xp_parser to show details if our setting is  debug
  set_xp_parser_verbose(debug);

  //single line string
  string sample("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n");
  ASSERT_EQ(1, xp_set_xml_buffer_from_string(sample.c_str(), dumpxml)) << "xp_set_xml_buffer_from_string() returned error code";
  EXPECT_EQ(sample, xp_get_xmlbuffer()) << "private variable xp_file does not match input string";

    // multi line string
  string str(
      "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n"
      "<!DOCTYPE scenario SYSTEM \"sipp.dtd\">\n"
      "<scenario name=\"An offerless INVITE message. Ie: No SDP is included. \" parameters=\"-mc -aa -rtp_echo\" xmlns:xi=\"http://www.w3.org/2001/XInclude\">\n"
      "\n"
      "\n"
      "<send>\n"
      "<![CDATA[\n"
      "\n"
      "     INVITE sip:[service]@[remote_ip] SIP/2.0\n"
      "     Via: SIP/2.0/[transport] [local_ip]:[local_port];rport;branch=[branch]\n"
      "     From: \"SIPp\" <sip:[local_ip]:[local_port]>[local_tag_param]\n"
      "     To: <sip:[service]@[remote_ip];user=phone>[remote_tag_param]\n"
      "     CSeq: [cseq] INVITE\n"
      "     Call-Id: [call_id]\n"
      "     Contact: <sip:[local_ip]:[local_port]>\n"
      "     Allow: INVITE, ACK, BYE, CANCEL, OPTIONS, INFO, MESSAGE, SUBSCRIBE, NOTIFY, PRACK, UPDATE, REFER\n"
      "     User-Agent: Sipp\n"
      "     Supported: 100rel,replaces\n"
      "     Allow-Events: talk,hold,conference\n"
      "     Max-Forwards: 70\n"
      "\n"
      "]]>\n"
      "</send>\n"
      "\n"
      "</scenario>\n");

  ASSERT_EQ(1, xp_set_xml_buffer_from_string(str.c_str(), dumpxml)) << "xp_set_xml_buffer_from_string() returned error code";
  EXPECT_EQ(str, xp_get_xmlbuffer()) << "private variable xp_file does not match input string";
}

// string processing does not interpret xi includes at all
TEST(xp_parser, xp_set_xml_buffer_from_string_test_xiinclude){
  string noncommented_xi_include_string ("<?xml-- <xi:include href=\"100.xml\"?> -->");
  ASSERT_EQ(1, xp_set_xml_buffer_from_string(noncommented_xi_include_string.c_str(), dumpxml)) << "xp_set_xml_buffer_from_string() returned error code";
  EXPECT_EQ(noncommented_xi_include_string, xp_get_xmlbuffer()) << "xp_set_xml_buffer_from_string does not process xi_includes: xp_file should be identical to input" ;
}


/////////////////////////////////////////////
// Test xi:include functionality - verifies it works
//
TEST(xp_parser, xp_set_xml_buffer_from_file_no_embedded_comment){
  const string sipp_script_file_name("Include_example.xml");
  const string include_file_name("100.xml");
  const string expected_result(get_file_as_string("Include_example-ExpectedResult.xml"));

  int file1_size = get_file_length(sipp_script_file_name);
  int file2_size = get_file_length(include_file_name);
  ASSERT_NE(-1, file1_size) << "Could not stat " << sipp_script_file_name << ".";
  ASSERT_NE(-1, file2_size) << "Could not stat " << include_file_name << ".";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(sipp_script_file_name.c_str(), dumpxml)) << "xp_set_xml_buffer_from_file() returned error";

  //correct formula is : expected size = size1+size2-xi_include_linesize
  //for this test case : 705           = 298  + 447 - 40
  unsigned totalsize = get_file_length(sipp_script_file_name) + get_file_length(include_file_name) -40 ;
  EXPECT_EQ(totalsize , xp_get_xmlbuffer().length()) << "Length XML string must match expected. Total size = " <<
        file1_size << " + " << file2_size << " - "  << 40 << " =  " <<  totalsize ;
//  if (debug) printf ("totalsize = %d + %d - %d  = %d\n", file1_size, file2_size,  40 ,  totalsize);
  EXPECT_STREQ(expected_result.c_str(), xp_get_xmlbuffer().c_str()) << "Resulting XML string must match. Actual = " <<
        xp_get_xmlbuffer() << "'\n Expected = '" << expected_result << "'";
  //EXPECT_EQ on String objects : equivalent to above
  EXPECT_EQ(expected_result, xp_get_xmlbuffer() ) << "Resulting XML string must match. Actual = " <<
         xp_get_xmlbuffer() << "'\n Expected = '" << expected_result << "'";
}

///////////////////////////////////////
// test xi include inside comment - verify that commented out xi-includes are not included
TEST(xp_parser, xp_set_xml_buffer_from_file_w_include_inside_comment){
  string sipp_file_commented_xiinclude("Commented_xi_include.xml");
  string include_target_file("100.xml");  // should not get included since it is inside comment
  int inputfilesize = get_file_length(sipp_file_commented_xiinclude);

  ASSERT_EQ(1, xp_set_xml_buffer_from_file(sipp_file_commented_xiinclude.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(sipp_file_commented_xiinclude), xp_get_xmlbuffer()) << "xmlbuffer should be identical to input string. xp_file size = " <<
        xp_get_xmlbuffer().length() << " input_file_size = " << inputfilesize << ". target include file size = " << get_file_length(include_target_file) ;

  // file a has include b; file b has commented include c. file c contents should not be in result
  string valid_xi_include_file = "valid_xi_include.xml";
  string expected_valid_xi_include_file = "expected_valid_xi_include.xml";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(valid_xi_include_file.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(expected_valid_xi_include_file), xp_get_xmlbuffer()) << "xmlbuffer should not contain commented xi include file";

  // note double minus not allowed inside comments,
  // http://www.w3.org/TR/2008/REC-xml-20081126/#sec-comments
  // no need to test nested comments.
}

////////////////////////////////////////
// test the xi include filename handling
TEST(xp_parser, xp_set_xml_buffer_from_file_include_fn_path){

  // if included file name as a blank in it, file should still get included
  string xi_include_file_w_blank_in_name = "include_fn_with_blank.xml";
  string expected_result_file_blank_in_name= "include_fn_with_blank-expected_result.xml";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(xi_include_file_w_blank_in_name.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(expected_result_file_blank_in_name), xp_get_xmlbuffer()) << "xmlbuffer should contain xi include file even though fn contains blanks";
  // if included file name has a path with a blank in it, file should still get included
  string xi_include_file_w_blank_in_path = "include_fn_with_blank.xml";
  string expected_result_file_blank_in_path= "include_fn_with_blank-expected_result.xml";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(xi_include_file_w_blank_in_path.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(expected_result_file_blank_in_path), xp_get_xmlbuffer()) << "xmlbuffer should contain xi include file even though fn contains blanks";
}

////////////////////////////////////////////
// verify handling of environment variable as part of filename for xi includes
// sets and unsets environment variables as part of testing.
TEST(xp_parser, expand_env_var)
{
  int expand_env_var(char*,int);
  char input[4096];

  // single substitution at front of path
  char* env_value = getenv("SIPPED");
  ASSERT_STRNE(env_value,NULL) << "SIPPED environment variable is not set";
  string pathandfilename="%SIPPED%/src/test/xp_parser.cpp";
  
  string expected_pathandfilename=string(env_value) + string("/src/test/xp_parser.cpp");
  strcpy(input, pathandfilename.c_str());
  int rc = expand_env_var(input,0);
  EXPECT_GT(rc,0) << "number of substitutions should be greater than zero";
  EXPECT_STREQ(expected_pathandfilename.c_str(), input) << "path and filename should have env var expanded";

  // two environment variables in filename
  char* env_val01 = getenv("ENV_VAR01");
  char* env_val02 = getenv("ENV_VAR02");
  char* tmp01 = NULL;
  char* tmp02 = NULL;
  if (env_val01)
    strcpy(tmp01,env_val01);
  if (env_val02)
    strcpy(tmp02,env_val02);
  setenv("ENV_VAR01","richard",1);
  setenv("ENV_VAR02","here",1);
  char testpath[4096];
  strcpy(testpath,"%ENV_VAR01%/was/%ENV_VAR02%");
  rc = expand_env_var(testpath,0);
  EXPECT_EQ(16,rc) << "length of filename after substitution not correct";
  EXPECT_STREQ("richard/was/here",testpath) << "two env var substitutions should match";
  //restore env var if we changed them, otherwise unset them
  if (tmp01)
  {
    setenv("ENV_VAR01",tmp01,1);
  }else
  {
    unsetenv ("ENV_VAR01");
  }
  if (tmp02)
  {
    setenv("ENV_VAR02",tmp02,1);
  }else
  {
    unsetenv("ENV_VAR02");
  }

  // test xi include that TA_DIR environment variable
  char* tadir = getenv("TA_DIR");
  ASSERT_STRNE(tadir,NULL) << "TA_DIR environment variable is not set";
  string include_file_with_env_var = "include_using_envvar.xml";
  string include_file_with_env_var_expected = "include_using_envvar-expected.xml";
  EXPECT_TRUE(tadir!=NULL) << "TA_DIR environment variable must be set to complete this test";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(include_file_with_env_var.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(include_file_with_env_var_expected), xp_get_xmlbuffer()) << "xmlbuffer should contain xi include file even though fn contains blanks";

}

/////////////////////////////////////////
// test the metafile information for xp_file that provides infrastructure for report
// equivalence of location in source files being processed taking into account
// multiple nested include file capabilities

TEST(xp_parser, CompositeDocument)
{

  // newline offsets for composite document formed by
  // %TA_DIR%/SIPped/SIPped/src/test/include_substitution.sipp
  // and it's include documents  -- verified newlines by hand as correct reference
  // for byte offset of all new lines in composite document
  const int newlines[] = {
  44 ,     82 ,     83 ,    182 ,    183 ,    268 ,    315 ,    353 ,    454 ,
  455 ,    528 ,    576 ,    614 ,    615 ,    716 ,    717 ,    796 ,    799 ,
  820 ,    833 ,    894 ,    922 ,    930 ,    940 ,    941 ,    962 ,    975 ,
  1033 ,   1064 ,   1092 ,   1100 ,   1110 ,   1113 ,   1126 ,   1129 ,   1150 ,
  1163 ,   1224 ,   1252 ,   1260 ,   1270 ,   1271 ,   1292 ,   1305 ,   1363 ,
  1394 ,   1422 ,   1430 ,   1440 ,   1443 ,   1464 ,   1477 ,   1541 ,   1569 ,
  1577 ,   1587 ,   1590 ,   1602 ,   1605 ,   1691 ,   1738 ,   1776 ,   1877 ,
  1878 ,   1951 ,   1999 ,   2037 ,   2038 ,   2139 ,   2140 ,   2219 ,   2222 ,
  2243 ,   2256 ,   2317 ,   2345 ,   2353 ,   2363 ,   2364 ,   2385 ,   2398 ,
  2456 ,   2487 ,   2515 ,   2523 ,   2533 ,   2536 ,   2549 ,   2552 ,   2573 ,
  2586 ,   2647 ,   2675 ,   2683 ,   2693 ,   2694 ,   2715 ,   2728 ,   2786 ,
  2817 ,   2845 ,   2853 ,   2863 ,   2866 ,   2887 ,   2900 ,   2964 ,   2992 ,
  3000 ,   3010 ,   3013 ,   3025 ,   3026 ,   3038 ,   3039 ,   3040
  };
  //initilize nl vector from newlines array from element 0 to element 115)
  const vector<int> nl(newlines, newlines+116);
  // for simplicity, transform from vector of byte locations where a newline is to
  // a vector indexed by byte to line number
  vector<int> byteToLine;
  int linenumber =1;
  for (int byte=0; byte<nl[nl.size()-1];byte++)
  {
    byteToLine.push_back(linenumber);
    if (byte>=nl[linenumber-1])  //vector is zero based while lines are 1 based
    {
      linenumber++;
    }
  }
  //byteToLine now contains a vector of line numbers with byte offset as the index
  // Here, the newline is treated as the first char of the next line

  string expected_xp_file = get_file_as_string("expected_xp_file_from_include_substitution_sipp.txt");

  // expected_xp_file now contains expected composite document
  // (resultant document after all includes executed)
  // for include_substitution.sipp
  string rootSippWithIncludes="../test/include_substitution.sipp";
  int dumpxml =0;

  /**
   * Note that CompositeDocument NEEDS TO BE RESET between tests
   * of CompositeDocument.
   * This is done by build_xp_file_metadata .
   */
  CompositeDocument metadata = build_xp_file_metadata(rootSippWithIncludes, dumpxml);

  // does newline map in xp_buffer match CompositeDocument?
  EXPECT_TRUE(metadata.checkNewLineSynch(xp_get_xmlbuffer().c_str())) << "CompositeDocument Mapped Newlines not found in buffer xp_file";
  // does composite document newlines match reference in nl vector
  for (unsigned int i=1; i< nl.size();i++){
    EXPECT_EQ(nl[i],metadata.getLineOffsetMap()[i]) << "Newline offsets need to match expected reference";
  }
  // Does xp_parser buffer contents match expected composite document
  EXPECT_EQ(expected_xp_file,xp_get_xmlbuffer() ) << "Buffer should match expected composite document";
  // test byte offset to composite line number.
  for (int i=0; i<3040;i++)
  {
    EXPECT_EQ(byteToLine[i], metadata.compositeLineNumberFromIndex(i)) << "Line Number should match expected for byte " << i;
  }

//generate reference set of docStacks for validation
//  fstream exp_docstack;
//  exp_docstack.open("expected_docStacks.txt", ios_base::out);
//  exp_docstack << metadata.dumpStacks();
//  exp_docstack.close();

  //test mapping of composite line number to document stack
  //  verified against source include documents
  string exp_stackdump = get_file_as_string("expected_docStacks.txt");
  EXPECT_EQ(exp_stackdump, metadata.dumpStacks()) << "Dump of all saved Document Stacks should match expected_docStacks.txt";

  // tested byte to composite line number above
  // now need to test composite line number to stack
  string allStacksbyLine ="";
  for (int i=1;i<116;i++)
  {
    allStacksbyLine += stringFrNum(i) + "---------\n";
    allStacksbyLine += metadata.strStackFromCompositeLineNumber(i);
  }
  EXPECT_EQ(get_file_as_string("expected_allStacks.txt"),allStacksbyLine) << "Stack dump by line for all lines does not match expected";

}


