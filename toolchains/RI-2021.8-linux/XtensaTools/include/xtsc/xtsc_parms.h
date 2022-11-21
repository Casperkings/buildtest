#ifndef _XTSC_PARMS_h_
#define _XTSC_PARMS_h_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_types.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>


// Thwart Synopsys vcs cavalier #define of C++ reserved words in /cad/vcs/Y-2006.06-SP1-9/include/acc_user.h 
#undef true
#undef false


namespace xtsc {


/**
 * Base class for core and component module construction parameters.
 *
 * This class is the base class for all core and component module constructor parameters
 * sub-classes (xtsc_core_parms, xtsc_component::xtsc_memory_parms,
 * xtsc_component::xtsc_arbiter_parms, etc).
 *
 * This class maintains a map of name-value pairs for each of the following
 * parameter types:
 *  \verbatim
     bool
     double
     u32
     u64
     vector<u32>
     vector<u64>
     void*
     char*
     char**
     xtsc_parms
   \endverbatim
 * 
 * A name must be unique across all the name-value maps.
 *
 * When a parameter name-value pair is first added (using the appropriate add() method),
 * it can be flagged as being read-only so that subsequent calls to the corresponding
 * set() method will result in an exception being thrown.
 *
 * This class can be sub-classed as needed to provide default parameter values.
 * Typically, this would be done by calling the appropriate set methods from the
 * sub-class constructor.
 *
 * For example, to have a widget bit_width parameter with a default u32 value of 16,
 * one could create a xtsc_parms subclass like this:
 *
 *  \verbatim
   class widget_parms : public xtsc_parms {
   public:
     widget_parms() {
       add("bit_width", (u32) 16);
     }
   };
   \endverbatim
 *
 * Note:  When an xtsc_parms object is copied a deep copied is performed for all
 *        parameters except for void* parameters.  See copy_void_pointer().
 *
 * @see xtsc_core_parms
 * @see xtsc_component::xtsc_arbiter_parms
 * @see xtsc_component::xtsc_lookup_parms
 * @see xtsc_component::xtsc_lookup_driver_parms
 * @see xtsc_component::xtsc_master_parms
 * @see xtsc_component::xtsc_memory_parms
 * @see xtsc_component::xtsc_queue_parms
 * @see xtsc_component::xtsc_queue_consumer_parms
 * @see xtsc_component::xtsc_queue_producer_parms
 * @see xtsc_component::xtsc_router_parms
 * @see xtsc_component::xtsc_slave_parms
 * @see xtsc_component::xtsc_wire_parms
 */
class XTSC_API xtsc_parms {
public:

  enum xtsc_parameter_type {
    PARM_TYPE_BOOL,             ///<  parameter of type bool
    PARM_TYPE_DOUBLE,           ///<  parameter of type double
    PARM_TYPE_U32,              ///<  parameter of type u32
    PARM_TYPE_U64,              ///<  parameter of type u64
    PARM_TYPE_VOID_POINTER,     ///<  parameter of type void*
    PARM_TYPE_U32_VECTOR,       ///<  parameter of type vector<u32>
    PARM_TYPE_U64_VECTOR,       ///<  parameter of type vector<u64>
    PARM_TYPE_C_STR,            ///<  parameter of type char*
    PARM_TYPE_C_STR_ARRAY,      ///<  parameter of type char**
    PARM_TYPE_XTSC_PARMS        ///<  parameter of type xtsc_parms
  };


  /// Create an xtsc_parms object.
  xtsc_parms();


  /// Copy constructor.
  xtsc_parms(const xtsc_parms& parms);


  /// Copy assignment.
  xtsc_parms& operator=(const xtsc_parms& parms);


  // Delete any stuff we new'd.
  virtual ~xtsc_parms();


  /**
   * Return the kind (C++ type) of this xtsc_parms object.  Sub-classes should override
   * this method and return a c-string identifying their type (e.g. "xtsc_core_parms",
   * "xtsc_memory_parms", etc.).
   */
  virtual const char* kind() const { return "xtsc_parms"; }


  /**
   * Dump all dumpable parameters alphabetically.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @return the total number of parameters.
   */
  u32 dump(std::ostream& os = std::cout) const;


