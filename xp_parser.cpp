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
 *  Copyright (C) 2003 - The Authors
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

/*
 * Mini xml parser:
 *
 * WARNING 1: Only supports printable
 * ASCII characters in xml files. '\0'
 * is not a valid character. Returned string are
 * NULL-terminated.
 *
 * WARNING 2: Does not supports multithreading. Works
 * with static buffer, no memory allocation.
 */

/*******************  Include files *********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CompositeDocument.hpp"
#include "xp_parser.hpp"

#ifdef WIN32
#define snprintf _snprintf
#include "win32_compatibility.hpp"
#endif
//
#include <ctype.h>
#include <string>
#include <stddef.h>


using namespace std;

/************* Constants and Global variables ***********/

#define XP_MAX_NAME_LEN   256
#define XP_MAX_FILE_LEN   655360
#define XP_MAX_STACK_LEN  256
#define MAXERRSIZE 4096

char   xp_file     [XP_MAX_FILE_LEN + 1];
char * xp_position [XP_MAX_STACK_LEN];
int    xp_stack    = 0;

char xml_special_char_encoding[][6] = {"lt;", "gt;", "amp;", "apos;", "quot;", "\0"};
char xml_special_char[]           = {'<', '>', '&', '\'', '"'};

int     verbose = 0;
string xp_error_messages = "";
char err[MAXERRSIZE];

// collects newline, include file locations withing the xp_file buffer as it
// is built from source sipp and xml files.  Initialized in xp_set_xml_buffer_from_file
CompositeDocument xp_file_metadata;
int xp_file_metadata_is_not_valid =0;

/****************** Internal routines ********************/
// set locally with xp_store_error_message, retrieved globally by xp_get_errors()



void store_error_message(string message)
{
  xp_error_messages = xp_error_messages + message;
}

string xp_get_errors()
{
  return xp_error_messages;
}

int xp_replace(char *source, char *dest, const char *search, const char *replace)
{
  char *position;
  char *occurances;
  int number = 0;

  if (!source || !dest || !search || !replace) {
    return -1;
  }
  dest[0] = '\0';
  position = source;
  occurances = strstr(position, search);
  while (occurances) {
    strncat(dest, position, occurances - position);
    strcat(dest, replace);
    position = occurances + strlen(search);
    occurances = strstr(position, search);
    number++;
  }
  strcat(dest, position);
  return number;
}

/* This finds the end of something like <send foo="bar">, and does not recurse
 * into other elements. */
char * xp_find_start_tag_end(char *ptr)
{
  while(*ptr) {
    if (*ptr == '<') {
      if (!strncmp(ptr, "<!--", strlen("<!--"))) {
        char * comment_end = strstr(ptr, "-->");
        if(!comment_end) return NULL;
        ptr = comment_end + 3;
      } else {
        return NULL;
      }
    } else  if((*ptr == '/') && (*(ptr+1) == '>')) {
      return ptr;
    } else if (*ptr == '"') {
      ptr++;
      while(*ptr) {
        if (*ptr == '\\') {
          ptr += 2;
        } else if (*ptr=='"') {
          ptr++;
          break;
        } else {
          ptr++;
        }
      }
    } else if (*ptr == '>') {
      return ptr;
    } else {
      ptr++;
    }
  }
  return ptr;
}

char * xp_find_local_end()
{
  char * ptr = xp_position[xp_stack];
  int level = 0;

  while(*ptr) {
    if (*ptr == '<') {
      if (!strncmp(ptr,"<![CDATA[", strlen("<![CDATA["))) {
        char * cdata_end = strstr(ptr, "]]>");
        if(!cdata_end) return NULL;
        ptr = cdata_end + 3;
      } else if (!strncmp(ptr, "<!--", strlen("<!--"))) {
        char * comment_end = strstr(ptr, "-->");
        if(!comment_end) return NULL;
        ptr = comment_end + 3;
      } else if(*(ptr+1) == '/') {
        level--;
        if(level < 0) return ptr;
      } else {
        level ++;
      }
    } else  if((*ptr == '/') && (*(ptr+1) == '>')) {
      level --;
      if(level < 0) return ptr;
    } else if (*ptr == '"') {
      ptr++;
      while(*ptr) {
        if (*ptr == '\\') {
          ptr ++; /* Skip the slash. */
        } else if (*ptr=='"') {
          break;
        }
        ptr++;
      }
    }
    ptr++;
  }
  return ptr;
}

