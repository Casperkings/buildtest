#ifndef _XTSC_COMP_H_
#define _XTSC_COMP_H_

// Copyright (c) 2014 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file xtsc_comp.h
 *
 */


#include <xtsc/xtsc.h>


/**
 * This method can be used to get the xtsc_plugin_interface of the "builtin" models in
 * the XTSC core and component libraries (libxtsc and libxtsc_comp).
 *
 * A builtin model is a model that exists in one of the XTSC libraries and that also is
 * supported by an instance of the xtsc_plugin_interface that can be accessed by calling
 * this method.  This is done to avoid having to add specific support for the model to
 * xtsc-run, instead relying on the plugin support in xtsc-run.  You can use xtsc-run to
 * get a list of builtin models in XTSC like this:
 * \verbatim
        xtsc-run --show_plugins
        xtsc-run --show_plugins=verbose
   \endverbatim
 *
 * A builtin model can be configured, created, and connected in sc_main using either the
 * plugin methodology or using the traditional methodology of #include'ing the model's 
 * header file.
 *
 */
extern "C" XTSC_COMP_API xtsc::xtsc_plugin_interface& xtsc_get_builtin_plugin_interface();



#endif  // _XTSC_COMP_H_