  /**
   * Dump all parameters grouped by type (that is, all bool parameters, then all u32
   * parameters, etc).
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     recurse         If false (the default), then only the name of each
   *                            hierarchical xtsc_parms objects is dumped (i.e their
   *                            contents are not dumped).  If true, the name of each
   *                            hierarchical xtsc_parms objects is dumped and then
   *                            xtsc_parms::dump_by_type() is recursively called on that
   *                            xtsc_parms object.  Using recurse=true is only
   *                            meaningful when hierarchical parameters exist (i.e. when
   *                            this xtsc_parms object contains other xtsc_parms objects
   *                            --see dump_xtsc_parms_map()).
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  A non-empty prefix is generally only
   *                            used when recurse is true.
   *
   * @return the total number of parameters.  If recurse is true, then this number
   * includes the number of parameters in each child xtsc_parms object recursively, but
   * does not count any child xtsc_parms object itself.  If recurse is false, then this
   * number includes 1 for each immediate child xtsc_parms object, but does not count
   * any parameters contained in those child xtsc_parms objects.
   */
  u32 dump_by_type(std::ostream& os = std::cout, bool recurse = false, const std::string& prefix = "") const;



  /**
   * Dump all parameters of the specified type.
   *
   * @param     parm_type       The parameter type to dump.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   */
  void dump_type(xtsc_parameter_type parm_type, std::ostream& os = std::cout) const;



  /**
   * Dump the value of the specified parameter.
   *
   * @param     name            The name of the parameter to dump.
   *
   * @param     os              The ostream object to which the parameter value is to be
   *                            dumped.  Default is cout.
   *
   */
  void dump_value(const char *name, std::ostream& os = std::cout) const;



  /**
   * Determine if a parameter with the specified name exists
   *
   * The is method returns true if a parameter with the specified name
   * exists, otherwise it returns false.
   */
  bool exists(const char *name) const;



  /**
   * Determine if the named parameter is dumpable.
   *
   * This method returns true if the named parameter is dumpable,
   * it returns false if the named parameter is not dumpable, and it
   * throws an exception if the named parameter does not exist.
   */
  bool dumpable(const char *name) const;



  /**
   * Determine if the named parameter is writable.
   *
   * The is method returns true if the named parameter is writable,
   * it returns false if the named parameter is read-only, and it
   * throws an exception if the named parameter does not exist.
   */
  bool writable(const char *name) const;



  /**
   * This method returns the xtsc_parameter_type of the parameter with the specified
   * name.  It throws an exception if the parameter does not exists.
   */
  xtsc_parameter_type get_parameter_type(const char *name) const;



  /// Return a c-string corresponding to the specified xtsc_parameter_type
  const char* get_parameter_type_name(xtsc_parameter_type parm_type) const;



  /**
   * Add a new named parameter of type bool.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter. Note: value should be of
   *                            type bool.  If it is of type int or u32, then the name-
   *                            value pair will be stored in the u32 map instead of the
   *                            bool map.  For literals, this means true/false should be
   *                            used instead of 1/0.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, bool value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type bool.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter. Note: value should be of
   *                            type bool.  If it is of type int or u32, then the name-
   *                            value pair will be stored in the u32 map instead of the
   *                            bool map.  For literals, this means true/false should be
   *                            used instead of 1/0.
   */
public:    void set(const char *name, bool value);

  /// Get the value of the named parameter of type bool.
  void get(const char *name, bool& value) const;

  /// Return the value of the named parameter of type bool.
  bool get_bool(const char *name) const;

  /// Get the name-to-bool-value map
  const std::map<std::string, bool>& get_bool_map() { return m_bool_map; }

  /**
   * Dump all dumpable bool parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of bool parameters.
   */
  u32 dump_bool_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type double.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter. Note: value should be of
   *                            type double.  For literals, this means a decimal point
   *                            should be present.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, double value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type double.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter. Note: value should be of
   *                            type double.  For literals, this means a decimal point
   *                            should be present.
   */
public:    void set(const char *name, double value);

  /// Get the value of the named parameter of type double.
  void get(const char *name, double& value) const;

