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
 *           Marc LAMBERTON
 *           Olivier JACQUES
 *           Herve PELLAN
 *           David MANSUTTI
 *           Francois-Xavier Kowalski
 *           Gerard Lyonnaz
 *           From Hewlett Packard Company.
 *           F. Tarek Rogers
 *           Peter Higginson
 *           Vincent Luba
 *           Shriram Natarajan
 *           Guillaume Teissier from FTR&D
 *           Clement Chen
 *           Wolfgang Beck
 *           Charles P Wright from IBM Research
 */

#include <assert.h>
#include <stdio.h>

#include "opentask.hpp"
#include "reporttask.hpp"
#include "scenario.hpp"
#include "screen.hpp"
#include "sipp_globals.hpp"
#include "win32_compatibility.hpp"
#include "stat.hpp"
#include <time.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

class stattask *stattask::instance = NULL;
class screentask *screentask::instance = NULL;

void stattask::initialize()
{
  assert(instance == NULL);
  if (dumpInFile || useCountf || rate_increase) {
    instance = new stattask();
  }
}

void screentask::initialize()
{
  assert(instance == NULL);
  if (report_freq) {
    instance = new screentask();
  }
}

void stattask::dump()
{
  WARNING("Statistics reporting task.");
}
void screentask::dump()
{
  WARNING("Screen update task.");
}

void screentask::report(bool last)
{
  if (print_statistics(last)==INTERNAL_ERROR)
    REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
  display_scenario->stats->computeStat(CStat::E_RESET_PD_COUNTERS);
  last_report_time  = getmilliseconds();
  scheduling_loops = 0;
}

bool screentask::run()
{
  if (quitting > 11) {
    delete this;
    return false;
  }

  if (getmilliseconds() - last_report_time >= report_freq) {
    report(false);
  }

  setPaused();
  return true;
}

unsigned int screentask::wake()
{
  return last_report_time + report_freq;
}

void stattask::report()
{
  if(dumpInFile) {
    main_scenario->stats->dumpData();
  }
  if (useCountf) {
    print_count_file(countf, 0);
  }

  main_scenario->stats->computeStat(CStat::E_RESET_PL_COUNTERS);
  last_dump_time = clock_tick;
}

bool stattask::run()
{
  /* Statistics Logs. */
  if((getmilliseconds() - last_dump_time) >= report_freq_dumpLog)  {
    if (rate_increase) {
      rate += rate_increase;
      if (rate_max && (rate > rate_max)) {
        rate = rate_max;
        if (rate_quit) {
          quitting += 10;
        }
      }
      opentask::set_rate(rate);
    }
    report();
  }
  setPaused();
  return true;
}

unsigned int stattask::wake()
{
  return last_dump_time + report_freq_dumpLog;
}






// screen printing functions used by sipp and reporttask from sipp.cpp
/************** Statistics display  *************/

void print_header_line(FILE *f, int last)
{
  switch(currentScreenToDisplay) {
  case DISPLAY_STAT_SCREEN :
    fprintf(f,"----------------------------- Statistics Screen ------- [1-9]: Change Screen --" SIPP_ENDL);
    break;
  case DISPLAY_REPARTITION_SCREEN :
    fprintf(f,"---------------------------- Repartition Screen ------- [1-9]: Change Screen --" SIPP_ENDL);
    break;
  case DISPLAY_VARIABLE_SCREEN  :
    fprintf(f,"----------------------------- Variables Screen -------- [1-9]: Change Screen --" SIPP_ENDL);
    break;
  case DISPLAY_TDM_MAP_SCREEN  :
    fprintf(f,"------------------------------ TDM map Screen --------- [1-9]: Change Screen --" SIPP_ENDL);
    break;
  case DISPLAY_SECONDARY_REPARTITION_SCREEN :
    fprintf(f,"--------------------------- Repartition %d Screen ------ [1-9]: Change Screen --" SIPP_ENDL, currentRepartitionToDisplay);
    break;
  case DISPLAY_SCENARIO_SCREEN :
  default:
    fprintf(f,"------------------------------ Scenario Screen -------- [1-9]: Change Screen --" SIPP_ENDL);
    break;
  }
}


