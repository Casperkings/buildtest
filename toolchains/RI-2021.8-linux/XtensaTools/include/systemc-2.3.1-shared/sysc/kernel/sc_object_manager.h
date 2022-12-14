/*****************************************************************************

  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2014 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.accellera.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  sc_object_manager.h -- Manager of objects (naming, &c.)

  Original Author: Stan Y. Liao, Synopsys, Inc.

  CHANGE LOG AT THE END OF THE FILE
 *****************************************************************************/


#ifndef SC_OBJECT_MANAGER_H
#define SC_OBJECT_MANAGER_H

#include <map>
#include <vector>

namespace sc_core {

class sc_event;
class sc_object;
class sc_module_name;


#ifdef _MSC_VER
template class SYSTEMC_API sc_phash_iter<const char*, sc_object*>;
template class SYSTEMC_API sc_phash     <const char*, sc_object*>;
// TODO: Ric does something else need to be here now?
//template class SYSTEMC_API sc_pvector<sc_object*>;
template class SYSTEMC_API sc_plist<sc_object*>;
#endif

// ----------------------------------------------------------------------------
//  CLASS : sc_object_manager
//
//  Manager of objects.
// ----------------------------------------------------------------------------

class SYSTEMC_API sc_object_manager
{
    friend class sc_event;
    friend class sc_object;
    friend class sc_simcontext;

protected:
    struct table_entry
    {
        table_entry() : m_event_p(NULL), m_object_p(NULL) {}

	sc_event*  m_event_p;   // if non-null this is an sc_event.
        sc_object* m_object_p;  // if non-null this is an sc_object.
    };

public:
    typedef std::map<std::string,table_entry> instance_table_t;
    typedef std::vector<sc_object*>           object_vector_t;

    sc_object_manager();
    ~sc_object_manager();

    sc_event* find_event(const char* name);

    sc_object* find_object(const char* name);
    sc_object* first_object();
    sc_object* next_object();

    void hierarchy_push(sc_object* mdl);
    sc_object* hierarchy_pop();
    sc_object* hierarchy_curr();
    int hierarchy_size();

    void push_module_name(sc_module_name* mod_name);
    sc_module_name* pop_module_name();
    sc_module_name* top_of_module_name_stack();


private:
    std::string create_name( const char* leaf_name );
    void insert_event(const std::string& name, sc_event* obj);
    void insert_object(const std::string& name, sc_object* obj);
    void remove_event(const std::string& name);
    void remove_object(const std::string& name);

private:

    instance_table_t::iterator m_event_it;          // event instance iterator.
    bool                       m_event_walk_ok;     // true if can walk events.
    instance_table_t           m_instance_table;    // table of instances.
    sc_module_name*            m_module_name_stack; // sc_module_name stack.
    instance_table_t::iterator m_object_it;         // object instance iterator.
 SC_PRAGMA_WARNING(push)
 SC_PRAGMA_WARNING(disable:4251)
    object_vector_t            m_object_stack;      // sc_object stack.
 SC_PRAGMA_WARNING(pop)
    bool                       m_object_walk_ok;    // true if can walk objects.
};

} // namespace sc_core

// $Log: sc_object_manager.h,v $
// Revision 1.9  2011/08/26 20:46:10  acg
//  Andy Goodrich: moved the modification log to the end of the file to
//  eliminate source line number skew when check-ins are done.
//
// Revision 1.8  2011/03/06 15:55:11  acg
//  Andy Goodrich: Changes for named events.
//
// Revision 1.7  2011/03/05 19:44:20  acg
//  Andy Goodrich: changes for object and event naming and structures.
//
// Revision 1.6  2011/03/05 01:39:21  acg
//  Andy Goodrich: changes for named events.
//
// Revision 1.5  2011/02/18 20:27:14  acg
//  Andy Goodrich: Updated Copyrights.
//
// Revision 1.4  2011/02/13 21:47:37  acg
//  Andy Goodrich: update copyright notice.
//
// Revision 1.3  2010/07/22 20:02:33  acg
//  Andy Goodrich: bug fixes.
//
// Revision 1.2  2008/05/22 17:06:26  acg
//  Andy Goodrich: updated copyright notice to include 2008.
//
// Revision 1.1.1.1  2006/12/15 20:20:05  acg
// SystemC 2.3
//
// Revision 1.3  2006/01/13 18:44:30  acg
// Added $Log to record CVS changes into the source.

#endif