  /// Return the value of the named parameter of type double.
  double get_double(const char *name) const;

  /// Get the name-to-double-value map
  const std::map<std::string, double>& get_double_map() { return m_double_map; }

  /**
   * Dump all dumpable double parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of double parameters.
   */
  u32 dump_double_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type int in the u32 map.  Note: int values are stored
   * in the u32 map as u32 values.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, int value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type int in the u32 map.
   * Note: int values are stored in the u32 map as u32 values.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, int value);

  /// Get the named value from the u32 map as type int.
  void get(const char *name, int& value) const;

  /// Get the named value from the u32 map as type int.
  int get_int(const char *name) const;



  /**
   * Add a new named parameter of type u32.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, u32 value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type u32 or u64.  If this method is called for a
   * named parameter of type u64, the high 32 bits of the named parameter will be set to
   * 0.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, u32 value);

  /**
   * Get the value of the named parameter of type u32 or u64.  If this method is 
   * called for a named parameter of type u64, the high 32 bits of the named parameter
   * will not be returned.
   */
  void get(const char *name, u32& value) const;

  /**
   * Return the value of the named parameter of type u32 or u64.  If this method is
   * called for a named parameter of type u64, the high 32 bits of the named parameter
   * will not be returned.
   */
  u32 get_u32(const char *name) const;

  /**
   * Return the value of the named parameter of type u32 or u64 if it is non-zero.  If
   * this method is called for a named parameter of type u64, the high 32 bits of the
   * named parameter will not be considered when checking for non-zero nor will they be
   * returned.
   *
   * This is a convenience method to get a u32 value and ensure it is not 0.  If it is
   * zero, an exception is thrown which gives the xtsc_parms sub-class and the parameter
   * name.
   *
   * @Throws xtsc_exception if the value is 0.
   */
  u32 get_non_zero_u32(const char *name) const;

  /// Get the name-to-u32-value map
  const std::map<std::string, u32>& get_u32_map() { return m_u32_map; }