void print_variable_list()
{
  CActions  * actions;
  CAction   * action;
  int printed = 0;
  bool found;

  printf("Action defined Per Message :" SIPP_ENDL);
  printed++;
  found = false;
  for(unsigned int i=0; i<display_scenario->messages.size(); i++) {
    message *curmsg = display_scenario->messages[i];
    actions = curmsg->M_actions;
    if(actions != NULL) {
      switch(curmsg->M_type) {
      case MSG_TYPE_RECV:
        printf("=> Message[%d] (Receive Message) - "
               "[%d] action(s) defined :" SIPP_ENDL,
               i,
               actions->getActionSize());
        printed++;
        break;
      case MSG_TYPE_RECVCMD:
        printf("=> Message[%d] (Receive Command Message) - "
               "[%d] action(s) defined :" SIPP_ENDL,
               i,
               actions->getActionSize());
        printed++;
        break;
      default:
        printf("=> Message[%d] - [%d] action(s) defined :" SIPP_ENDL,
               i,
               actions->getActionSize());
        printed++;
        break;
      }

      for(int j=0; j<actions->getActionSize(); j++) {
        action = actions->getAction(j);
        if(action != NULL) {
          printf("   --> action[%d] = ", j);
          action->afficheInfo();
          printf(SIPP_ENDL);
          printed++;
          found = true;
        }
      }
    }
  }
  if(!found) {
    printed++;
    printf("=> No action found on any messages"SIPP_ENDL);
  }

  printf(SIPP_ENDL);
  for(unsigned int i=0; i<(display_scenario->messages.size() + 5 - printed); i++) {
    printf(SIPP_ENDL);
  }
}


void print_tdm_map()
{
  int interval = 0;
  int i = 0;
  int in_use = 0;
  interval = (tdm_map_a+1) * (tdm_map_b+1) * (tdm_map_c+1);

  printf("TDM Circuits in use:"  SIPP_ENDL);
  while (i<interval) {
    if (tdm_map[i]) {
      printf("*");
      in_use++;
    } else {
      printf(".");
    }
    i++;
    if (i%(tdm_map_c+1) == 0) printf(SIPP_ENDL);
  }
  printf(SIPP_ENDL);
  printf("%d/%d circuits (%d%%) in use", in_use, interval, int(100*in_use/interval));
  printf(SIPP_ENDL);
  for(unsigned int i=0; i<(display_scenario->messages.size() + 8 - int(interval/(tdm_map_c+1))); i++) {
    printf(SIPP_ENDL);
  }
}

/*
  return codes
   0 ok
  -1 not initialized yet
  SCENARIO_NOT_IMPLEMENTED 

  */
