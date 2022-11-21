#ifndef _XTSC_IF_VP_STUBS_H_
#define _XTSC_IF_VP_STUBS_H_

// Copyright (c) 2019 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_queue_pop_if.h>
#include <xtsc/xtsc_queue_push_if.h>
#include <xtsc/xtsc_wire_read_if.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <xtsc/xtsc_lookup_if.h>

namespace xtsc {

    class xtsc_request_if_vp_stub : virtual public xtsc::xtsc_request_if, public sc_core::sc_export<xtsc::xtsc_request_if> {
        public:
            xtsc_request_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_request_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_request_if>*)this)->bind(*(xtsc::xtsc_request_if*)this);              
            } 
        private:    	
            virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer)  {}
            virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer){}
            virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request){ return true;}
            virtual void nb_request(const xtsc::xtsc_request& request){}
    };

    class xtsc_respond_if_vp_stub : virtual public xtsc::xtsc_respond_if, public sc_core::sc_export<xtsc::xtsc_respond_if> {
        public:
            xtsc_respond_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_respond_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_respond_if>*)this)->bind(*(xtsc::xtsc_respond_if*)this);
            }
        private:
            virtual bool nb_respond(const xtsc::xtsc_response& response){ return true;}
    };

    class xtsc_queue_pop_if_vp_stub : virtual public xtsc::xtsc_queue_pop_if, public sc_core::sc_export<xtsc::xtsc_queue_pop_if> {
        public:
            xtsc_queue_pop_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_queue_pop_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_queue_pop_if>*)this)->bind(*(xtsc::xtsc_queue_pop_if*)this);
            }
        private:
            virtual bool nb_can_pop() { return true; }
            virtual bool nb_pop(sc_dt::sc_unsigned& element, u64& ticket = pop_ticket) { return true; }
            virtual u32 nb_get_bit_width() { return 0; }
    };

    class xtsc_queue_push_if_vp_stub : virtual public xtsc::xtsc_queue_push_if, public sc_core::sc_export<xtsc::xtsc_queue_push_if> {
        public:
            xtsc_queue_push_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_queue_push_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_queue_push_if>*)this)->bind(*(xtsc::xtsc_queue_push_if*)this);
            }
        private:
            virtual bool nb_can_push() { return true; }
            virtual bool nb_push(const sc_dt::sc_unsigned& element, u64& ticket = push_ticket) { return true;}
            virtual u32 nb_get_bit_width() { return 0; }
    };

    class xtsc_wire_read_if_vp_stub : virtual public xtsc::xtsc_wire_read_if, public sc_core::sc_export<xtsc::xtsc_wire_read_if> {
        public:
            xtsc_wire_read_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_wire_read_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_wire_read_if>*)this)->bind(*(xtsc::xtsc_wire_read_if*)this);
            }
        private:
            virtual sc_dt::sc_unsigned nb_read() {
                sc_dt::sc_unsigned value;
                value=0;
                return value;
            }
            virtual u32 nb_get_bit_width() { return 0;}
    };

    class xtsc_wire_write_if_vp_stub : virtual public xtsc::xtsc_wire_write_if, public sc_core::sc_export<xtsc::xtsc_wire_write_if> {
        public:
            xtsc_wire_write_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_wire_write_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_wire_write_if>*)this)->bind(*(xtsc::xtsc_wire_write_if*)this);
            }
        private:
            virtual void nb_write(const sc_dt::sc_unsigned& value) { }
            virtual u32 nb_get_bit_width() {return 0;}
    };

    class xtsc_lookup_if_vp_stub : virtual public xtsc::xtsc_lookup_if, public sc_core::sc_export<xtsc::xtsc_lookup_if> {
        public:
            xtsc_lookup_if_vp_stub(const char* module_name):sc_core::sc_export<xtsc::xtsc_lookup_if>(module_name){ 
                ((sc_core::sc_export<xtsc::xtsc_lookup_if>*)this)->bind(*(xtsc::xtsc_lookup_if*)this);
            }
        private:	
            virtual void nb_send_address(const sc_dt::sc_unsigned& address) {}
            virtual sc_dt::sc_unsigned nb_get_data() {
                sc_dt::sc_unsigned value;
                value=0;
                return value;
            }
            virtual u32 nb_get_address_bit_width() {return 0;}
            virtual u32 nb_get_data_bit_width() {return 0;}
    };

} // namespace xtsc

#endif