  /**
   * Dump all dumpable u32 parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of u32 parameters.
   */
  u32 dump_u32_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type u64.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, u64 value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type u64.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, u64 value);

  /// Get the value of the named parameter of type u64.
  void get(const char *name, u64& value) const;

  /// Return the value of the named parameter of type u64.
  u64 get_u64(const char *name) const;

  /**
   * Return the value of the named parameter of type u64 if it is non-zero.
   *
   * This is a convenience method to get a u64 value and ensure it is not 0.  If it is
   * zero, an exception is thrown which gives the xtsc_parms sub-class and the parameter
   * name.
   *
   * @Throws xtsc_exception if the value is 0.
   */
  u64 get_non_zero_u64(const char *name) const;

  /// Get the name-to-u64-value map
  const std::map<std::string, u64>& get_u64_map() { return m_u64_map; }

  /**
   * Dump all dumpable u64 parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of u64 parameters.
   */
  u32 dump_u64_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type vector<u32>.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, const std::vector<u32>& value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type vector<u32> or vector<u64>.  If this method
   * is called for a named parameter of type vector<u64>, the high 32 bits of each entry
   * of the named parameter will be set to 0.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, const std::vector<u32>& value);

  /**
   * Get the value of the named parameter of type vector<u32> or vector<u64>.  If
   * this method is called for a named parameter of type vector<u64>, each entry in the
   * returned vector will be truncated to 32 bits.
   */
  void get(const char *name, std::vector<u32>& value) const;

  /**
   * Return the value of the named parameter of type vector<u32> or vector<u64>.  If
   * this method is called for a named parameter of type vector<u64>, each entry in the
   * returned vector will be truncated to 32 bits.
   */
  const std::vector<u32> get_u32_vector(const char *name) const;

  /// Get the name-to-vector<u32>-value map
  const std::map<std::string, std::vector<u32> >& get_u32_vector_map() { return m_u32_vector_map; }

  /**
   * Dump all dumpable vector<u32> parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of vector<u32> parameters.
   */
  u32 dump_u32_vector_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type vector<u64>.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, const std::vector<u64>& value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type vector<u64>.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, const std::vector<u64>& value);

  /// Get the value of the named parameter of type vector<u64>
  void get(const char *name, std::vector<u64>& value) const;

  /// Return the value of the named parameter of type vector<u64>
  const std::vector<u64> get_u64_vector(const char *name) const;

  /// Get the name-to-vector<u64>-value map
  const std::map<std::string, std::vector<u64> >& get_u64_vector_map() { return m_u64_vector_map; }

  /**
   * Dump all dumpable vector<u64> parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of vector<u64> parameters.
   */
  u32 dump_u64_vector_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type void*.  Caution:  By default, when an xtsc_parms
   * object is copied, the void* parameters are copied as opaque pointers.  The
   * (unknown) objects pointed to by the void* parameters are NOT copied.  For a means
   * to change the default behavior, see copy_void_pointer().  
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, const void *value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type void*.  Caution:  By default, when an
   * xtsc_parms object is copied, the void* parameters are copied as opaque pointers.
   * The (unknown) objects pointed to by the void* parameters are NOT copied.  For a
   * means to change the default behavior, see copy_void_pointer().
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, const void *value);

  /// Get the value of the named parameter of type void*.
  void get(const char *name, const void*& value) const;

  /// Return the value of the named parameter of type void*.
  const void *get_void_pointer(const char *name) const;

  /// Get the name-to-void*-value map
  const std::map<std::string, const void *>& get_void_pointer_map() { return m_void_pointer_map; }

  /**
   * Dump all dumpable void* parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of void* parameters.
   */
  u32 dump_void_pointer_map(std::ostream& os = std::cout, const std::string& prefix = "") const;


  /**
   * Add a new named parameter of type char* (c-string).
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, const char *value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type char* (c-string).
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, const char *value);

  /// Get the value of the named parameter of type char* (c-string).
  void get(const char *name, const char*& value) const;

  /// Return the value of the named parameter of type char* (c-string).
  const char *get_c_str(const char *name) const;

  /**
   * Return the value of the named parameter of type char* if it is neither NULL nor empty.
   *
   * This is a convenience method to get a u32 value and ensure it is neither NULL nor
   * empty.  If it is NULL or empty an exception is thrown which gives the xtsc_parms
   * sub-class and the parameter name.
   *
   * @Throws xtsc_exception if the value is NULL or empty.
   */
  const char *get_non_empty_c_str(const char *name) const;

  /// Get the name-to-char*-value map
  const std::map<std::string, const char *>& get_c_str_map() { return m_c_str_map; }

  /**
   * Dump all dumpable char* parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).  This is mainly used for hierarchical
   *                            parameters (i.e. when an xtsc_parms object contains
   *                            other xtsc_parms objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of char* parameters.
   */
  u32 dump_c_str_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type char** (NULL-terminated array of c-strings).
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   *
   * Note:  value can be NULL; however, if it is not NULL, then the last
   *        entry of the array (i.e. value[last]) MUST be NULL.
   */