int print_stats_in_file(FILE * f, int last, int diagram_only)
{
  static char temp_str[256];
  int divisor;



  /* We are not initialized yet. */
  if (!display_scenario) {
    return -1;
  }

  if (!diagram_only) {
    /* Optional timestamp line for files only */
    if(f != stdout) {
      time_t tim;
      time(&tim);
      fprintf(f, "  Timestamp: %s" SIPP_ENDL, ctime(&tim));
    }

    /* Header line with global parameters */
    if (users >= 0) {
      sprintf(temp_str, "%d (%d ms)", users, duration);
    } else {
      sprintf(temp_str, "%3.1f(%d ms)/%5.3fs", rate, duration, (double)rate_period_ms / 1000.0);
    }
    unsigned long long total_calls = display_scenario->stats->GetStat(CStat::CPT_C_IncomingCallCreated) + display_scenario->stats->GetStat(CStat::CPT_C_OutgoingCallCreated);
    if( creationMode == MODE_SERVER) {
      fprintf
      (f,
       "  Port   Total-time  Total-calls  Transport"
       SIPP_ENDL
       "  %-5d %6lu.%02lu s     %8llu  %s"
       SIPP_ENDL SIPP_ENDL,
       local_port,
       clock_tick / 1000, (clock_tick % 1000) / 10,
       total_calls,
       TRANSPORT_TO_STRING(transport));
    } else if( creationMode == MODE_CLIENT)  {
      assert(creationMode == MODE_CLIENT);
      if (users >= 0) {
        fprintf(f, "     Users (length)");
      } else {
        fprintf(f, "  Call-rate(length)");
      }
      fprintf(f, "   Port   Total-time  Total-calls  Remote-host" SIPP_ENDL
              "%19s   %-5d %6lu.%02lu s     %8llu  %s:%d(%s)" SIPP_ENDL SIPP_ENDL,
              temp_str,
              local_port,
              clock_tick / 1000, (clock_tick % 1000) / 10,
              total_calls,
              remote_ip,
              remote_port,
              TRANSPORT_TO_STRING(transport));
    } else {
      fprintf(f, "Neither CLIENT nor SERVER mode.");
    }

    /* 1st line */
    if(total_calls < stop_after) {
      sprintf(temp_str, "%llu new calls during %lu.%03lu s period ",
              display_scenario->stats->GetStat(CStat::CPT_PD_IncomingCallCreated) +
              display_scenario->stats->GetStat(CStat::CPT_PD_OutgoingCallCreated),
              (clock_tick-last_report_time) / 1000,
              ((clock_tick-last_report_time) % 1000));
    } else {
      sprintf(temp_str, "Call limit reached (-m %lu), %lu.%03lu s period ",
              stop_after,
              (clock_tick-last_report_time) / 1000,
              ((clock_tick-last_report_time) % 1000));
    }
    divisor = scheduling_loops;
    if(!divisor) {
      divisor = 1;
    }
    fprintf(f,"  %-38s %lu ms scheduler resolution"
            SIPP_ENDL,
            temp_str,
            (clock_tick-last_report_time) / divisor);

    /* 2nd line */
    if( creationMode == MODE_SERVER) {
      sprintf(temp_str, "%llu calls", display_scenario->stats->GetStat(CStat::CPT_C_CurrentCall));
    } else {
      sprintf(temp_str, "%llu calls (limit %d)", display_scenario->stats->GetStat(CStat::CPT_C_CurrentCall), open_calls_allowed);
    }
    fprintf(f,"  %-38s Peak was %llu calls, after %llu s" SIPP_ENDL,
            temp_str,
            display_scenario->stats->GetStat(CStat::CPT_C_CurrentCallPeak),
            display_scenario->stats->GetStat(CStat::CPT_C_CurrentCallPeakTime));
    fprintf(f,"  %d Running, %d Paused, %d Woken up" SIPP_ENDL,
            last_running_calls, last_paused_calls, last_woken_calls);
    last_woken_calls = 0;

    /* 3rd line dead call msgs, and optional out-of-call msg */
    sprintf(temp_str,"%llu dead call msg (discarded)",
            display_scenario->stats->GetStat(CStat::CPT_G_C_DeadCallMsgs));
    fprintf(f,"  %-37s", temp_str);
    if( creationMode == MODE_CLIENT) {
      sprintf(temp_str,"%llu out-of-call msg (discarded)",
              display_scenario->stats->GetStat(CStat::CPT_G_C_OutOfCallMsgs));
      fprintf(f,"  %-37s", temp_str);
    }
    fprintf(f,SIPP_ENDL);

#ifndef WIN32
    if(compression) {
      fprintf(f,"  Comp resync: %d sent, %d recv" ,
              resynch_send, resynch_recv);
      fprintf(f,SIPP_ENDL);
    }
#endif

    /* 4th line , sockets and optional errors */
    sprintf(temp_str,"%d open sockets",
            pollnfds);
    fprintf(f,"  %-38s", temp_str);
    if(nb_net_recv_errors || nb_net_send_errors || nb_net_cong) {
      fprintf(f,"  %lu/%lu/%lu %s errors (send/recv/cong)" SIPP_ENDL,
              nb_net_send_errors,
              nb_net_recv_errors,
              nb_net_cong,
              TRANSPORT_TO_STRING(transport));
    } else {
      fprintf(f,SIPP_ENDL);
    }

#ifdef PCAPPLAY
    /* if has media abilities */
    if (hasMedia != 0) {
      sprintf(temp_str, "%lu Total RTP pckts sent ",
              rtp_pckts_pcap);
      if (clock_tick-last_report_time) {
        fprintf(f,"  %-38s %lu.%03lu last period RTP rate (kB/s)" SIPP_ENDL,
                temp_str,
                (rtp_bytes_pcap)/(clock_tick-last_report_time),
                (rtp_bytes_pcap)%(clock_tick-last_report_time));
      }
      rtp_bytes_pcap = 0;
      rtp2_bytes_pcap = 0;
    }
#endif

    /* 5th line, RTP echo statistics */
    if (rtp_echo_enabled && (media_socket > 0)) {
      sprintf(temp_str, "%lu Total echo RTP pckts 1st stream",
              rtp_pckts);

      // AComment: Fix for random coredump when using RTP echo
      if (clock_tick-last_report_time) {
        fprintf(f,"  %-38s %lu.%03lu last period RTP rate (kB/s)" SIPP_ENDL,
                temp_str,
                (rtp_bytes)/(clock_tick-last_report_time),
                (rtp_bytes)%(clock_tick-last_report_time));
      }
      /* second stream statitics: */
      sprintf(temp_str, "%lu Total echo RTP pckts 2nd stream",
              rtp2_pckts);

      // AComment: Fix for random coredump when using RTP echo
      if (clock_tick-last_report_time) {
        fprintf(f,"  %-38s %lu.%03lu last period RTP rate (kB/s)" SIPP_ENDL,
                temp_str,
                (rtp2_bytes)/(clock_tick-last_report_time),
                (rtp2_bytes)%(clock_tick-last_report_time));
      }
      rtp_bytes = 0;
      rtp2_bytes = 0;
    }

    /* Scenario counters */
    fprintf(f,SIPP_ENDL);
    if(!lose_packets) {
      fprintf(f,"                                 "
              " Messages   Retrans   Timeout  Unexpected-Msg"
              SIPP_ENDL);
    } else {
      fprintf(f,"                                 "
              " Messages   Retrans   Timeout  Unexp.    Lost"
              SIPP_ENDL);
    }
  } // if diagram_only
  for(unsigned long index = 0;
      index < display_scenario->messages.size();
      index ++) {
    message *curmsg = display_scenario->messages[index];

    if(do_hide && curmsg->hide) {
      continue;
    }
    if (show_index) {
      fprintf(f, "%-3lu:", index);
    }

    if(SendingMessage *src = curmsg -> send_scheme) {
      char dialog_str[10] = "";
      if (curmsg->dialog_number != -1)
        sprintf(dialog_str, "(%-2d)", curmsg->dialog_number);
      if (src->isResponse()) {
        sprintf(temp_str, "%d%s", src->getCode(), dialog_str);
      } else {
        sprintf(temp_str, "%s%s", src->getMethod(), dialog_str);
      }

      if(creationMode == MODE_SERVER) {
        fprintf(f,"  <---------- %-14s ", temp_str);
      } else {
        fprintf(f,"  %14s ----------> ", temp_str);
      }
      if (!diagram_only) {
        if (curmsg -> start_rtd) {
          fprintf(f, " B-RTD%d ", curmsg -> start_rtd);
        } else if (curmsg -> stop_rtd) {
          fprintf(f, " E-RTD%d ", curmsg -> stop_rtd);
        } else {
          fprintf(f, "        ");
        }

        if(curmsg -> retrans_delay) {
          fprintf(f,"%-8lu %-8lu %-8lu %-8s" ,
                  curmsg -> nb_sent,
                  curmsg -> nb_sent_retrans,
                  curmsg -> nb_timeout,
                  "" /* Unexpected */);
        } else {
          fprintf(f,"%-8lu %-8lu %-8s %-8s" ,
                  curmsg -> nb_sent,
                  curmsg -> nb_sent_retrans,
                  "", /* Timeout. */
                  "" /* Unexpected. */);
        }
      } // if !diagram_only
    } else if(curmsg -> recv_response) {
      if (curmsg->dialog_number != -1)
        sprintf(temp_str, "%d(%-2d)", curmsg -> recv_response, curmsg->dialog_number);
      else
        sprintf(temp_str, "%d", curmsg -> recv_response);
      if(creationMode == MODE_SERVER) {
        if (curmsg->optional == OPTIONAL_TRUE)
          fprintf(f,"  -Optional-> %-14s ", temp_str);
        else
          fprintf(f,"  ----------> %-14s ", temp_str);
      } else {
        if (curmsg->optional == OPTIONAL_TRUE)
          fprintf(f,"  %14s <-Optional- ", temp_str);
        else
          fprintf(f,"  %14s <---------- ", temp_str);
      }

      if (!diagram_only) {
        if (curmsg -> start_rtd) {
          fprintf(f, " B-RTD%d ", curmsg -> start_rtd);
        } else if (curmsg -> stop_rtd) {
          fprintf(f, " E-RTD%d ", curmsg -> stop_rtd);
        } else {
          fprintf(f, "        ");
        }

        if(curmsg->retrans_delay) {
          fprintf(f,"%-8ld %-8ld %-8ld %-8ld" ,
                  curmsg->nb_recv,
                  curmsg->nb_recv_retrans,
                  curmsg->nb_timeout,
                  curmsg->nb_unexp);
        } else {
          fprintf(f,"%-8ld %-8ld %-8ld %-8ld" ,
                  curmsg -> nb_recv,
                  curmsg -> nb_recv_retrans,
                  curmsg -> nb_timeout,
                  curmsg -> nb_unexp);
        }
      } // !diagram_only
    } else if (curmsg -> pause_distribution ||
               (curmsg -> pause_variable != -1)) {
      char *desc = curmsg->pause_desc;
      if (!desc) {
        desc = (char *)malloc(24);
        if (curmsg->pause_distribution) {
          desc[0] = '\0';
          curmsg->pause_distribution->timeDescr(desc, 23);
        } else {
          snprintf(desc, 23, "$%s", display_scenario->allocVars->getName(curmsg->pause_variable));
        }
        desc[23] = '\0';
        curmsg->pause_desc = desc;
      }
      int len = strlen(desc) < 9 ? 9 : strlen(desc);

      if(creationMode == MODE_SERVER) {
        fprintf(f,"  [%8s] Pause%*s     ", desc, 23 - len > 0 ? 23 - len : 0, "");
      } else {
        fprintf(f,"          Pause   [%8s]%*s", desc, 18 - len > 0 ? 18 - len : 0, "");
      }

      if (!diagram_only) {
        fprintf(f,"%-8d", curmsg->sessions);
        fprintf(f,"                   %-8lu" , curmsg->nb_unexp);
      }
    } else if(curmsg -> recv_request) {
      if (curmsg->dialog_number != -1)
        sprintf(temp_str, "%s(%-2d)", curmsg -> recv_request, curmsg->dialog_number);
      else
        sprintf(temp_str, "%s", curmsg -> recv_request);
      if(creationMode == MODE_SERVER) {
        if (curmsg->optional == OPTIONAL_TRUE)
          fprintf(f,"  -Optional-> %-14s ", temp_str);
        else
          fprintf(f,"  ----------> %-14s ", temp_str);
      } else {
        if (curmsg->optional == OPTIONAL_TRUE)
          fprintf(f,"  %14s <-Optional- ", temp_str);
        else
          fprintf(f,"  %14s <---------- ", temp_str);
      }

      if (!diagram_only) {
        if (curmsg -> start_rtd) {
          fprintf(f, " B-RTD%d ", curmsg -> start_rtd);
        } else if (curmsg -> stop_rtd) {
          fprintf(f, " E-RTD%d ", curmsg -> stop_rtd);
        } else {
          fprintf(f, "        ");
        }

        fprintf(f,"%-8ld %-8ld %-8ld %-8ld" ,
                curmsg -> nb_recv,
                curmsg -> nb_recv_retrans,
                curmsg -> nb_timeout,
                curmsg -> nb_unexp);
      } // !diagram_only
    } else if(curmsg -> M_type == MSG_TYPE_NOP) {
      if (curmsg->display_str) {
        fprintf(f," %s", curmsg->display_str);
      } else if(creationMode == MODE_SERVER) {
        fprintf(f,"              [ NOP ]              ");
      } else {
        fprintf(f,"     [ NOP ]                       ");
      }
    } else if(curmsg -> M_type == MSG_TYPE_RECVCMD) {
      fprintf(f,"    [ Received Command ]         ");
      if (!diagram_only) {
        if(curmsg->retrans_delay) {
          fprintf(f,"%-8ld %-8s %-8ld %-8s" ,
                  curmsg->M_nbCmdRecv,
                  "",
                  curmsg->nb_timeout,
                  "");
        } else {
          fprintf(f,"%-8ld %-8s           %-8s" ,
                  curmsg -> M_nbCmdRecv,
                  "",
                  "");
        }
      } // !diagram_only
    } else if(curmsg -> M_type == MSG_TYPE_SENDCMD) {
      fprintf(f,"        [ Sent Command ]         ");
      if (!diagram_only)
        fprintf(f,"%-8lu %-8s           %-8s" ,
                curmsg -> M_nbCmdSent,
                "",
                "");
    } else {
      return SCENARIO_NOT_IMPLEMENTED;
      //REPORT_ERROR("Scenario command not implemented in display\n");
    }

    if(lose_packets && (curmsg -> nb_lost) && (!diagram_only)) {
      fprintf(f," %-8lu" SIPP_ENDL,
              curmsg -> nb_lost);
    } else {
      fprintf(f,SIPP_ENDL);
    }

    if(curmsg -> crlf) {
      fprintf(f,SIPP_ENDL);
    }
  }
  return 0;
}



