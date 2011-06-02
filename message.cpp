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
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           Olivier Jacques
 *           From Hewlett Packard Company.
 *           Shriram Natarajan
 *           Peter Higginson
 *           Eric Miller
 *           Venkatesh
 *           Enrico Hartung
 *           Nasir Khan
 *           Lee Ballard
 *           Guillaume Teissier from FTR&D
 *           Wolfgang Beck
 *           Venkatesh
 *           Vlad Troyanker
 *           Charles P Wright from IBM Research
 *           Amit On from Followap
 *           Jan Andres from Freenet
 *           Ben Evans from Open Cloud
 *           Marc Van Diest from Belgacom
 *	     Stefan Esser
 *           Andy Aicken
 */

#include "sipp.hpp"
#include "message.hpp"

struct KeywordMap {
	const char *keyword;
	MessageCompType type;
};

typedef std::map<std::string, customKeyword> kw_map;
kw_map keyword_map;

/* These keywords take no parameters. */
struct KeywordMap SimpleKeywords[] = {
  {"remote_ip", E_Message_Remote_IP },
  {"remote_host", E_Message_Remote_Host },
  {"remote_port", E_Message_Remote_Port },
  {"transport", E_Message_Transport },
  {"local_ip", E_Message_Local_IP },
  {"local_ip_type", E_Message_Local_IP_Type },
  {"local_port", E_Message_Local_Port },
  {"server_ip", E_Message_Server_IP },
  {"media_ip", E_Message_Media_IP },
#ifdef PCAPPLAY
  {"auto_media_port", E_Message_Auto_Media_Port },
#endif
  {"media_port", E_Message_Media_Port },
  {"media_ip_type", E_Message_Media_IP_Type },
  {"call_number", E_Message_Call_Number },
  {"dynamic_id", E_Message_DynamicId }, // wrapping global counter
  {"pid", E_Message_PID },
  {"service", E_Message_Service },
  {"branch", E_Message_Branch },
  {"msg_index", E_Message_Index },
  {"len", E_Message_Len },
  {"tdmmap", E_Message_TDM_Map },
  {"clock_tick", E_Message_ClockTick },
  {"users", E_Message_Users },
  {"userid", E_Message_UserID },
  {"timestamp", E_Message_Timestamp },
  {"sipp_version", E_Message_SippVersion },
};

// Index of the last keyword in DialogSpecificKeywords that is also transaction-specific
// Used to enforce rule that dialog= attribute may not be specified when use_txn is specified.
const int HighestTransactionSpecificKeywordsIndex = 11;

/* These keywords take an optional dialog= parameter and no other processing. */
// First keywords are also transaction-specific. See HighestTransactionSpecificKeywordsIndex
/* Note: must place longer keyword names before shorter ones with same base (ie [cseq_method] before [cseq]) */
struct KeywordMap DialogSpecificKeywords[] = {
  {"cseq_method", E_Message_CSEQ_Method },
  {"cseq", E_Message_CSEQ },
  {"client_cseq_method", E_Message_Client_CSEQ_Method },
  {"client_cseq", E_Message_Client_CSEQ },
  {"server_cseq_method", E_Message_Server_CSEQ_Method },
  {"server_cseq", E_Message_Server_CSEQ },
  {"received_cseq_method", E_Message_Received_CSEQ_Method },
  {"received_cseq", E_Message_Received_CSEQ },
  {"last_cseq_number", E_Message_Last_CSeq_Number },
  {"last_branch", E_Message_Last_Branch },
  {"last_Request_URI", E_Message_Last_Request_URI },
  {"last_message", E_Message_Last_Message },
// end of transaction-specific keywords
  {"call_id", E_Message_Call_ID },
  {"next_url", E_Message_Next_Url },
  {"routes", E_Message_Routes },
  {"peer_tag_param", E_Message_Peer_Tag_Param },
  {"remote_tag_param", E_Message_Remote_Tag_Param },
  {"remote_tag", E_Message_Remote_Tag },
  {"local_tag_param", E_Message_Local_Tag_Param },
  {"local_tag", E_Message_Local_Tag },
  {"contact_uri", E_Message_Contact_Uri },
  {"contact_name_and_uri", E_Message_Contact_Name_And_Uri },
  {"to_uri", E_Message_To_Uri },
  {"to_name_and_uri", E_Message_To_Name_And_Uri },
  {"from_uri", E_Message_From_Uri },
  {"from_name_and_uri", E_Message_From_Name_And_Uri },
// contact_number?
};