protected: void add(const char *name, const char * const *value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type char** (NULL-terminated array of c-strings).
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   *
   * Note:  value can be NULL; however, if it is not NULL, then the last
   *        entry of the array (i.e. value[last]) MUST be NULL.
   */
public:    void set(const char *name, const char * const *value);

  /// Get the value of the named parameter of type char** (NULL-terminated array of c-strings).
  void get(const char *name, const char * const *& value) const;

  /// Return the value of the named parameter of type char** (NULL-terminated array of c-strings).
  const char * const *get_c_str_array(const char *name) const;

  /// Get the name-to-char**-value map
  const std::map<std::string, char **>& get_c_str_array_map() { return m_c_str_array_map; }

  /**
   * Dump all dumpabe char** parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty, then
   *                            a period is also prepended (between prefix and parameter
   *                            name).  This is mainly used for hierarchical parameters
   *                            (i.e. when an xtsc_parms object contains other xtsc_parms
   *                            objects--see dump_xtsc_parms_map()).
   *
   * @return the total number of char** parameters.
   */
  u32 dump_c_str_array_map(std::ostream& os = std::cout, const std::string& prefix = "") const;



  /**
   * Add a new named parameter of type xtsc_parms.
   *
   * @param     name            The name of the parameter.  The first character of the
   *                            name can be any alphabetic character plus underscore
   *                            (_).  Other characters can be any alphanumeric character
   *                            plus underscore (_).  
   *
   * @param     value           The value of the parameter.
   *
   * @param     read_only       If true, then the value cannot be changed (calls to the
   *                            set method will result in an exception being thrown).
   *                            If false (the default), then the value can be changed by
   *                            calls to the set method.
   *
   * @param     dumpable        If true (the default), then this parameter is dumpable.
   *                            If false, then the parameter will not be dumped.
   */
protected: void add(const char *name, const xtsc_parms& value, bool read_only=false, bool dumpable=true);

  /**
   * Set an existing named parameter of type xtsc_parms.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   */
public:    void set(const char *name, const xtsc_parms& value);

  /// Get the value of the named parameter of type xtsc_parms
  void get(const char *name, xtsc_parms& value) const;

  /// Return the value of the named parameter of type xtsc_parms
  xtsc_parms get_xtsc_parms(const char *name) const;

  /// Get the name-to-xtsc_parms-value map
  const std::map<std::string, xtsc_parms>& get_xtsc_parms_map() { return m_xtsc_parms_map; }

  /**
   * Dump all dumpable xtsc_parms parameters.
   *
   * @param     os              The ostream object to which the parameters are dumped.
   *                            Default is cout.
   *
   * @param     recurse         If false (the default), then only the name of each
   *                            hierarchical xtsc_parms objects is dumped (i.e their
   *                            contents are not dumped).  If true, the name of each
   *                            hierarchical xtsc_parms objects is dumped and then
   *                            xtsc_parms::dump() is recursively called on that
   *                            xtsc_parms object.
   *
   * @param     prefix          A string prefix to prepend to the parameter name.
   *                            Default is an empty string.  If prefix is not empty,
   *                            then a period is also prepended (between prefix and
   *                            parameter name).
   *
   * @return If recurse is true, then returns the number of parameters in each child
   * xtsc_parms object recursively, but does not count any child xtsc_parms object
   * itself.  If recurse is false, then returns the number of immediate child xtsc_parms
   * objects.
   */
  u32 dump_xtsc_parms_map(std::ostream& os = std::cout, bool recurse = false, const std::string& prefix = "") const;




  /// Set the width of the name column used by the dump methods
  void set_name_width(u32 name_width) { m_name_width = name_width; }

  /// Get the width of the name column used by the dump methods
  u32 get_name_width() { return m_name_width; }

  /**
   * Extract any applicable parameters from a c-string array.
   *
   * This optional convenience method is designed to make it easy to pass in module
   * configuration parameters on the command line.  This method will scan the argv array
   * looking for c-strings in one of the following 3 formats (leading hyphens are
   * ignored):
   *  \verbatim
       1. <ParmName>=<NewValue>
       2. <ParmType>::<ParmName>=<NewValue>
       3. <id>.<ParmName>=<NewValue>
      \endverbatim
   * For each match found, if <ParmName> matches an existing parameter name then
   * that parameters value will be changed to <NewValue> subject to the following
   * constraits on formats 2 and 3.
   *
   * For format 2, <ParmType> must match the value returned by this objects kind()
   * method.
   *
   * For format 3, <id> must match the id argument passed into this method.
   *
   * Each extracted parameter value is logged at INFO_LOG_LEVEL.
   *
   * @param     argc            The number of c-strings in argv.
   *
   * @param     argv            An array of c-strings.
   *
   * @param     id              A c-string that can be used in conjuction with
   *                            format 3 to narrow which argv c-strings apply.
   *                            One technique is to pass in name of the module
   *                            that these parameters will be used to configure.
   *
   * Note: The id check does not apply to argv entries in formats 1 or 2.
   *
   * Note: If <ParmName> is a parameter of type bool, then <NewValue> can be
   *       one of 0|1|t|f|true|false (case insensitive).
   *
   * Note: If <ParmName> is a parameter of type vector<u32> or char** (c-string array),
   *       then <NewValue> can be a comma-separated list of integers or strings,
   *       respectively.
   *
   * Note: If any problem is detected with an argv entry in format 1, that argv entry
   *       will be disregarded under the assumption that it doesn't apply to this
   *       object.
   *
   * As a technique, you may wish to put a call to this method in sc_main between the
   * xtsc_parms sub-class constructor call and the XTSC module constructor call for each
   * XTSC module that the user might wish to configure at run time.
   *
   * Examples:
   *  \verbatim
       In sc_main.cpp:
           xtsc_core_parms core_parms(CONFIG_NAME, XTENSA_REGISTRY, TDK_DIR);
           core_parms.extract_parms(argc, argv, "core0");
           xtsc_core core0("core0", core_parms);

           xtsc_memory_parms core0_pif_parms(core0, "pif");
           core0_pif_parms.extract_parms(argc, argv, "core0_pif");
           xtsc_memory core0_pif("core0_pif", core0_pif_parms);
       
       Running program prog from command line:
           prog --xtsc_core_parms::SimClients=summary --core0_pif.read_delay=8
           prog --core0.SimTargetArgs=1,two,"Three Amigos"
      \endverbatim
   *
   *
   * @return the number of parameters changed.
   */
  u32 extract_parms(int argc, const char * const argv[], const std::string& id = "");


protected:

  /**
   * This virtual method does a shallow copy of the void* pointer (the pointer is
   * copied, but the (unknown) object it points to is not).  Sub-classes can override
   * this virtual method to provide their own copy mechanism.
   *
   * @param     name            The name of the parameter.
   *
   * @param     value           The value of the parameter.
   *
   */
  virtual void copy_void_pointer(const std::string& name, const void *value);

  virtual void copy_their_stuff(const xtsc_parms& parms);

  virtual void delete_our_stuff();

  void no_value_found(xtsc_parameter_type parm_type, const char *name) const;

  /**
   * This method sets the dump attribute of a parameter.
   *
   * @param     name            The name of the parameter.
   *
   * @param     dumpable        The new value of dump attribute.
   *
   */
  void set_dumpable(const char *name, bool dumpable);

  /// Helper method for add() to validate the parameter name
  void validate_add_name(const char *name, xtsc_parameter_type parm_type) const;

  /// Helper method for set() to validate the parameter name
  void validate_set_name(const char *name, xtsc_parameter_type parm_type) const;


private:

  /// Helper method for dumping
  void dump_helper(std::ostream&        os,
                   const std::string&   prefix,
                   const std::string&   name) const;

#ifndef DOXYGEN_SKIP
XTSC_PRAGMA_WARNING(push)
XTSC_PRAGMA_WARNING(disable:4251)
#endif // DOXYGEN_SKIP

  std::map<std::string, xtsc_parameter_type>    m_type_map;                     ///<  name-to-type map

  std::map<std::string, bool>                   m_bool_map;                     ///<  name-to-value map for bool 
  std::map<std::string, double>                 m_double_map;                   ///<  name-to-value map for double 
  std::map<std::string, u32>                    m_u32_map;                      ///<  name-to-value map for u32 
  std::map<std::string, u64>                    m_u64_map;                      ///<  name-to-value map for u64 
  std::map<std::string, const void *>           m_void_pointer_map;             ///<  name-to-value map for void* 
  std::map<std::string, std::vector<u32> >      m_u32_vector_map;               ///<  name-to-value map for vector<u32> 
  std::map<std::string, std::vector<u64> >      m_u64_vector_map;               ///<  name-to-value map for vector<u64> 
  std::map<std::string, const char *>           m_c_str_map;                    ///<  name-to-value map for char* 
  std::map<std::string, char **>                m_c_str_array_map;              ///<  name-to-value map for char** 
  std::map<std::string, xtsc_parms>             m_xtsc_parms_map;               ///<  name-to-value map for xtsc_parms 

  std::set<std::string>                         m_read_only_set;                ///< read-only set
  std::set<std::string>                         m_not_dumpable_set;             ///< not-dumpable set

#ifndef DOXYGEN_SKIP
XTSC_PRAGMA_WARNING(pop)
#endif // DOXYGEN_SKIP

  u32                                           m_name_width;                   ///<  width of the name column for dumping

};



} // namespace xtsc

#endif  // _XTSC_PARMS_h_