/*
  * input name: environment variable name
  * input idx: index into xp_file, used for error location reporting
 * output value of the environment variable
 *
 */
string translate_envvar(string name, int idx)
{
  char* value = getenv(name.c_str());
  if (!value) {
    store_error_message( "Undefined Environment Variable : " + name + "\r\n" +
                         "Found at: \n" + convert_whereami_key_to_string(idx) +"\r\n");
    return "";
  }

  return string(value);
}
/**
 * input:path_and_fn
 *        array containing filename with optional path and optional environment variable
 *        environment variable delimited by percentage char at start and end %NAME%
 * input:idx
 *        pointer to current location within xp_file (the buffer being filled with include files)
          this is required for location reporting should we run into errors.
 * output: environment variables are expanded and replaced with users settings
 * return the size of the new array string
 *        -1 if not able to substitute if array is not large enough to hold result
 * precondition:  the array is large enough to hold expanded path/filename

 */

int expand_env_var(char* path_and_fn,int idx)
{

  char varMarker = '%';
  size_t startp, endp;
  //int substitution_count = 0;
  string pathandfn = string(path_and_fn);
  startp = pathandfn.find(varMarker, 0 );
  while (startp != pathandfn.npos) {
    //substitution_count++;
    endp = pathandfn.find(varMarker, startp+1);
    if (endp == pathandfn.npos ) {
      store_error_message("Malformed environment environment variable - missing closing % \r\n" +
                          string("Found at:\n ") + convert_whereami_key_to_string(idx) + string("\r\n"));
      return -1;
    }
    string envvar = pathandfn.substr(startp+1,endp-startp-1); // stripped of leading and trailing varMarker
    string envvalue = translate_envvar(envvar, idx);
    if (envvalue.size()==0) {
      store_error_message("Environment Value not retrieved\r\n");
      return -1; // unable to retrieve value for environment variable
    }
    pathandfn = pathandfn.substr(0,startp) + envvalue + pathandfn.substr(endp+1);
    // look to see if there are anymore env variables
    startp = pathandfn.find(varMarker, endp+1 );
  }

  if (pathandfn.size()+1 <= XP_MAX_NAME_LEN) {
    pathandfn.copy(path_and_fn, pathandfn.size(), 0 );
    path_and_fn[pathandfn.size()]=0;
    return pathandfn.size();
  } else {
    store_error_message("Expanded path/filename larger than %d\r\n" +  XP_MAX_NAME_LEN +
                        string("Found at:\n ") + convert_whereami_key_to_string(idx) +string("\r\n"));
    return -1;  // array is not large enough to hold result
  }
}

/********************* Interface routines ********************/

//////////////////////////////////
// Goal is to get the byte offset into xp_file that we are currently processing
// output: byte offset into xp_file that is currently used by xp_parser. Valid
//    after xp_file has been filled and is being parsed. (eg scenario pushes
//    ptrs to parts of xp_file onto xp_position as a stack of
//    pointers for parsing/processing
//
//    DO NOT USE THIS DURING INTIAL BUFFERING OF XP_FILE
//    if you want location while xp_file is being initially buffered, USE
//    index defined in xp_set_xml_buffer_from_file instead.
// note that some methods method takes a copy of xp_position, eg
//    char* ptr = xp_position[xp_stack]
//    Local manipulation of  ptr not visible and in those methods,
//    you might want to use local ptr instead
//    eg use  (ptr-xp_file) instead of this method to get
//    index for input to convert_whereami_key_to_string
unsigned int xp_get_whereami_key()
{
  //byte offset into xp_file
  unsigned int index = xp_position[xp_stack] - xp_file;
  return index;
}
// input takes byte offset into xp_file
string convert_whereami_key_to_string(unsigned int key)
{
  if (xp_file_metadata.getQtyStacks() !=0) {
    return xp_file_metadata.strStackFromIndex(key);
  } else { // using a built-in scenario
    return "(built-in scenario)";
  }
}
// this is the most generic way to get location info while parsing
// contents of xp_file.  Valid if xp_file has not yet been overwritten
// by xp_set_xml_buffer_from_string, which it always seems to be on
// sipp second call to scenario to build responses.
string whereami_if_valid()
{
  if (xp_file_metadata_is_not_valid)
    return "";
  return convert_whereami_key_to_string(xp_get_whereami_key());
}