/*
return 0 ok
return 

*/

int print_bottom_line(FILE *f, int last)
{
  if(last) {
    fprintf(f,"------------------------------ Test Terminated --------------------------------" SIPP_ENDL);
  } else if(quitting) {
    fprintf(f,"------- Waiting for active calls to end. Press [q] again to force exit. -------" SIPP_ENDL );
  } else if(paused) {
    fprintf(f,"----------------- Traffic Paused - Press [p] again to resume ------------------" SIPP_ENDL );
  } else if(cpu_max) {
    fprintf(f,"-------------------------------- CPU CONGESTED ---------------------------------" SIPP_ENDL);
  } else if(outbound_congestion) {
    fprintf(f,"------------------------------ OUTBOUND CONGESTION -----------------------------" SIPP_ENDL);
  } else {
    if (creationMode == MODE_CLIENT) {
      switch(thirdPartyMode) {
      case MODE_MASTER :
        fprintf(f,"-----------------------3PCC extended mode - Master side -------------------------" SIPP_ENDL);
        break;
      case MODE_3PCC_CONTROLLER_A :
        fprintf(f,"----------------------- 3PCC Mode - Controller A side -------------------------" SIPP_ENDL);
        break;
      case MODE_3PCC_NONE:
        fprintf(f,"------ [+|-|*|/]: Adjust rate ---- [q]: Soft exit ---- [p]: Pause traffic -----" SIPP_ENDL);
        break;
      default:
        return INTERNAL_ERROR;
        //REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
      }
    } else {
      assert(creationMode == MODE_SERVER);
      switch(thirdPartyMode) {
      case MODE_3PCC_A_PASSIVE :
        fprintf(f,"------------------ 3PCC Mode - Controller A side (passive) --------------------" SIPP_ENDL);
        break;
      case MODE_3PCC_CONTROLLER_B :
        fprintf(f,"----------------------- 3PCC Mode - Controller B side -------------------------" SIPP_ENDL);
        break;
      case MODE_MASTER_PASSIVE :
        fprintf(f,"------------------ 3PCC extended mode - Master side (passive) --------------------" SIPP_ENDL);
        break;
      case MODE_SLAVE :
        fprintf(f,"----------------------- 3PCC extended mode - Slave side -------------------------" SIPP_ENDL);
        break;
      case MODE_3PCC_NONE:
        fprintf(f,"------------------------------ Sipp Server Mode -------------------------------" SIPP_ENDL);
        break;
      default:
        return INTERNAL_ERROR;
        //REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
      }
    }
  }
  fprintf(f,SIPP_ENDL);
  fflush(stdout);
  return 0;
}