#define KEYWORD_SIZE 256

SendingMessage::SendingMessage(scenario *msg_scenario, const char *src, bool skip_sanity, int dialog_number, bool use_txn) : 
  response(false), ack(false), cancel(false)
{
  // should we parse out the _n portion of call here or later? Here would be faster and more
  // in keepin with the existing style, I suppose...
  const char *osrc = src;
  char * literal;
  int    literalLen;
  char * dest;
  char * key;
  char   current_line[MAX_HEADER_LEN];
  const char * line_mark = NULL;
  const char * tsrc;
  int    num_cr = get_cr_number(src);

  this->msg_scenario = msg_scenario;
  this->dialog_number = dialog_number;

  dest = literal = (char *)malloc(strlen(src) + num_cr + 1);
  literalLen = 0;

  current_line[0] = '\0';
  *dest = 0;

  while(*src) {
    /* If start of new line, copy through next \n into current_line */
    if (current_line[0] == '\0') {
      line_mark = strchr(src, '\n');
      if (line_mark) {
        int header_len = line_mark - src;
        if (header_len > MAX_HEADER_LEN-1)
          header_len = MAX_HEADER_LEN-1;
        memcpy(current_line, src, header_len);
        current_line[header_len] = '\0';
      }
    }

    /* This hex encoding could be done in XML parsing, allowing us to skip
    * these conditionals and branches. */
    if ((*src == '\\') && (*(src+1) == 'x')) {
      /* Allows any hex coded char like '\x5B' ([) */
      src += 2;
      if (isxdigit(*src)) {
        int val = get_decimal_from_hex(*src);
        src++;
        if (isxdigit(*src)) {
          val = (val << 4) + get_decimal_from_hex(*src);
        }
        *dest++ = val & 0xff;
      }
      src++;
    } else if (*src == '\n') {
      *dest++ = '\r';
      *dest++ = *src++;
      current_line[0] = '\0';
    } else if (*src != '[') {
      *dest++ = *src++;
    } else {
      /* We have found a keyword, store the literal that we have been generating. */
      literalLen = dest - literal;
      if (literalLen) {
        *dest = '\0';
        literal = (char *)realloc(literal, literalLen + 1);
        if (!literal) { ERROR("Out of memory!"); }

        MessageComponent *newcomp = (MessageComponent *)calloc(1, sizeof(MessageComponent));
        if (!newcomp) { ERROR("Out of memory!"); }

        newcomp->type = E_Message_Literal;
        newcomp->literal = literal;
        newcomp->literalLen = literalLen; // length without the terminator 
        newcomp->dialog_number = dialog_number;
        messageComponents.push_back(newcomp);
      } else {
        free(literal);
      }

      dest = literal = (char *)malloc(strlen(src) + num_cr + 1);
      literalLen = 0;
      *dest = '\0';

      /* Now lets determine which keyword we have. */
      MessageComponent *newcomp = (MessageComponent *)calloc(1, sizeof(MessageComponent));
      if (!newcomp) { ERROR("Out of memory!"); }

      newcomp->dialog_number = dialog_number;

      char keyword [KEYWORD_SIZE+1];
      src++;

      /* Like strchr, but don't count things in quotes. */
      for(tsrc = src; *tsrc; tsrc++) {
        if (*tsrc == '\"') {
          do {
            tsrc++;
          } while(*tsrc && *tsrc != '\"');
          if (!*tsrc) {
            break;
          }
        }
        if (*tsrc == '[')
          break;
      }
      if (*tsrc != '[') {
        tsrc = NULL;
      }

      /* Like strchr, but don't count things in quotes. */
      // cast away const of src [*UGLY*]
      for(key = (char *) src; *key; key++) {
        if (*key == '\"') {
          do {
            key++;
          } while(*key && *key != '\"');
        }
        if (*key == ']')
          break;
      }
      if (*key != ']') {
        key = NULL;
      }

      if ((tsrc) && (tsrc<key)){
        memcpy(keyword, src-1,  tsrc - src + 1);
        src=tsrc+1;
        dest += sprintf(dest, "%s", keyword);
      }

      if((!key) || ((key - src) > KEYWORD_SIZE) || (!(key - src))){
        ERROR("Syntax error or invalid [keyword] in scenario while parsing '%s'", current_line);
      }
      memcpy(keyword, src,  key - src);
      keyword[key - src] = 0;
      src = key + 1;
      // allow +/-n for numeric variables
      newcomp->offset = 0;
      if ((strncmp(keyword, "authentication", strlen("authentication")) && strncmp(keyword, "tdmmap", strlen("tdmmap")))){
        newcomp->offset = parseOffset(keyword);
        /* end the string before the +/- sign so that the parser doesn't read it as an unrecognized keyword */
        if(((key = strchr(keyword,'+')) || (key = strchr(keyword,'-'))) && isdigit(*(key+1))) *key = 0;
      }

      char *spc = NULL;
      char ospc;
      if ((spc = strchr(keyword, ' '))) {
        ospc = *spc;
        *spc = '\0';
      }
      kw_map::iterator it = keyword_map.find(keyword);
      if (spc) {
        *spc = ospc;
      }

      if (it != keyword_map.end()) {
        newcomp->type = E_Message_Custom;
        newcomp->comp_param.fxn = it->second;
        messageComponents.push_back(newcomp);
        continue;
      }

      bool simple_keyword = false;
      for (unsigned int i = 0; i < sizeof(SimpleKeywords)/sizeof(SimpleKeywords[0]); i++) {
        if (!strcmp(keyword, SimpleKeywords[i].keyword)) {
          newcomp->type = SimpleKeywords[i].type;
          simple_keyword = true;
          break;
        }
      }

      bool dialog_keyword = false;
      for (unsigned int i = 0; i < sizeof(DialogSpecificKeywords)/sizeof(DialogSpecificKeywords[0]); i++) {
        if (strstr(keyword, DialogSpecificKeywords[i].keyword) == keyword) {
          newcomp->type = DialogSpecificKeywords[i].type;
          parse_dialog_number(keyword, newcomp);
          if ((use_txn) && (i <= HighestTransactionSpecificKeywordsIndex) && (newcomp->dialog_number != dialog_number)) {
            ERROR("Cannot use 'dialog=' attribute in [%s] keyword when also specifying 'use_txn' for the message.\n", keyword);
          }
          dialog_keyword = true;
          break;
        }
      }

      if (simple_keyword || dialog_keyword) {
        messageComponents.push_back(newcomp);
        continue;
      }

      if(!strncmp(keyword, "field", strlen("field"))) {
        newcomp->type = E_Message_Injection;

        /* Parse out the interesting things like file and number. */
        newcomp->comp_param.field_param.field = atoi(keyword + strlen("field"));

        char fileName[KEYWORD_SIZE];
        getKeywordParam(keyword, "file=", fileName);
        if (fileName[0] == '\0') {
          if (!default_file) {
            ERROR("No injection file was specified!\n");
          }
          newcomp->comp_param.field_param.filename = strdup(default_file);
        } else {
          newcomp->comp_param.field_param.filename = strdup(fileName);
        }
        if (inFiles.find(newcomp->comp_param.field_param.filename) == inFiles.end()) {
          ERROR("Invalid injection file: %s\n", fileName);
        }

        char line[KEYWORD_SIZE];
        getKeywordParam(keyword, "line=", line);
        if (line[0]) {
          /* Turn this into a new message component. */
          newcomp->comp_param.field_param.line = new SendingMessage(msg_scenario, line, true, dialog_number);
        }
      } else if(!strncmp(keyword, "file", strlen("file"))) {
        newcomp->type = E_Message_File;

        /* Parse out the interesting things like file and number. */
        char fileName[KEYWORD_SIZE];
        getKeywordParam(keyword, "name=", fileName);
        if (fileName[0] == '\0') {
          ERROR("No name specified for 'file' keyword!\n");
        }
        /* Turn this into a new message component. */
        newcomp->comp_param.filename = new SendingMessage(msg_scenario, fileName, true, dialog_number);
      } else if(*keyword == '$') {
        newcomp->type = E_Message_Variable;
        if (!msg_scenario) {
          ERROR("SendingMessage with variable usage outside of scenario!");
        }
        newcomp->varId = msg_scenario->get_var(keyword + 1, "Variable keyword");
      } else if(!strncmp(keyword, "fill", strlen("fill"))) {
        newcomp->type = E_Message_Fill;
        char filltext[KEYWORD_SIZE];
        char varName[KEYWORD_SIZE];

        getKeywordParam(keyword, "text=", filltext);
        if (filltext[0] == '\0') {
          strcpy(filltext, "X");
        }
        getKeywordParam(keyword, "variable=", varName);

        newcomp->literal = strdup(filltext);
        newcomp->literalLen = strlen(newcomp->literal);
        if (!msg_scenario) {
          ERROR("SendingMessage with variable usage outside of scenario!");
        }
        newcomp->varId = msg_scenario->get_var(varName, "Fill Variable");

      } else if(!strncmp(keyword, "last_", strlen("last_"))) {
        newcomp->type = E_Message_Last_Header;

        // parse optional dialog parameter 
        if (parse_dialog_number(keyword, newcomp)) {
          // if dialog= specified, only copy header portion
          if (use_txn) {
            ERROR("Cannot use 'dialog=' attribute in [last_] keyword when also specifying 'use_txn' for the message.\n");
          }
          const char *diagptr = strstr(keyword, "dialog=");
          assert(diagptr);
          while ((diagptr > keyword) && (*(diagptr-1) == ' ')) diagptr--;
          newcomp->literal = strndup(keyword + strlen("last_"), diagptr - keyword - strlen("last_"));
        }
        else
          newcomp->literal = strdup(keyword + strlen("last_"));
        newcomp->literalLen = strlen(newcomp->literal);

      } else if(!strncmp(keyword, "authentication", strlen("authentication"))) {
        parseAuthenticationKeyword(msg_scenario, newcomp, keyword);
      }
#ifndef PCAPPLAY
      else if(!strcmp(keyword, "auto_media_port") ||
        !strcmp(keyword, "media_port") ||
        !strcmp(keyword, "media_ip") ||
        !strcmp(keyword, "media_ip_type")) {
          ERROR("The %s keyword requires PCAPPLAY.\n", keyword);
      }
#endif
#ifndef _USE_OPENSSL
      else if(!strcmp(keyword, "authentication")) {
        ERROR("The %s keyword requires OpenSSL.\n", keyword);
      }
#endif
      else {
        // scan for the generic parameters - must be last test

        int i = 0;
        while (generic[i]) {
          char *msg1 = *generic[i];
          char *msg2 = *(generic[i] + 1);
          if(!strcmp(keyword, msg1)) {
            newcomp->type = E_Message_Literal;
            newcomp->literal = strdup(msg2);
            newcomp->literalLen = strlen(newcomp->literal);
            break;
          }
          ++i;
        }
        if (!generic[i]) {
          ERROR("Unsupported keyword '%s' in xml scenario file",
            keyword);
        }
      }

      messageComponents.push_back(newcomp);
    }
  }
  if (literal[0]) {
    *dest++ = '\0';
    literalLen = dest - literal;
    literal = (char *)realloc(literal, literalLen);
    if (!literal) { ERROR("Out of memory!"); } 

    MessageComponent *newcomp = (MessageComponent *)calloc(1, sizeof(MessageComponent));
    if (!newcomp) { ERROR("Out of memory!"); }

    newcomp->type = E_Message_Literal;
    newcomp->literal = literal;
    newcomp->literalLen = literalLen-1;
    messageComponents.push_back(newcomp);
  } else {
    free(literal);
  }

  if (skip_sanity) {
    cancel = response = ack = false;
    method = NULL;
    return;
  }

  if (numComponents() < 1) {
    ERROR("Can not create a message that is empty!");
  }
  if (getComponent(0)->type != E_Message_Literal) {
    ERROR("You can not use a keyword for the METHOD or to generate \"SIP/2.0\" to ensure proper [cseq] operation!\n%s\n", osrc);
  }

  char *p = method = strdup(getComponent(0)->literal);
  char *q;
  while (isspace(*p)) {
    p++;
  }
  if (!(q = strchr(method, ' '))) {
    ERROR("You can not use a keyword for the METHOD or to generate \"SIP/2.0\" to ensure proper [cseq] operation!%s\n", osrc);
  }
  *q++ = '\0';
  while (isspace(*q)) { q++; }
  if (!strcmp(method, "SIP/2.0")) {
    char *endptr;
    code = strtol(q, &endptr, 10);
    if (*endptr && !isspace(*endptr)) {
      ERROR("Invalid reply code: %s\n", q);
    }
    if (code < 100 || code >= 700) {
      ERROR("Response codes must be in the range of 100-700");
    }
    response = true;
    ack = false;
    cancel = false;
    free(method);
    method = NULL;
  } else {
    if (p != method) {
      memmove(method, p, strlen(p) + 1);
    }
    method = (char *)realloc(method, strlen(method) + 1);
    if (!method) { ERROR("Out of memory"); }
    ack = (!strcmp(method, "ACK"));
    cancel = (!strcmp(method, "CANCEL"));
    response = false;
  }
}