int is_xp_file_metadata_valid()
{
  return !xp_file_metadata_is_not_valid;
}


int xp_set_xml_buffer_from_string(const char * str, int dumpxml)
{
  size_t len = strlen(str);


  if(len > XP_MAX_FILE_LEN) {
    return 0;
  }
  strcpy(xp_file, str);
  // flag since this means that CompositeDocument metadata is no longer
  // applicable to contents of xp_file.
  xp_file_metadata_is_not_valid=1;

  xp_stack = 0;
  xp_position[xp_stack] = xp_file;

  if (dumpxml)
    printf("%s", xp_file);

  if(strncmp(xp_position[xp_stack], "<?xml", strlen("<?xml"))) return 0;
  if(!strstr(xp_position[xp_stack], "?>")) return 0;
  xp_position[xp_stack] = xp_position[xp_stack] + 2;

  return 1;
}

// return index where path ends (and file spec begins) or 0 if it is missing
// so filename[end] will be at first letter of filename
int xp_get_start_index_of_filename(const char * filename)
{
  int end = strlen(filename);
  for (; ((end>0) && (filename[end-1] != '/') && (filename[end-1] != '\\')); end--);
  return end;
}

#define DEBUG(x, ...) { if (verbose) { printf(x, ##__VA_ARGS__); } }

