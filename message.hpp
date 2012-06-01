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
 *           Andy Aicken
 */

#ifndef __MESSAGE__
#define __MESSAGE__

#include <vector>

class scenario;

struct MessageComponent;

typedef enum {
  E_Message_Literal,
  E_Message_Remote_IP,
  E_Message_Remote_Host,
  E_Message_Remote_Port,
  E_Message_Transport,
  E_Message_Local_IP,
  E_Message_Local_IP_Type,
  E_Message_Local_Port,
  E_Message_Server_IP,
  E_Message_Media_IP,
  E_Message_Auto_Media_Port,
  E_Message_Media_Port,
  E_Message_Media_IP_Type,
  E_Message_Call_Number,
  E_Message_DynamicId,   // general usage, global, autoincrementing and wrapping counter
  E_Message_Call_ID,
  E_Message_CSEQ,
  E_Message_CSEQ_Method,
  E_Message_Client_CSEQ,
  E_Message_Client_CSEQ_Method,
  E_Message_Received_CSEQ,
  E_Message_Received_CSEQ_Method,
  E_Message_Server_CSEQ,        // synonym with Received_CSEQ
  E_Message_Server_CSEQ_Method, // synonym with Received_CSEQ_Method
  E_Message_PID,
  E_Message_Service,
  E_Message_Branch,
  E_Message_Index,
  E_Message_Next_Url,
  E_Message_Len,
  E_Message_Peer_Tag_Param,
  E_Message_Remote_Tag_Param,
  E_Message_Remote_Tag,
  E_Message_Local_Tag_Param,
  E_Message_Local_Tag,
  E_Message_Contact_Uri,
  E_Message_Contact_Name_And_Uri,
  E_Message_To_Uri,
  E_Message_To_Name_And_Uri,
  E_Message_From_Uri,
  E_Message_From_Name_And_Uri,
  E_Message_Routes,
  E_Message_Variable,
  E_Message_Fill,
  E_Message_Injection,
  E_Message_Last_Header,
  E_Message_Last_Request_URI,
  E_Message_Last_CSeq_Number,
  E_Message_Last_Branch,
  E_Message_Last_Message,
  E_Message_TDM_Map,
  E_Message_Authentication,
  E_Message_ClockTick,
  E_Message_Users,
  E_Message_UserID,
  E_Message_Timestamp,
  E_Message_SippVersion,
  E_Message_File,
  E_Message_Custom,
  E_Message_Remote_IP_noesc,
  E_Message_Local_IP_noesc,
  E_Message_Server_IP_noesc,
  E_Message_Media_IP_noesc,
} MessageCompType;

class SendingMessage {
public:
  SendingMessage(scenario *msg_scenario, const char *msg, bool skip_sanity = false, int dialog_number = -1, bool use_txn = false);
  ~SendingMessage();

  struct MessageComponent *getComponent(int);
  int numComponents();

  char *getMethod();
  int getCode();

  bool isResponse();
  bool isAck();
  bool isCancel();

  int getDialogNumber() {
    return dialog_number;
  }

  static void parseAuthenticationKeyword(scenario *msg_scenario, struct MessageComponent *dst, char *keyword);
  static void freeMessageComponent(struct MessageComponent *comp);
private:
  std::vector <struct MessageComponent *> messageComponents;

  char *method;
  int code;

  bool ack;
  bool cancel;
  bool response;

  scenario *msg_scenario;

  // Must store dialog number as many keywords will have different values depending on this
  int dialog_number;

  // Get parameters from a [keyword]
  static void getQuotedParam(char * dest, char * src, int * len);
  static void getHexStringParam(char * dest, char * src, int * len);
  static void getKeywordParam(char * src, const char * param, char * output);

  // store dialog number in message component if specified with tag
  // return value of true indicates dialog= keyword was found
  bool parse_dialog_number(char * src, struct MessageComponent* newcomp);
  bool parse_value_only (char * src, struct MessageComponent* newcomp);
  void parse_encoding (char * src, struct MessageComponent* newcomp);
  void parse_generated (char * src, struct MessageComponent* newcomp);

};

/* Custom Keyword Function Type. */
struct MessageComponent;
class call;
typedef int (*customKeyword)(call *, struct MessageComponent *, char *, int);
/* Custom Keyword Registration Function. */
int registerKeyword(char *keyword, customKeyword fxn);

struct MessageComponent {
  MessageCompType type;
  char *literal;
  int encoding; // method for encoding header value (0 indicates basic encoding)
  int literalLen;
  int offset;        // amount added or subtracted from stored value (-1 in case of [cseq-1])
  int varId;
  int dialog_number; // component may refer to another dialog in case of _n syntax
  bool valueOnly;  // don't include header when printing info out.
  bool auto_generate_remote_tag;
  union u {
    /* Authentication Parameters. */
    struct {
      SendingMessage *auth_user;
      SendingMessage *auth_pass;
      SendingMessage *aka_OP;
      SendingMessage *aka_AMF;
      SendingMessage *aka_K;
    } auth_param;
    /* Field Substitution. */
    struct {
      char *filename;
      int field;
      SendingMessage *line;
    } field_param;
    SendingMessage *filename;
    customKeyword fxn;
  } comp_param;
};


#endif