SendingMessage::~SendingMessage() {
  for (int i = 0; i < numComponents(); i++) {
    freeMessageComponent(messageComponents[i]);
  }
  free(method);
}

bool SendingMessage::isAck() { return ack; }
bool SendingMessage::isCancel() { return cancel; }
bool SendingMessage::isResponse() { return response; }
char *SendingMessage::getMethod() { return method; }
int SendingMessage::getCode() { return code; }

void SendingMessage::getQuotedParam(char * dest, char * src, int * len)
{
  *len=0;
  /* Allows any hex coded string like '0x5B07F6' */
  while (char c = *src++) {
    switch(c) {
      case '"':
	*len++;
	*dest = '\0';
	return;
      case '\\':
	c = *src++;
	*len++;
	if (c == 0) {
	  *dest = '\0';
	  return;
	}
	/* Fall-Through. */
      default:
	*dest++ = c;
	*len++;
    }
  }
  *dest = '\0';
}

void SendingMessage::getHexStringParam(char * dest, char * src, int * len)
{
  *len=0;
  /* Allows any hex coded string like '0x5B07F6' */
  while (isxdigit(*src)) {
    int val = get_decimal_from_hex(*src);
    src++;
    if (isxdigit(*src)) {
      val = (val << 4) + get_decimal_from_hex(*src);
      src++;
    }
    *dest++ = val & 0xff;
    (*len)++;
  }
}