// return 1 on success, 0 on error
int xp_open_and_buffer_file(const char * filename, char * path, int *index, unsigned *sub_list, unsigned sub_length)
{
  int include_index = 0;
  const char* include_tag = "<xi:include href=\"";
  int include_tag_length = strlen(include_tag);
  int dialog_index = 0;
  const char* dialog_param = "dialog=\"";
  int dialog_param_length = strlen(dialog_param);
  int replace_index = 0;
  const char* replace_param = "dialogs=\"";
  int replace_param_length = strlen(replace_param);
  const int XP_MAX_LOCAL_SUB_LIST_LEN = 26;
  unsigned local_sub_list[XP_MAX_LOCAL_SUB_LIST_LEN];
  int local_sub_length = 0;

  int comment_index = 0;  // used for both open and close
  const char* start_comment_tag = "<!--";
  const char* end_comment_tag = "-->";
  int inside_comment = 0; // flag to set if we are currently inside a comment
  int start_comment_length = strlen(start_comment_tag);
  int end_comment_length = strlen(end_comment_tag);

  char include_file_name[XP_MAX_NAME_LEN];
  char new_path[XP_MAX_NAME_LEN];
  char path_and_filename[XP_MAX_NAME_LEN];

  // combine path and filename for file open if open without fails;
  // add file path to existing path (or replace if absolute) for recursive opens
  if ((filename[0] != '\\') && (filename[0] != '/') )
    snprintf(path_and_filename, XP_MAX_NAME_LEN, "%s%s", path, filename);
  else
    strcpy(path_and_filename, filename);
  int new_path_length = xp_get_start_index_of_filename(path_and_filename);
  strncpy(new_path, path_and_filename, new_path_length);
  new_path[new_path_length] = 0;
  DEBUG("filename = '%s', path = '%s', path_and_filename = '%s', new_path = '%s'\n", filename, path, path_and_filename, new_path);

  int c;
  FILE * f = fopen(filename, "rb");
  if (!f) {
    if ((filename[0] != '\\') && (filename[0] != '/') && (strlen(path)) ) {
      f = fopen(path_and_filename, "rb");
      if(!f) {
        snprintf(err,MAXERRSIZE, "Unable to open file name '%s' directly or in '%s'\r\n", filename, new_path);
        store_error_message(err);
        return 0;
      }
    } else {
      snprintf(err, MAXERRSIZE, "Unable to open file name '%s'\r\n", filename);
      store_error_message(err);
      return 0;
    }
  } // if !f

  xp_file_metadata.includeFile(filename);   // for track nesting of include files and line numbers
  while((c = fgetc(f)) != EOF) {
    // look for comment tags and set inside_comment flag when required
    if (!inside_comment) { // not in comment, look for start tag
      if (c == start_comment_tag[comment_index]) {
        comment_index++;
      } else {
        comment_index=0;
      }
      // matched all comment start chars, flag inside_comment
      if (comment_index >= start_comment_length ) {
        inside_comment = 1;
        comment_index = 0;
      }
    } else { // we are inside a comment, scan for end_comment
      if (c == end_comment_tag[comment_index]) {
        comment_index++;
      } else {
        comment_index=0;
      }
      // matched all comment end chars,no longer  inside comment
      if (comment_index >= end_comment_length) {
        inside_comment = 0;
        comment_index = 0;
      }
    }

    // Handle match for include file
    if ((c == include_tag[include_index]) && (!inside_comment)) {
      include_index++;
      if (include_index >= include_tag_length) {
        // match: move index to spot to overwrite, read out the filename, recursively call to place inline
        *index = *index - (include_tag_length-1); // index not bumped for final quote yet

        // read up until '">' (if EOF or EOLN before its an error)
        include_index = 0; // use to track file name
        while((c = fgetc(f)) != '"') {
          if (c == EOF || c == '\r' || c == '\n') {
            snprintf(err, MAXERRSIZE, "xi:include tag must be formatted exactly '<xi:include href=\"filename\"/>'  No EOF or EOLN permitted.\r\n");
            store_error_message(err);
            store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
            fclose(f);
            return 0;
          }
          include_file_name[include_index] = c;
          include_index++;
        } // while fgetc != '"'
        // null terminate the filename so we can work with it as a string
        include_file_name[include_index]=0;
        //  we should have full path/filename inside "include_file_name"
        //  lets scan it for a environment variable marker and substitute if required
        //  Also adjust include_index to reflect new size of string
        include_index=expand_env_var(include_file_name, *index);
        if (include_index<0) {
          store_error_message("unable to get Environment variable value\r\n");
          return 0; // failed to expand env_var: not set , or value too long
        }

        c = getc(f);
        while ((c == ' ') || (c == '\t')) c = fgetc(f); // skip whitespace

        if (c != '/') { // if not closing tab, specifying dialogs="1,2,3"
          DEBUG("dialogs parameter expected.\n");

          // match 'dialogs="'
          replace_index = 0;
          while ( (replace_index < replace_param_length) && (c == replace_param[replace_index]) ) {
            c = fgetc(f);
            replace_index++;
          }
          if (replace_index != replace_param_length) {
            snprintf(err, MAXERRSIZE, "Error: only valid option to xi:include is 'dialogs=\"1,2,3\"'.\r\n");
            store_error_message(err);
            store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
            fclose(f);
            return 0;
          }
          // extract n1,n2,n3 until " is encountered
          DEBUG("dialogs tag matched, extracting list.\n");
          char number_str[8];
          int number_len = 0;
          do {
            if ((c >= '0' && c <= '9') || (toupper(c) >= 'A' && toupper(c) <= 'Z')) {
              number_str[number_len] = c;
              number_len++;
              if (number_len > 6) {
                number_str[number_len] = 0;
                snprintf(err, MAXERRSIZE,"Error in '%s': dialogs substitution string %s is too long. dialogs tag value must be comma separated numbers.\r\n", filename, number_str);
                store_error_message(err);
                store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
                fclose(f);
                return 0;
              }
            } else if (c == ',' || c == '"') {
              if (number_len > 0) {
                // add numbers / letters
                int d;
                if (local_sub_length >= XP_MAX_LOCAL_SUB_LIST_LEN) {
                  snprintf(err, MAXERRSIZE, "Error in '%s': Too many substitution parameters. Maximum is %d.\r\n", filename, XP_MAX_LOCAL_SUB_LIST_LEN);
                  store_error_message(err);
                  store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
                  fclose(f);
                  return 0;
                }
                number_str[number_len] = 0;
                if (number_str[0] >= '0' && number_str[0] <= '9') {
                  d = atoi(number_str);
                  if (d > 99) {
                    snprintf(err, MAXERRSIZE, "Error in '%s': Maximum dialog ID for substitution is 99.\r\n", filename);
                    store_error_message(err);
                    store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
                    fclose(f);
                    return 0;
                  }
                } else if ((toupper(number_str[0]) >= 'A') && (toupper(number_str[0]) <= 'Z') &&
                           (toupper(number_str[1]) == toupper(number_str[0])) && (number_len == 2)) {
                  d = number_str[0] - (int)'A';
                  if (d >= (int)sub_length) {
                    snprintf(err, MAXERRSIZE, "'Error in '%s': %s' => %d is greater than largest substitution available of %d.\r\n", filename, number_str, d+1, sub_length);
                    store_error_message(err);
                    store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
                    fclose(f);
                    return 0;
                  }
                  DEBUG("number_str = %s => %d. sub_list[%d] = %d\n", number_str, d, d, sub_list[d]);
                  d = sub_list[d];
                } else {
                  snprintf(err, MAXERRSIZE, "Invalid dialogs parameter value '%s'.\r\n", number_str);
                  store_error_message(err);
                  store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
                  fclose(f);
                  return 0;
                }

                DEBUG("Adding %d to local_sub_list[%d].\n", d, local_sub_length);
                local_sub_list[local_sub_length] = d;
                local_sub_length++;
                number_len = 0;
              }
              if (c == '"')
                break;
            } // if c== ',' || '"'
            else {
              snprintf(err, MAXERRSIZE, "Error, xi:include tag's dialogs list must contain numbers separated with commas and no whitespace.\r\n");
              store_error_message(err);
              store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
              fclose(f);
              return 0;
            }
            c = fgetc(f);
          } while (1);

          c = fgetc(f);
          while ((c == ' ') || (c == '\t')) c = fgetc(f); // skip whitespace

        } // if dialogs option specified


        if (c != '/' || fgetc(f) != '>') {
          snprintf(err, MAXERRSIZE, "xi:include tag must be formatted exactly '<xi:include href=\"filename\" dialogs=\"1,2,3\"] />'  '/>' must be together.\r\n");
          store_error_message(err);
          store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
          fclose(f);
          return 0;
        }
        include_file_name[include_index] = 0;

        if (!xp_open_and_buffer_file(include_file_name, new_path, index, local_sub_list, local_sub_length))
          return 0;
        include_index = 0;
        local_sub_length = 0;
        c = '\r'; // don't add this letter if returning from recursive parse.
      } // if include_index >= include_tag_length
    } // if c == include_tag[include_index])
    else
      include_index = 0; // not include tag: reset search

    // Handle parsing file into xp_file
    if(c == '\r') continue;

    // as we write to xp_file, check for newline and store location info
    if (c=='\n')  xp_file_metadata.incr_line(*index);
    xp_file[(*index)++] = c;
    // check for dialog="nn" substitution
    if (sub_list) {
      // check 'dialog="' part
      if (dialog_index < dialog_param_length) {
        if (dialog_param[dialog_index] == c)
          dialog_index++;
        else
          dialog_index = 0;
      } // dialog_index < dialog_param_length
      // check for 1st letter
      else if (dialog_index == dialog_param_length) {
        if ((toupper(c) >= 'A') && (toupper(c) <= 'Z'))
          dialog_index++;
        else
          dialog_index = 0;
      }
      // check 1st & second letters match each other (ie AA not AB)
      else if (dialog_index == dialog_param_length+1) {
        if (xp_file[(*index)-2] == c)
          dialog_index++;
        else
          dialog_index = 0;
      }
      // final quote: substitute number
      // Note this implementation replaces 2 letters with 2 digits, but algorithm could
      // easily be extended to accomodate variable-length substitution as replacement
      // is performed inline.
      else if (dialog_index == dialog_param_length+2) {
        if (c == '"') {
          // convert to integer s = [AA => 0, BB => 1, etc]
          // Note: index is new spot; i-1 is ", i-2 & i-3 are the letters to replace
          unsigned int sub_idx = toupper(xp_file[(*index)-2] ) - (int)'A';
          if (sub_idx >= sub_length) {
            snprintf(err, MAXERRSIZE, "Error in '%s': not enough include parameters while attempting to substitute '%s'. This requires at least %d dialog id(s) specified with the include, but there were only %d.\r\n", filename, (char *) &xp_file[(*index)-(dialog_param_length+4)], sub_idx+1, sub_length);
            store_error_message(err);
            store_error_message("Found at: \n" + convert_whereami_key_to_string(*index) +"\r\n");
            fclose(f);
            return 0;
          }

          char str[3];
          snprintf(str, 3, "%.2d", sub_list[sub_idx]);
          xp_file[(*index)-3] = str[0];
          xp_file[(*index)-2] = str[1];
          // (if not in index, error)
        }
        dialog_index = 0;
      }

      if (dialog_index == 0 && dialog_param[0] == c)
        dialog_index = 1;
    } // if sub_list != 0

    if(*index >= XP_MAX_FILE_LEN) {
      snprintf(err, MAXERRSIZE, "Error: XML scenario file too long (current maximum for all content including includes is %d bytes).\r\n", XP_MAX_FILE_LEN);
      store_error_message(err);
      fclose(f);
      return 0;
    }
  }
  fclose(f);

  // for tracking of include documents and line numbers
  xp_file_metadata.endIncludeFile();
  if (xp_error_messages != "") {
    return 0;
  }

  return 1;
} // xp_open_and_buffer_file




