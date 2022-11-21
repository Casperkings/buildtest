// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.


#include <cstdlib>
#include <ostream>
#include <string>
#include <xtsc/xtsc_tlm2.h>

using namespace std;
using namespace tlm;
using namespace xtsc;


#define xtsc_tlm2


std::ostream& xtsc_component::operator<<(std::ostream& os, const tlm_generic_payload& gp) {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();

  tlm_command cmd = gp.get_command();

  if (cmd == TLM_IGNORE_COMMAND) {
    os << "<Ignore> ";
  }
  else {
    tlm_response_status  status       = gp.get_response_status();
    u64                  adr          = gp.get_address();
    u8                  *ptr          = gp.get_data_ptr();
    u32                  len          = gp.get_data_length();
    u32                  sw           = gp.get_streaming_width();
    u8                  *bep          = gp.get_byte_enable_ptr();
    u32                  bel          = gp.get_byte_enable_length();
    bool                 dmi          = gp.is_dmi_allowed();
    bool                 read         = (cmd == TLM_READ_COMMAND);
    bool                 inc          = (status == TLM_INCOMPLETE_RESPONSE);
    bool                 ok           = (status == TLM_OK_RESPONSE);
    bool                 write        = !read;
    u8                   enb          = TLM_BYTE_ENABLED;
    os << (read ? "READ " : "WRITE") << " 0x" << hex << setfill('0') << setw(8) << adr << " " << dec << setfill(' ') << setw(2)
       << len << ":" << setw(2) << sw << ":" << (dmi ? "D" : " ") << ":";
    switch (status) {
      case TLM_OK_RESPONSE:                os << "TLM_OKAY"; break;
      case TLM_INCOMPLETE_RESPONSE:        os << "TLM_INC "; break;
      case TLM_GENERIC_ERROR_RESPONSE:     os << "TLM_GERR"; break;
      case TLM_ADDRESS_ERROR_RESPONSE:     os << "TLM_AERR"; break;
      case TLM_COMMAND_ERROR_RESPONSE:     os << "TLM_CERR"; break;
      case TLM_BURST_ERROR_RESPONSE:       os << "TLM_BERR"; break;
      case TLM_BYTE_ENABLE_ERROR_RESPONSE: os << "TLM_EERR"; break;
      default:                             os << "TLM_UNKN"; break;
    }
    if (ptr && ((read && ok) || (write && inc))) {
      if (!bep) {
        bep = &enb;
        bel = 1;
      }
      os << hex << setfill('0') << "=";
      for (u32 i=0; i < len; ++i) {
        if (bep[i % bel] == enb) {
          os << " " << setw(2) << (u32)ptr[i];
        }
        else {
          os << " ..";
        }
      }
    }
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);

  return os;
}



const char *xtsc_component::xtsc_tlm_phase_text(tlm_phase& phase) {
  if (phase == BEGIN_REQ        ) return "BEG_REQ";
  if (phase == BEGIN_RESP       ) return "BEG_RSP";
  if (phase == END_REQ          ) return "END_REQ";
  if (phase == END_RESP         ) return "END_RSP";
  if (phase == BEGIN_RDATA      ) return "BEG_RDT";
  if (phase == END_RDATA        ) return "END_RDT";
  if (phase == BEGIN_WDATA      ) return "BEG_WDT";
  if (phase == END_WDATA        ) return "END_WDT";
  if (phase == BEGIN_WDATA_LAST ) return "BEG_WDL";
  if (phase == END_WDATA_LAST   ) return "END_WDL";
  ostringstream oss;
  oss << phase;
  static string str = oss.str();
  return str.c_str();
}



const char *xtsc_component::xtsc_tlm_sync_enum_text(tlm_sync_enum& result) {
  return ((result == TLM_ACCEPTED) ? "TLM_ACC" : (result == TLM_UPDATED) ? "TLM_UPD" : (result == TLM_COMPLETED) ? "TLM_CMP" : "TLM_UNK");
}