void SendingMessage::getKeywordParam(char * src, const char * param, char * output)
{
  char *key, *tmp;
  int len;

  len = 0;
  key = NULL;
  if ((tmp = strstr(src, param))) {
    tmp += strlen(param);
    key = tmp;
    if ((*key == '0') && (*(key+1) == 'x')) {
      key += 2;
      getHexStringParam(output, key, &len);
      key += len * 2;
    } else if (*key == '\"') {
      key++;
      getQuotedParam(output, key, &len);
      key += len;
    } else {
      while (*key) {
        if (((key - src) > KEYWORD_SIZE) || (!(key - src))) {
          ERROR("Syntax error parsing '%s' parameter", param);
        } else if (*key == ']' || *key < 33 || *key > 126) {
          break;
        }
        key++;
      }
      strncpy(output, tmp, key-tmp);
      output[key-tmp] = '\0';
    }
  } else {
    output[0] = '\0';
  }
}

// store dialog number in MessageComponent if specified
// return value of true indicates dialog= keyword was found
bool SendingMessage::parse_dialog_number(char * src, struct MessageComponent* newcomp)
{    
  char dialogNumber[KEYWORD_SIZE];
  getKeywordParam(src, "dialog=", dialogNumber);
  if (dialogNumber[0] != '\0') {
    char *endptr;
    newcomp->dialog_number = strtol(dialogNumber, &endptr, 10);
    if (endptr == dialogNumber)
      ERROR("Invalid dialog number specified in last_ tag.");
    return true;
  }
  else return false;
}