// return 1 on success, 0 on error
int xp_set_xml_buffer_from_file(const char * filename, int dumpxml)
{
  int index = 0;
  char empty[] = "";
  //reset metadata in case this is called multiple times
  //  valid until either xp_file loaded from string or
  //  this method called again.
  xp_file_metadata.reset();
  xp_file_metadata_is_not_valid =0;

  int result = xp_open_and_buffer_file(filename, empty, &index, 0, 0);

  xp_file[index++] = 0;
  xp_stack = 0;
  xp_position[xp_stack] = xp_file;

  if (dumpxml)
    printf("%s", xp_file);

  if (!result) {
    store_error_message("Failed to retrieve file " + string(filename) + "\r\n");
    return 0;
  }

  if(strncmp(xp_position[xp_stack], "<?xml", strlen("<?xml"))) return 0;
  if(!strstr(xp_position[xp_stack], "?>")) return 0;
  xp_position[xp_stack] = xp_position[xp_stack] + 2;

  if (verbose) {
    printf("xp_file metadata: stack images collected\n");
    printf("%s\n",xp_file_metadata.dumpStacks().c_str());
    printf("xp file metadata: map of newlines collected\n");
    xp_file_metadata.showLineOffsetMap();
    printf( "xp file metadata for newline sychroniziation is ");
    if (xp_file_metadata.checkNewLineSynch(xp_file)) {
      printf(" good\n");
    } else {
      printf(" bad.  Not all stored newlines were found in source xp_file\n");
      printf(" Call Ed and have him send out a search party for missing newlines\n");
    }
  }
  return 1;
}