int print_statistics(int last)
{
  static int first = 1;

  if(backgroundMode == false && display_scenario) {
    if(!last) {
      //screen_clear();
#ifdef WIN32
      ClearScreen();
#else
      printf("\033[2J");
#endif
    }

    if(first) {
      first = 0;
      printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
             "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    }
    if (command_mode) {
      printf(SIPP_ENDL);
    }
    print_header_line(stdout,last);
    switch(currentScreenToDisplay) {
    case DISPLAY_STAT_SCREEN :
      display_scenario->stats->displayStat(stdout);
      break;
    case DISPLAY_REPARTITION_SCREEN :
      display_scenario->stats->displayRepartition(stdout);
      break;
    case DISPLAY_VARIABLE_SCREEN  :
      print_variable_list();
      break;
    case DISPLAY_TDM_MAP_SCREEN  :
      print_tdm_map();
      break;
    case DISPLAY_SECONDARY_REPARTITION_SCREEN :
      display_scenario->stats->displayRtdRepartition(stdout, currentRepartitionToDisplay);
      break;
    case DISPLAY_SCENARIO_SCREEN :
    default:
      print_stats_in_file(stdout, last);
      break;
    };
    if ( print_bottom_line(stdout,last) == INTERNAL_ERROR){
      return INTERNAL_ERROR;
      //REPORT_ERROR("Internal error: creationMode=%d, thirdPartyMode=%d", creationMode, thirdPartyMode);
    }

    if (!last && screen_last_error[0]) {
      char *errstart = jump_over_timestamp(screen_last_error);
      if (strlen(errstart) > 60) {
        printf("Last Message: %.60s..." SIPP_ENDL, errstart);
      } else {
        printf("Last Message: %s" SIPP_ENDL, errstart);
      }
      fflush(stdout);
    }
    if (command_mode) {
      printf("Command: %s", command_buffer ? command_buffer : "");
      fflush(stdout);
    }
    if(last) {
      fprintf(stdout,"\n");
    }
  }
  return 0;
}


int print_count_file(FILE *f, int header)
{
  char temp_str[256];

  if (!main_scenario || (!header && !main_scenario->stats)) {
    return 0;
  }

  if (header) {
    fprintf(f, "CurrentTime%sElapsedTime%s", stat_delimiter, stat_delimiter);
  } else {
    struct timeval currentTime, startTime;
    GET_TIME(&currentTime);
    main_scenario->stats->getStartTime(&startTime);
    unsigned long globalElapsedTime = CStat::computeDiffTimeInMs (&currentTime, &startTime);
    fprintf(f, "%s%s", CStat::formatTime(&currentTime), stat_delimiter);
    fprintf(f, "%s%s", CStat::msToHHMMSSmmm(globalElapsedTime), stat_delimiter);
  }

  for(unsigned int index = 0; index < main_scenario->messages.size(); index ++) {
    message *curmsg = main_scenario->messages[index];
    if(curmsg->hide) {
      continue;
    }

    if(SendingMessage *src = curmsg -> send_scheme) {
      if(header) {
        if (src->isResponse()) {
          sprintf(temp_str, "%d_%d_", index, src->getCode());
        } else {
          sprintf(temp_str, "%d_%s_", index, src->getMethod());
        }

        fprintf(f, "%sSent%s", temp_str, stat_delimiter);
        fprintf(f, "%sRetrans%s", temp_str, stat_delimiter);
        if(curmsg -> retrans_delay) {
          fprintf(f, "%sTimeout%s", temp_str, stat_delimiter);
        }
        if(lose_packets) {
          fprintf(f, "%sLost%s", temp_str, stat_delimiter);
        }
      } else {
        fprintf(f, "%lu%s", curmsg->nb_sent, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_sent_retrans, stat_delimiter);
        if(curmsg -> retrans_delay) {
          fprintf(f, "%lu%s", curmsg->nb_timeout, stat_delimiter);
        }
        if(lose_packets) {
          fprintf(f, "%lu%s", curmsg->nb_lost, stat_delimiter);
        }
      }
    } else if(curmsg -> recv_response) {
      if(header) {
        sprintf(temp_str, "%u_%d_", index, curmsg->recv_response);

        fprintf(f, "%sRecv%s", temp_str, stat_delimiter);
        fprintf(f, "%sRetrans%s", temp_str, stat_delimiter);
        fprintf(f, "%sTimeout%s", temp_str, stat_delimiter);
        fprintf(f, "%sUnexp%s", temp_str, stat_delimiter);
        if(lose_packets) {
          fprintf(f, "%sLost%s", temp_str, stat_delimiter);
        }
      } else {
        fprintf(f, "%lu%s", curmsg->nb_recv, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_recv_retrans, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_timeout, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_unexp, stat_delimiter);
        if(lose_packets) {
          fprintf(f, "%lu%s", curmsg->nb_lost, stat_delimiter);
        }
      }
    } else if(curmsg -> recv_request) {
      if(header) {
        sprintf(temp_str, "%u_%s_", index, curmsg->recv_request);

        fprintf(f, "%sRecv%s", temp_str, stat_delimiter);
        fprintf(f, "%sRetrans%s", temp_str, stat_delimiter);
        fprintf(f, "%sTimeout%s", temp_str, stat_delimiter);
        fprintf(f, "%sUnexp%s", temp_str, stat_delimiter);
        if(lose_packets) {
          fprintf(f, "%sLost%s", temp_str, stat_delimiter);
        }
      } else {
        fprintf(f, "%lu%s", curmsg->nb_recv, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_recv_retrans, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_timeout, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_unexp, stat_delimiter);
        if(lose_packets) {
          fprintf(f, "%lu%s", curmsg->nb_lost, stat_delimiter);
        }
      }
    } else if (curmsg -> pause_distribution ||
               curmsg -> pause_variable) {

      if(header) {
        sprintf(temp_str, "%d_Pause_", index);
        fprintf(f, "%sSessions%s", temp_str, stat_delimiter);
        fprintf(f, "%sUnexp%s", temp_str, stat_delimiter);
      } else {
        fprintf(f, "%d%s", curmsg->sessions, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_unexp, stat_delimiter);
      }
    } else if(curmsg -> M_type == MSG_TYPE_NOP) {
      /* No output. */
    }  else if(curmsg -> M_type == MSG_TYPE_RECVCMD) {
      if(header) {
        sprintf(temp_str, "%d_RecvCmd", index);
        fprintf(f, "%s%s", temp_str, stat_delimiter);
        fprintf(f, "%s_Timeout%s", temp_str, stat_delimiter);
      } else {
        fprintf(f, "%lu%s", curmsg->M_nbCmdRecv, stat_delimiter);
        fprintf(f, "%lu%s", curmsg->nb_timeout, stat_delimiter);
      }
    } else if(curmsg -> M_type == MSG_TYPE_SENDCMD) {
      if(header) {
        sprintf(temp_str, "%d_SendCmd", index);
        fprintf(f, "%s%s", temp_str, stat_delimiter);
      } else {
        fprintf(f, "%lu%s", curmsg->M_nbCmdSent, stat_delimiter);
      }
    } else {
      return UNKOWN_COUNT_FILE_MSGTYPE;
      //REPORT_ERROR("Unknown count file message type:");
    }
  }
  fprintf(f, "\n");
  fflush(f);
  return 0;
}