void SendingMessage::parseAuthenticationKeyword(scenario *msg_scenario, struct MessageComponent *dst, char *keyword) {
  char my_auth_user[KEYWORD_SIZE + 1];
  char my_auth_pass[KEYWORD_SIZE + 1];
  char my_aka[KEYWORD_SIZE + 1];

  dst->type = E_Message_Authentication;

  memset(my_auth_user,0,KEYWORD_SIZE);
  memset(my_auth_pass,0,KEYWORD_SIZE);
  /* Look for optional username and password parameters */
  getKeywordParam(keyword, "username=", my_auth_user);
  getKeywordParam(keyword, "password=", my_auth_pass);

  if(*my_auth_user == '\0') {
    strcpy(my_auth_user, service);
  }
  if(*my_auth_pass == '\0') {
    strcpy(my_auth_pass, auth_password);
  }


  dst->comp_param.auth_param.auth_user = new SendingMessage(msg_scenario, my_auth_user, true /* skip sanity */);
  dst->comp_param.auth_param.auth_pass = new SendingMessage(msg_scenario, my_auth_pass, true);

  /* add aka_OP, aka_AMF, aka_K */
  getKeywordParam(keyword, "aka_K=", my_aka);
  if (my_aka[0]==0){
    memcpy(my_aka,my_auth_pass,16);
    my_aka[16]=0;
  }
  dst->comp_param.auth_param.aka_K = new SendingMessage(msg_scenario, my_aka, true);

  getKeywordParam(keyword, "aka_OP=", my_aka);
  dst->comp_param.auth_param.aka_OP = new SendingMessage(msg_scenario, my_aka, true);
  getKeywordParam(keyword, "aka_AMF=", my_aka);
  dst->comp_param.auth_param.aka_AMF = new SendingMessage(msg_scenario, my_aka, true);
}