char * xp_open_element(int index)
{
  return xp_open_element_skip_control(index, 1);
}

char * debug_buffer(char * ptr)
{
  static char          buf[50];
  int i=0;

  if (ptr) {
    for (; (i<49&&ptr[i]!=0); i++) {
      buf[i] = ptr[i];
    }
  }
  buf[i] = 0;
  return buf;
}

// index is the element index we want to open.
// It will start at beginning of buffer and decrement for each <start tag> of interest
// until it reaches 0, at which point it returns the tags name.
char * xp_open_element_skip_control(int index, int skip_scenario)
{
  char * ptr = xp_position[xp_stack];
  int level = 0;
  static char name[XP_MAX_NAME_LEN];

  DEBUG("xp_open_element_skip_control STARTED: index = %d, skip_scenario = %d, xp_stack = %d, ptr = '%s'\n", index, skip_scenario, xp_stack, debug_buffer(ptr));
  if (is_xp_file_metadata_valid()) DEBUG("Source File Location:\n%s\n", convert_whereami_key_to_string(ptr-xp_file).c_str());
  while(*ptr) {
    if (*ptr == '<') {
      if (!strncmp(ptr,"<![CDATA[", strlen("<![CDATA["))) {
        char * cdata_end = strstr(ptr, "]]>");
        if(!cdata_end) return NULL;
        ptr = cdata_end + 3;
      } else if (!strncmp(ptr,"<!--", strlen("<!--"))) {
        char * comment_end = strstr(ptr, "-->");
        if(!comment_end) return NULL;
        ptr = comment_end + 3;
      } else if (!strncmp(ptr,"<?xml", strlen("<?xml"))) {
        char * xml_end = strstr(ptr, "?>");
        if(!xml_end) return NULL;
        ptr = xml_end + 2;
      } else if (!strncmp(ptr,"<!DOCTYPE", strlen("<!DOCTYPE"))) {
        char * doctype_end = strstr(ptr, ">");
        if(!doctype_end) return NULL;
        ptr = doctype_end + 1;
      } else if ((skip_scenario) && !(strncmp(ptr,"<scenario", strlen("<scenario")))) {
        char * scenario_end = strstr(ptr, ">");
        DEBUG("  Skipping over embedded <scenario> tag (%d bytes); level = %d, index = %d\n", scenario_end-ptr, level, index);
        if(!scenario_end) return NULL;
        ptr = scenario_end + 1;
      } else if (*(ptr+1) == '/') {
        if (!strncmp(ptr, "</scenario", strlen("</scenario"))) {
          char * scenario_end = strchr(ptr, '>');
          DEBUG("  Skipping over embedded </scenario> tag (%d bytes); level = %d, index = %d\n", scenario_end-ptr, level, index);
          if(!scenario_end) return NULL;
          ptr = scenario_end + 1;
        } else {
          level--;
          if(level < 0) return NULL;
        }
      } else {
        // It's a < but not special
        if(level==0) {
          if (index) {
            index --;
            DEBUG("  < found, level=0, index>0: decrementing index; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
          } else {
            char * end = xp_find_start_tag_end(ptr + 1);
            char * p;
            if(!end) return NULL;

            p = strchr(ptr, ' ');
            if(p && (p < end))  {
              end = p;
            }
            p = strchr(ptr, '\t');
            if(p && (p < end))  {
              end = p;
            }
            p = strchr(ptr, '\r');
            if(p && (p < end))  {
              end = p;
            }
            p = strchr(ptr, '\n');
            if(p && (p < end))  {
              end = p;
            }
            p = strchr(ptr, '/');
            if(p && (p < end))  {
              end = p;
            }

            memcpy(name, ptr + 1, end-ptr-1);
            name[end-ptr-1] = 0;

            xp_position[++xp_stack] = end;
            DEBUG("  xp_open_element_skip_control Returning element '%s'; level = %d, index = %d, xp_stack = %d, ptr='%s'\n", name, level, index, xp_stack, debug_buffer(ptr));
            return name;
          }
        }

        /* We want to skip over this particular element .*/
        DEBUG("  We want to skip over this particular element, calling xp_find_start_tag_end; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
        ptr = xp_find_start_tag_end(ptr + 1);
        if (ptr) ptr--;
        level ++;
      }
    } else if((*ptr == '/') && (*(ptr+1) == '>')) {
      level --;
      DEBUG("  Found />; if (level<0) will return null; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
      if(level < 0) return NULL;
    }
    ptr++;
  }

  DEBUG("  xp_open_element_skip_control Reached end of file, returning NULL; level = %d, index = %d\n", level, index);
  return NULL;
}

void xp_close_element()
{
  if(xp_stack) {
    xp_stack--;
  }
}

void xp_root()
{
  xp_stack = 0;
}

char * xp_get_value(const char * name)
{
  int         index = 0;
  static char buffer[XP_MAX_FILE_LEN + 1];
  char      * ptr, *end, *check;

  end = xp_find_start_tag_end(xp_position[xp_stack] + 1);
  if(!end) return NULL;

  ptr = xp_position[xp_stack];

  while(*ptr) {
    ptr = strstr(ptr, name);

    if(!ptr) return NULL;
    if(ptr > end) return NULL;
    // FIXME: potential BUG in parser: we must retrieve full word,
    // so the use of strstr as it is is not enough.
    // we should check that the retrieved word is not a piece of another one.
    check = ptr-1;
    if(check >= xp_position[xp_stack]) {
      if((*check != '\r') &&
          (*check != '\n') &&
          (*check != '\t') &&
          (*check != ' ' )) {
        ptr += strlen(name);
        continue;
      }
    } else
      return(NULL);

    ptr += strlen(name);
    while((*ptr == '\r') ||
          (*ptr == '\n') ||
          (*ptr == '\t') ||
          (*ptr == ' ' )    ) {
      ptr ++;
    }
    if(*ptr != '=') continue;
    ptr ++;
    while((*ptr == '\r') ||
          (*ptr == '\n') ||
          (*ptr == '\t') ||
          (*ptr ==  ' ')    ) {
      ptr ++;
    }
    ptr++;
    if(*ptr) {
      while(*ptr) {
        if (*ptr == '\\') {
          ptr++;
          switch(*ptr) {
          case '\\':
            buffer[index++] = '\\';
            break;
          case '"':
            buffer[index++] = '\"';
            break;
          case 'n':
            buffer[index++] = '\n';
            break;
          case 't':
            buffer[index++] = '\t';
            break;
          case 'r':
            buffer[index++] = '\r';
            break;
          default:
            buffer[index++] = '\\';
            buffer[index++] = *ptr;
            break;
          }
          ptr++;
        } else if (*ptr=='"') {
          break;
        } else {
          buffer[index++] = *ptr++;
        }
        if(index > XP_MAX_FILE_LEN) return NULL;
      }
      buffer[index] = 0;
      xp_convert_special_characters(buffer);
      return buffer;
    }
  }
  return NULL;
}

char * xp_get_cdata()
{
  static char buffer[XP_MAX_FILE_LEN + 1];
  char      * end = xp_find_local_end();
  char      * ptr;

  ptr = strstr(xp_position[xp_stack],"<![CDATA[");
  if(!ptr) {
    return NULL;
  }
  ptr += 9;
  if(ptr > end) return NULL;
  end = strstr(ptr, "]]>");
  if(!end) {
    return NULL;
  }
  if((end -ptr) > XP_MAX_FILE_LEN) return NULL;
  memcpy(buffer, ptr, (end-ptr));
  buffer[end-ptr] = 0;
  return buffer;
}

int xp_get_content_length(char * P_buffer)
{
  char * L_ctl_hdr;
  int    L_content_length = -1 ;
  unsigned char   short_form;

  short_form = 0;

  L_ctl_hdr = strcasestr(P_buffer, "\nContent-Length:");
  if(!L_ctl_hdr) {
    L_ctl_hdr = strstr(P_buffer, "\nl:");
    short_form = 1;
  }

  if( L_ctl_hdr ) {
    if (short_form) {
      L_ctl_hdr += 3;
    } else {
      L_ctl_hdr += 16;
    }
    while(isspace(*L_ctl_hdr)) L_ctl_hdr++;
    sscanf(L_ctl_hdr, "%d", &L_content_length);
  }
  // L_content_length = -1 the message does not contain content-length
  return (L_content_length);
}

/* convert &lt; (<), &amp; (&), &gt; (>), &quot; ("), and &apos; (') */
void xp_convert_special_characters(char * buffer)
{
  char * src = strchr(buffer, '&');
  char * dst = src;
  if (!src)
    return ;

  /* start at first & and then copy inline from src to dest with correct characters */
  while (*src) {
    if (*src == '&') {
      src ++;

      int i;
      for (i = 0; xml_special_char_encoding[i]; i++) {
        int len = strlen(xml_special_char_encoding[i]);
        if (!strncmp(src, xml_special_char_encoding[i], len)) {
          *dst = xml_special_char[i];
          src += len - 1;
          break;
        }
      }

      if (xml_special_char_encoding[i] == NULL) {
        /* Illegal use of &, but we'll be lenient (in violation of XML rules) and allow it through */
        snprintf(err, MAXERRSIZE, "Illegal use of '&' in XML attribute: you should be using '&amp;' instead\r\n");
        store_error_message(err);
        *dst = *src;
      }

    } else {
      *dst = *src;
    }
    dst++;
    src++;
  }
  *dst = 0;


}

// routines to assist unit testing
// Note: you must explicitly declare these in the test case.

string xp_get_xmlbuffer()
{
  return string(xp_file);
}

void set_xp_parser_verbose(int value)
{
  verbose = value;
}

CompositeDocument build_xp_file_metadata(string sippFile, int dumpxml)
{
  // re-initialize globals for reset between tests.
  xp_file_metadata.reset();
  xp_file_metadata_is_not_valid =0;
  if (xp_set_xml_buffer_from_file(sippFile.c_str(),dumpxml))
    return xp_file_metadata;
  else {
    // return an empty composite document if we failed to load file
    printf("failed to load file into buffer\n");
    CompositeDocument cdoc;
    return cdoc;
  }

}

