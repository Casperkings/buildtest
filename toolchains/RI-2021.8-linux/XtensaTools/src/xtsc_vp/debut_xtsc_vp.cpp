// Copyright (c) 2006-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

#include <xtsc_vp/xtsc_arbiter_vp.h>
#include <xtsc_vp/xtsc_clock_vp.h>
#include <xtsc_vp/xtsc_dma_engine_vp.h>
#include <xtsc_vp/xtsc_lookup_vp.h>
#include <xtsc_vp/xtsc_lookup_driver_vp.h>
#include <xtsc_vp/xtsc_master_vp.h>
#include <xtsc_vp/xtsc_memory_vp.h>
#include <xtsc_vp/xtsc_queue_vp.h>
#include <xtsc_vp/xtsc_queue_consumer_vp.h>
#include <xtsc_vp/xtsc_queue_producer_vp.h>
#include <xtsc_vp/xtsc_router_vp.h>
#include <xtsc_vp/xtsc_slave_vp.h>
#include <xtsc_vp/xtsc_wire_vp.h>
#include <xtsc_vp/xtsc_wire_source_vp.h>
#include <xtsc_vp/xtsc_lookup_if_vp_mon.h>
#include <xtsc_vp/xtsc_memory_if_vp_mon.h>
#include <xtsc_vp/xtsc_queue_pop_if_vp_mon.h>
#include <xtsc_vp/xtsc_queue_push_if_vp_mon.h>
#include <xtsc_vp/xtsc_wire_read_if_vp_mon.h>
#include <xtsc_vp/xtsc_wire_write_if_vp_mon.h>
#include <xtsc_vp/xtsc_pin2tlm_wire_transactor_vp.h>
#include <xtsc_vp/xtsc_tlm2pin_wire_transactor_vp.h>
#include <xtsc_vp/xtsc_queue_pin_vp.h>
#include <xtsc_vp/xtsc_lookup_pin_vp.h>
#include <xtsc_vp/xtsc_l2cc_vp.h>








void debut_xtsc_vp(void) {
  xtsc_vp::xtsc_arbiter_vp                      <1, 32>         debutante101("debutante101");
  xtsc_vp::xtsc_clock_vp                                        debutante102("debutante102");
  xtsc_vp::xtsc_dma_engine_vp                   <32>            debutante103("debutante103");
  xtsc_vp::xtsc_lookup_vp                       <8, 32>         debutante104("debutante104");
  xtsc_vp::xtsc_lookup_driver_vp                <8, 32>         debutante105("debutante105");
  xtsc_vp::xtsc_master_vp                       <32>            debutante106("debutante106");
  xtsc_vp::xtsc_memory_vp                       <1, 32>         debutante107("debutante107");
  xtsc_vp::xtsc_queue_vp                        <32>            debutante108("debutante108");
  xtsc_vp::xtsc_queue_consumer_vp               <32>            debutante109("debutante109");
  xtsc_vp::xtsc_queue_producer_vp               <32>            debutante110("debutante110");
  xtsc_vp::xtsc_router_vp                       <1, 32>         debutante111("debutante111");
  xtsc_vp::xtsc_slave_vp                        <32>            debutante112("debutante112");
  xtsc_vp::xtsc_wire_vp                         <1>             debutante114("debutante114");
  xtsc_vp::xtsc_wire_source_vp                  <1>             debutante115("debutante115");
  xtsc_vp::xtsc_lookup_if_vp_mon                <8, 32>         debutante116("debutante116");
  xtsc_vp::xtsc_memory_if_vp_mon                <32>            debutante117("debutante117");
  xtsc_vp::xtsc_queue_pop_if_vp_mon             <32>            debutante118("debutante118");
  xtsc_vp::xtsc_queue_push_if_vp_mon            <32>            debutante119("debutante119");
  xtsc_vp::xtsc_wire_read_if_vp_mon             <1>             debutante121("debutante121");
  xtsc_vp::xtsc_wire_write_if_vp_mon            <1>             debutante122("debutante122");
  xtsc_vp::xtsc_pin2tlm_wire_transactor_vp      <1, bool>       debutante123("debutante123");
  xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_RESET                debutante124("debutante124");
  xtsc_vp::xtsc_pin2tlm_wire_transactor_vp_CLOCK                debutante125("debutante125");
  xtsc_vp::xtsc_tlm2pin_wire_transactor_vp      <1, bool>       debutante126("debutante126");
  xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_RESET                debutante127("debutante127");
  xtsc_vp::xtsc_tlm2pin_wire_transactor_vp_CLOCK                debutante128("debutante128");
  xtsc_vp::xtsc_queue_pin_vp                    <32>            debutante129("debutante129");
  xtsc_vp::xtsc_lookup_pin_vp                   <8, 32>         debutante130("debutante130");
  xtsc_vp::xtsc_l2cc_vp                                         debutante131("debutante131");
}

