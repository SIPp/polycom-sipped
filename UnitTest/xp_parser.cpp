/*
 * xp_parser_TEST.cpp
 *
 *  Created on: Feb 1, 2012
 *      Author: sipped
 */

#include "gtest/gtest.h"
#include "xp_parser.hpp"

#include <limits.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include <string>
#include <stdexcept>

using namespace std;

// import secret test routines.for xp_parser.cpp
string xp_get_xmlbuffer();
void set_xp_parser_verbose(int value);


int debug = 0;
int dumpxml = debug;  // echo  the xml data after its loaded into xp_file


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

// get file as a string a line at a time. line length limited to bufsize
string get_file_as_string(const string &fn)
{
  FILE* pFile;
  int bufsize = 4096;
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
  return result;
}

TEST(GTEST, helper_routines)
{
  EXPECT_EQ(447, get_file_length("100.xml"));
  EXPECT_THROW(string mystring = get_file_as_string("nonexistantFile"), runtime_error);
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
  int expand_env_var(char*);
  char input[4096];

  // single substitution at front of path
  string pathandfilename="%SIPPED%/src/test/xp_parser.cpp";
  char* env_value = getenv("SIPPED");
  string expected_pathandfilename=string(env_value) + string("/src/test/xp_parser.cpp");
  strcpy(input, pathandfilename.c_str());
  int rc = expand_env_var(input);
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
  rc = expand_env_var(testpath);
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
  string include_file_with_env_var = "include_using_envvar.xml";
  string include_file_with_env_var_expected = "include_using_envvar-expected.xml";
  EXPECT_TRUE(tadir!=NULL) << "TA_DIR environment variable must be set to complete this test";
  ASSERT_EQ(1, xp_set_xml_buffer_from_file(include_file_with_env_var.c_str(), dumpxml)) << "Failed to set buffer - unable to proceed";
  EXPECT_EQ(get_file_as_string(include_file_with_env_var_expected), xp_get_xmlbuffer()) << "xmlbuffer should contain xi include file even though fn contains blanks";

}