void SendingMessage::freeMessageComponent(struct MessageComponent *comp) {
  free(comp->literal);
  if (comp->type == E_Message_Authentication) {
    if (comp->comp_param.auth_param.auth_user) {
      delete comp->comp_param.auth_param.auth_user;
    }
    if (comp->comp_param.auth_param.auth_pass) {
      delete comp->comp_param.auth_param.auth_pass;
    }
    if (comp->comp_param.auth_param.aka_K) {
      delete comp->comp_param.auth_param.aka_K;
    }
    if (comp->comp_param.auth_param.aka_AMF) {
      delete comp->comp_param.auth_param.aka_AMF;
    }
    if (comp->comp_param.auth_param.aka_OP) {
      delete comp->comp_param.auth_param.aka_OP;
    }
  } else if (comp->type == E_Message_Injection) {
    free(comp->comp_param.field_param.filename);
  }
  free(comp);
}

int SendingMessage::numComponents() {
  return messageComponents.size();
}
struct MessageComponent *SendingMessage::getComponent(int i) {
  return messageComponents[i];
}

/* This is very simplistic and does not yet allow any arguments, but it is a start. */
int registerKeyword(char *keyword, customKeyword fxn) {
	if (keyword_map.find(keyword) != keyword_map.end()) {
		ERROR("Can not register keyword '%s', already registered!\n", keyword);
	}
	keyword_map[keyword] = fxn;
	return 0;
}
