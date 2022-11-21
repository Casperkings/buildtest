#ifndef _XTSC_TLM2_H_
#define _XTSC_TLM2_H_

// Copyright (c) 2006-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>
#include <tlm.h>

#ifndef xtsc_tlm2
#define xtsc_tlm2
#undef xtsc_tlm2
#endif

namespace xtsc_component {

/**
 * Convenience macro declaration for adding custom TLM2 ignorable extensions to the 
 * TLM2 Generic Payload. The macro takes two arguments - name of the extension and 
 * the type of class member, which represents the attribute added to the generic
 * payload by this extension.
 * Example:
 *   DECLARE_XTSC_TLM2_IGNORABLE_EXTENSION(ID_extension, unsigned int)
 *   It declares an extension class with the name 'ID_Extension', which inherits
 *   from tlm::tlm_extension<ID_extension> and has an 'unsigned int' attribute with
 *   the name 'value'.
 *
 * The rules to set,clear,release this custom extension remains same as defined by TLM2
 * for any user defined ignorable extension.
 *
 * Typical Use model for extensions : 
 *
 * If any custom TLM2 based in an XTSC environment models wants to be able to
 *      - use exclusive accesses from an xtensa core, it would be mostly be directly or indirectly 
 *        connected with xtsc_xttlm2tlm2_transactor. xtsc_xttlm2tlm2_transactor in this case would 
 *        set extensions - exclusive_request and transaction_id to the TLM2 generic payload for 
 *        exclusive transactions from the core. The custom slave model would have to check for these 
 *        extensions in the incoming TLM2 transaction to know about exclusive accesses. To return the 
 *        exclusive response (if the case be), the TLM2 salve model would then set another extension -
 *        exclusive_response. It would then be used by xtsc_xttlm2tlm2_transactor to get the response 
 *        status (EXOKAY or OKAY) from the slave. Note : The responsibility to release extension set 
 *        by the slave model would lie with xtsc_xttlm2tlm2_transactor.
 *
 *      - use an xtsc model like xtsc_memory(which has inbuilt exclusive monitor) as a slave model. The
 *        custom model would most likely be connected through xtsc_tlm22xttlm_transactor to the 
 *        xtsc_memory. Just like in the above case, as done by xtsc_xttlm2tlm2_transactor, here  
 *        extensions - exclusive_request and transaction_id would be set by the custom TLM2 initiator
 *        model. xtsc_tlm22xttlm_transactor, would interpret these extensions and convert them to the 
 *        attributes as understood by xtsc_memory for an exclusive access. xtsc_tlm22xttlm_transactor 
 *        would indicate the status of exclusive access by setting exclusive_response extension. 
 *        Note : It would be the responsibility of TLM2 initiator model to release the extension-
 *        response_extension.
 * 
 */

#define DECLARE_XTSC_TLM2_IGNORABLE_EXTENSION(name, type) \
class name: public tlm::tlm_extension<name> { \
  public: \
  name() : value(0) {} \
  virtual tlm_extension_base* clone() const { \
    name* t = new name; \
    t->value = this->value; \
    return t; \
  } \
  void copy_from(tlm_extension_base const &ext) { \
    value = static_cast<name const &>(ext).value; \
  } \
  type value; \
}

/**
 * Declare TLM2 extension to inform downstream slave about a request as exclusive
 */
DECLARE_XTSC_TLM2_IGNORABLE_EXTENSION(exclusive_request, bool);

/**
 * Add transaction id (similar to AxID in AXI) as an extended attribute to TLM2 payload
 */
DECLARE_XTSC_TLM2_IGNORABLE_EXTENSION(transaction_id, unsigned int);

/**
 * Declare TLM2 extension to inform this upstream model about the status of sent exclusive request
 * Note : this extension is generally set by the target and released by initiator after 
 * receiving the response.
 */
DECLARE_XTSC_TLM2_IGNORABLE_EXTENSION(exclusive_response, bool);


XTSC_COMP_API std::ostream& operator<<(std::ostream& os, const tlm::tlm_generic_payload& gp);

XTSC_COMP_API const char *xtsc_tlm_phase_text(tlm::tlm_phase& phase);

XTSC_COMP_API const char *xtsc_tlm_sync_enum_text(tlm::tlm_sync_enum& result);


}  // namespace xtsc_component


// Global namespace 

/**
 * Declare TLM2 phase to inform downstream model about the start of write data transfer for a TLM2 request.
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(BEGIN_WDATA);


/**
 * Declare TLM2 phase to inform upstream model about the end/acceptance of write data transfer.
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(END_WDATA);


/**
 * Declare TLM2 phase to inform downstream model about the start of last write data transfer for a TLM2 request.
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(BEGIN_WDATA_LAST);


/**
 * Declare TLM2 phase to inform upstream model about the end/acceptance of last write data transfer. 
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(END_WDATA_LAST);


/**
 * Declare TLM2 phase to inform upstream model about the start of non-last read response data transfer for a 
 * TLM2 request. Phase BEGIN_RESP is used for the last response transfer.
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(BEGIN_RDATA);


/**
 * Declare TLM2 phase to inform downstream model about the end/acceptance of read response data transfer for a TLM2 request.
 * Phase END_RESP is used for the last response transfer.
 * Note : This phase is enabled in xtsc_xttlm2tlm2_transactor and xtsc_tlm22xttlm_transactor only conditionally. 
 * The transactors in normal cases use only the default four phases defined by TLM2
 */
DECLARE_EXTENDED_PHASE(END_RDATA);




#endif  // _XTSC_TLM2_H_
