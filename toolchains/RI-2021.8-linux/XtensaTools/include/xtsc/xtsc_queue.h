#ifndef _XTSC_QUEUE_H_
#define _XTSC_QUEUE_H_

// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_queue_push_if.h>
#include <xtsc/xtsc_queue_pop_if.h>
#include <vector>



namespace xtsc {
class xtsc_core;
}



namespace xtsc_component {

class xtsc_queue_producer;
class xtsc_queue_consumer;


/**
 * Constructor parameters for an xtsc_queue object.
 *
 * This class contains the constructor parameters for an xtsc_queue object.
 * \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------
  
   "bit_width"          u32     Width of each queue element in bits.

   "depth"              u32     Number of elements the queue can hold.

   "num_producers"      u32     Number of producers which can write to the queue.  If
                                "num_producers" is greater than 1 then a dedicated skid
                                buffer will be added to the queue for each producer.
                                Default = 1.
  
   "num_consumers"      u32     Number of consumers which can read from the queue.  If
                                "num_consumers" is greater than 1 then a dedicated skid
                                buffer will be added to the queue for each consumer.  
                                Default = 1.
  
   "push_file"          char*   Name of file to write nb_push elements to instead
                                of adding them to the queue fifo.  If the
                                "push_file" parameter is non-null and non-empty,
                                then calls to nb_push will cause the passed 
                                element to be written to the file and to NOT be
                                added to the queue fifo.  If the file named by the
                                "push_file" parameter value does not exist, it
                                will be created.  If it does exist, it will be 
                                overwritten.  If both "push_file" and "pop_file" 
                                parameters are null or empty, then calls to nb_push
                                will cause the passed element to be added to the
                                queue fifo.

   "timestamp"          bool    If true, then each value written to "push_file"
                                will include the SystemC timestamp as an
                                xtsc_script_file comment.  This parameter is ignored
                                unless "push_file" is non-null and non-empty.
                                Default = true.

   "pop_file"           char*   Name of file to read nb_pop elements from instead
                                of getting them from the queue.  If the "pop_file"
                                parameter is non-null and non-empty, then calls
                                to nb_pop will get their element from the file
                                instead of from the queue fifo.  The file named
                                by the "pop_file" parameter must exist.  If both
                                "pop_file" and "push_file" parameters are null or
                                empty, then calls to nb_pop will get their element
                                from the queue fifo.  Element values in the file can
                                be expressed in decimal or hexadecimal (using leading
                                '0x') format.  Element values must be separated by 
                                white-space and any C-style comments are ignored.
                                See xtsc::xtsc_script_file.

   "wraparound"         bool    Specifies what should happen when the end of file
                                (EOF) is reached on "pop_file".  When EOF is reached
                                and "wraparound" is true, "pop_file" will be reset
                                to the beginning of file and nb_pop will return
                                the first element from the file.  When EOF is
                                reached and "wraparound" is false, nb_can_pop and
                                nb_pop will return false.
                                Default = false.

   "host_shared_memory" bool    If true the storage for the queue tickets, data, and
                                read and write indices will be created at module
                                construction time as host OS shared memory using
                                shm_open() on Linux and CreateFileMapping() on
                                MS Windows.  If this parameter is set true, then
                                neither "pop_file" nor "push_file" may be used and
                                neither "num_consumers" nor "num_producers" may
                                exceed 1.
                                Default = false.

   "shared_memory_name"   char* The name of the host OS shared memory.  If this
                                parameter is left at its default setting of NULL, then
                                the shared memory name will be formed by concatenating
                                the user name, a period, and the module instance
                                hierarchical name.  This parameter is only used if
                                "host_shared_memory" is true.
                                Default = NULL (use default shared memory name)

   Note:  To cause xtsc_queue to function as a normal queue, set both "push_file"
          and "pop_file" parameter values to null (the default) or empty and 
          bind to both the xtsc_queue::m_producer and xtsc_queue::m_consumer ports.

          To cause xtsc_queue to function as an infinite sink of elements pushed 
          into it, specify a valid file name for "push_file" and bind to the 
          xtsc_queue::m_producer port.

          To cause xtsc_queue to function as a source of elements popped from
          it, specify a valid and existing file name for "pop_file" and bind to
          the xtsc_queue::m_consumer port.

          To cause xtsc_queue to function as both a sink and a source, specify
          both file names and bind to both ports.
          
          To cause xtsc_queue to function as a sink but not as a source, specify
          "pop_file" as null or empty and do NOT bind to the xtsc_queue::m_consumer
          port.
          
          To cause xtsc_queue to function as a source but not as a sink, specify
          "push_file" as null or empty and do NOT bind to the xtsc_queue::m_producer
          port.

          To use host OS shared memory for the queue, set "host_shared_memory" to true,
          do not set "pop_file" or "push_file", set "num_consumers" and "num_producers"
          to either 0 or 1, and ensure at most one process on the workstation writes to
          the queue and at most one process reads from the queue.

   \endverbatim
 *
 *
 * @see xtsc_queue
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_queue_parms : public xtsc::xtsc_parms {
public:


  /**
   * Constructor for an xtsc_queue_parms object.
   *
   * @param     width1          Width of each queue element in bits.
   *
   * @param     depth           Number of elements the queue can hold.
   *
   * @param     push_file       Name of file to write nb_push elements to instead
   *                            of adding them to the queue fifo.
   *
   * @param     pop_file        Name of file to read nb_pop elements from instead
   *                            of getting them from the queue fifo.
   *
   * @param     wraparound      Indicates if pop_file should wraparound to the
   *                            beginning of the file after the end of file is
   *                            reached.
   */
  xtsc_queue_parms(xtsc::u32    width1          = 32,
                   xtsc::u32    depth           = 16,
                   const char  *push_file       = 0,
                   const char  *pop_file        = 0,
                   bool         wraparound      = false)
  {
    init(width1, depth, push_file, pop_file, wraparound);
  }


  /**
   * Constructor for an xtsc_queue_parms object based upon an xtsc_core object and a
   * named TIE input or output queue.
   *
   * This constructor will determine width1 by querying the core object and then pass it
   * to the init() method.  If desired, after the xtsc_queue_parms object is
   * constructed, its data members can be changed using the appropriate
   * xtsc::xtsc_parms::set() method before passing it to the xtsc_queue constructor.
   *
   * @param     core            A reference to the xtsc_core object upon which
   *                            to base the xtsc_queue_parms.
   *
   * @param     queue_name      The name of the TIE queue as it appears in the
   *                            user's TIE code after the queue keyword.
   *
   * @param     depth           Number of elements the queue can hold.
   *
   * @param     push_file       Name of file to write nb_push elements to instead
   *                            of adding them to the queue fifo.
   *
   * @param     pop_file        Name of file to read nb_pop elements from instead
   *                            of getting them from the queue fifo.
   *
   * @param     wraparound      Indicates if pop_file should wraparound to the
   *                            beginning of the file after the end of file is
   *                            reached.
   */
  xtsc_queue_parms(const xtsc::xtsc_core&       core,
                   const char                  *queue_name,
                   xtsc::u32                    depth           = 16,
                   const char                  *push_file       = 0,
                   const char                  *pop_file        = 0,
                   bool                         wraparound      = false);


  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32    width1          = 32,
            xtsc::u32    depth           = 16,
            const char  *push_file       = 0,
            const char  *pop_file        = 0,
            bool         wraparound      = false)
  {
    add("bit_width",            width1);
    add("depth",                depth);
    add("num_producers",        1);
    add("num_consumers",        1);
    add("push_file",            push_file);
    add("timestamp",            true);
    add("pop_file",             pop_file);
    add("wraparound",           wraparound);
    add("host_shared_memory",   false);
    add("shared_memory_name",   (char*)NULL);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_queue_parms"; }
};








/**
 * A queue implementation that connects using TLM-level ports.
 *
 * This queue implements xtsc::xtsc_queue_push_if and is suitable for connecting to an
 * LX or NX TIE output queue interface.  It also implements xtsc::xtsc_queue_pop_if and
 * is suitable for connecting to an LX or NX TIE input queue interface. 
 *
 * Typically, the queue is connected on the push side to a queue producer such as a TIE
 * output queue interface of xtsc::xtsc_core and on the pop side to a queue consumer such
 * as a TIE input queue interface of xtsc::xtsc_core.  Alternatively, it can be left
 * unconnected on the pop side and then all elements pushed into it will be written to
 * a file.  It is also possible to leave it unconnected on the push side and then any
 * element popped from the queue will be read from a file.
 *
 * This queue can operate as a multi-client queue by setting the "num_producers"
 * parameter to a value larger then one and/or setting the "num_consumers" parameter to
 * a value larger then one.  When "num_producers" is larger then one then an additional
 * skid buffer is added to the queue for each producer (when "num_producers" is left at
 * its default value of 1 then no additional skid buffer is used).  If all the producer
 * skid buffers are empty then all producers can push an element into the queue in the
 * same cycle.  Similarly, when "num_consumers" is larger then one then an additional
 * jerk buffer is added to the queue for each consumer (again, this does not happen when
 * "num_consumers" is 1).  If all the consumer jerk buffers are full then all consumers
 * can pop an element from the queue in the same cycle.  
 *
 * The following diagram shows the situation when "num_producers" is 2, "num_consumers"
 * is 3, and "depth" is 4.  In this case the queue can potentially hold 9 (=2+3+4)
 * elements.  In the diagram, a percent sign represents a producer or consumer port,
 * 'X' represents a producer skid buffer, 'Y' represents a buffer in the main fifo, and
 * 'Z' represents a consumer jerk buffer.
 * \verbatim
          Producer A   % => X          Z  => %   Consumer 1
                              \      /
                                YYYY --Z  => %   Consumer 2
                              /      \
          Producer B   % => X          Z  => %   Consumer 3
   \endverbatim
 *
 * Note: In a multiple-producer/single-consumer scenario, elements pushed into the queue
 *       by different producers always come out ordered by the time when they went in
 *       (elements pushed into the queue in the same delta cycle have no concept of
 *       relative order amongst themselves).
 *
 * Note: There is no concept of "first-come-first-serve" when multiple consumers are
 *       popping from the queue.  When an element is added to the queue it lands in
 *       the first empty consumer jerk buffer in fair round-robin order without
 *       regard for any previous unsuccessful pop attempts at some other consumer port.
 *
 * Caution: The only way to get an element out of one of the consumer jerk buffers is
 *          for the corresponding consumer to pop an element from the queue.  If the
 *          user does not want any elements to be stuck in the queue, then they must
 *          ensure that all consumers periodically pop from the queue.
 *
 * By default this queue uses internal data members to store the queue elements.  If
 * desired, the queue can be configured to instead use host OS shared memory to store
 * the elements and read/write row indices by setting "host_shared_memory" to true.  
 * This can be used to speed up a multi-core simulation by partitioning it into multiple
 * host processes that run on separate processor cores of the host workstation.  It can
 * also be used to allow a separate (potentially non-XTSC) host process to feed and/or
 * drain the queue or just passively monitor it.  For example on Linux:
 * \verbatim
      hexdump -C /dev/shm/username.Q1
   \endverbatim
 * The layout and access protocol of the shared memory data structure are:
 * \verbatim
                     Shared Memory Layout and Access Protocol

                   -----------------------------------------------
         Row 0:    |  Ticket 0   | Data 0             | Filler 0 |
                   -----------------------------------------------
         Row 1:    |  Ticket 1   | Data 1             | Filler 1 |
                   -----------------------------------------------
         Row 2:    |  Ticket 2   | Data 2             | Filler 2 |
                   -----------------------------------------------
          ...      |    ...      |  ...               |   ...    |
                   -----------------------------------------------
         Row N:    |  Ticket N   | Data N             | Filler N |
                   -----------------------------------------------
         Indices:  | ridx | widx |
                   ---------------

   - Memory byte addresses increase left to right then top to bottom in the diagram.
   - All values are stored LSB first (left most byte in each field shown above).
   - In the following points, R represents any row index between 0 and N, inclusive.
   - N is equal to the "depth" parameter, so the number of "Row R" rows is "depth" + 1.
   - There is always at least one "unoccupied" row, even when the queue is full.
   - The size of the "ridx"     field is 4 bytes (u32).
   - The size of the "widx"     field is 4 bytes (u32).
   - The size of the "Ticket R" field is 8 bytes (u64).
   - The size of the "Data R"   field is ("bit_width" + 7) / 8
   - The size of the "Filler R" field is the smallest value (between 0 and 7) which
     makes the row byte size be a multiple of 8.
   - The ridx (Read row InDeX) and widx (Write row InDeX) fields each always contain a
     value between 0 and N, inclusive.
   - ridx is the index of the next row to be read.
   - widx is the index of the next row to be written.
   - The queue is empty when widx and ridx are the same.  That is, when:
         widx == ridx
   - The queue is full when widx is one "behind" ridx.  That is, when:
         (widx + 1) % (N + 1) == ridx
   - The shared memory contents should be zero-filled (especially ridx and widx) by the
     host OS process creating the shared memory.  Other than zero-filling at creation:
     - At most one host OS process (the producer process) may write to the "Row R" rows
       and the widx field.
     - At most one host OS process (the consumer process) may write to the ridx field.
   - If and only if the queue is not full, the producer process may write to the row
     whose index is widx and then update widx:  widx = (widx + 1) % (N + 1)
   - If and only if the queue is not empty, the consumer process may read the row
     whose index is ridx and then update ridx:  ridx = (ridx + 1) % (N + 1)
   \endverbatim
 *
 * Here is a block diagram of the system used in the xtsc_queue example (memories not
 * shown):
 * @image html  Example_xtsc_queue.jpg
 * @image latex Example_xtsc_queue.eps "xtsc_queue Example" width=10cm
 *
 * Here is the code to connect the system using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(core0, "pif", "", core0_pif);
    xtsc_connect(core1, "pif", "", core1_pif);
    xtsc_connect(core0, "OUTQ1", "queue_push", Q1);
    xtsc_connect(core1, "INQ1",  "queue_pop",  Q1);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    core0.get_request_port("pif")(*core0_pif.m_request_exports[0]);
    (*core0_pif.m_respond_ports[0])(core0.get_respond_export("pif"));
    core1.get_request_port("pif")(*core1_pif.m_request_exports[0]);
    (*core1_pif.m_respond_ports[0])(core1.get_respond_export("pif"));
    core0.get_output_queue("OUTQ1")(Q1.m_producer);
    core1.get_input_queue("INQ1")(Q1.m_consumer);
   \endverbatim
 *
 * @see xtsc_queue_parms
 * @see xtsc::xtsc_queue_pop_if
 * @see xtsc::xtsc_queue_push_if
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_COMP_API xtsc_queue : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<xtsc::xtsc_queue_push_if>  m_producer;     ///<  Single producer binds to this
  sc_core::sc_export<xtsc::xtsc_queue_pop_if>   m_consumer;     ///<  Single consumer binds to this

  sc_core::sc_export<xtsc::xtsc_queue_push_if>**m_producers;    ///<  Multiple producers bind to these
  sc_core::sc_export<xtsc::xtsc_queue_pop_if> **m_consumers;    ///<  Multiple consumers bind to these


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_queue"; }


  SC_HAS_PROCESS(xtsc_queue);


  /**
   * Constructor for an xtsc_queue.
   *
   * @param     module_name     Name of the xtsc_queue sc_module.
   *
   * @param     queue_parms     The remaining parameters for construction.
   *
   * @see xtsc_queue_parms
   */
  xtsc_queue(sc_core::sc_module_name module_name, const xtsc_queue_parms& queue_parms);


  /// Destructor.
  ~xtsc_queue();


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Get the number of producers allowed to bind to this module instance
  xtsc::u32 get_num_producers();


  /// Get the number of consumers allowed to bind to this module instance
  xtsc::u32 get_num_consumers();


  /**
   * Reset the xtsc_queue.
   */
  void reset(bool hard_reset = false);


  /// To drain the fifo's after a reset
  void drain_fifo_method();


  /**
   * Dump the contents of the xtsc_queue.
   */
  void dump(std::ostream &os);


  /**
   * Return the number of available data items in the queue (how many more elements can
   * be popped out).  If "pop_file" was specified then this method returns 0 or 1
   * depending on whether or not the end of file has been reached.
   */
  xtsc::u32 num_available();


  /**
   * Return the number of free spaces in the queue (how many more elements can be pushed
   * in).  If "push_file" was specified then this method always returns 1.
   */
  xtsc::u32 num_free();


  /**
   * Return the value of the nth element from the front of the queue.  An exception is
   * thrown if the queue does not currently have n elements or if "push_file" or
   * "pop_file" was specified.
   *
   * @param     nth             Specifies which element is desired (starting at 1).
   *
   * @param     value           Used to return the value of the nth element.
   */
  void peek(xtsc::u32 nth, sc_dt::sc_unsigned& value);


  /**
   * Overwrite the nth element from the front of the queue using the specified value.  
   * An exception is thrown if the queue does not currently have n elements or if
   * "push_file" or "pop_file" was specified.
   *
   * @param     nth             Specifies which element is desired (starting at 1).
   *
   * @param     value           The value to overwrite the nth element with.
   */
  void poke(xtsc::u32 nth, const sc_dt::sc_unsigned& value);


  /// Implementation of the xtsc::xtsc_command_handler_interface.
  virtual void man(std::ostream& os);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        can_pop [<PopPort>]
          Return nb_can_pop() for the specified <PopPort> (default 0).

        can_push [<PushPort>]
          Return nb_can_push() for the specified <PushPort> (default 0).

        dump
          Return the os buffer from calling xtsc_queue::dump(os).

        get_bit_width
          Return xtsc_queue::get_bit_width().

        get_num_consumers
          Return xtsc_queue::get_num_consumers().

        get_num_producers
          Return xtsc_queue::get_num_producers().

        get_pop_ticket
          Return the ticket of most recent previous element popped.

        get_push_ticket
          Return the ticket of most recent previous element pushed.

        num_available
          Return xtsc_queue::num_available().

        num_free
          Return xtsc_queue::num_free().

        peek <nth>
          Return value from calling xtsc_queue::peek(<nth>, value).

        poke <nth> <NewValue>
          Call xtsc_queue::poke(<nth>, <NewValue>).  Return old value.

        pop [<PopPort>]
          Return the value popped by calling nb_pop() for the specified <PopPort>
          (default 0).  Throws if cannot pop.

        push <Value> [<PushPort>]
          Return nb_push(<Value>) for the specified <PushPort> (default 0).

        reset
          Call xtsc_queue::reset().
      \endverbatim
   * 
   * Notes:
   *  - The poke command is not supported when "push_file" is specified.
   *  - The peek command is not supported when "pop_file" is specfied.
   *  - The can_push and push commands are not supported when "num_producers" is 0.
   *  - The can_pop and pop commands are not supported when "num_consumser" is 0.
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect to an xtsc_core.
   *
   * This method connects this xtsc_queue to the named input or output queue interface
   * of an xtsc_core.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     queue_name      Queue interface name as it appears in the user's TIE
   *                            code after the queue keyword.  This name must NOT begin
   *                            with the "TIE_" prefix.
   *
   * @param     port_num        If queue_name is an output queue interface of core then
   *                            this specifies the producer port number of this
   *                            xtsc_queue.  If queue_name is an input queue interface
   *                            of core then this specifies the consumer port number of
   *                            this xtsc_queue.
   *
   */
  void connect(xtsc::xtsc_core& core, const char *queue_name, xtsc::u32 port_num = 0);


  /**
   * Connect to an xtsc_queue_producer.
   *
   * This method connects the specified producer port of this xtsc_queue to the
   * specified xtsc_queue_producer.
   *
   * @param     producer        The xtsc_queue_producer to connect to.
   *
   * @param     port_num        The producer port number of this xtsc_queue to connect
   *                            to.
   *
   */
  void connect(xtsc_queue_producer& producer, xtsc::u32 port_num = 0);


  /**
   * Connect to an xtsc_queue_consumer.
   *
   * This method connects the specified consumer port of this xtsc_queue to the
   * specified xtsc_queue_consumer.
   *
   * @param     consumer        The xtsc_queue_consumer to connect to.
   *
   * @param     port_num        The consumer port number of this xtsc_queue to connect
   *                            to.
   *
   */
  void connect(xtsc_queue_consumer& consumer, xtsc::u32 port_num = 0);


  /// Get the width of this xtsc_queue in bits
  xtsc::u32 get_bit_width() { return m_width1; }


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


private:

  /// Throw an exception if using neither sc_fifo nor host OS shared memory (because "push_file" or "pop_file" was specified)
  void confirm_has_fifo_or_shmem(const char *function);

  /// Return the index of the nth element from the front or throw if there is none.
  xtsc::u32 get_index_of_nth_from_front(xtsc::u32 nth, const char *function);

  /// Get the xtsc_queue_push_if associated with port
  xtsc::xtsc_queue_push_if& get_queue_push_interface(xtsc::u32 port);

  /// Get the xtsc_queue_pop_if associated with port
  xtsc::xtsc_queue_pop_if& get_queue_pop_interface(xtsc::u32 port);

  /// Handle bookkeeping to support multi-client queue
  void delta_cycle_method();

  /// Set m_has_pop_file_element and m_pop_file_element
  void get_next_pop_file_element();


  /// Implementation of xtsc_queue_push_if for single producer.
  class xtsc_queue_push_if_impl : public xtsc::xtsc_queue_push_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_queue_push_if_impl(const char *object_name, xtsc_queue& queue) :
      sc_object (object_name),
      m_queue   (queue),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_queue_push_if
    xtsc::u32 nb_num_free_spaces();

    /// @see xtsc::xtsc_queue_push_if
    bool nb_can_push();

    /// @see xtsc::xtsc_queue_push_if
    bool nb_push(const sc_dt::sc_unsigned& element, xtsc::u64& ticket = push_ticket);

    /// @see xtsc::xtsc_queue_push_if
    xtsc::u32 nb_get_bit_width() { return m_queue.m_width1; }

    /**
     * Get the event that will be notified when the queue transitions from full
     * to not full.
     */
    virtual const sc_core::sc_event& default_event() const { return m_queue.m_nonfull_event; }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_queue&                 m_queue;                ///< Our xtsc_queue object
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  /// Implementation of xtsc_queue_pop_if for single consumer.
  class xtsc_queue_pop_if_impl : public xtsc::xtsc_queue_pop_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_queue_pop_if_impl(const char *object_name, xtsc_queue& queue) :
      sc_object (object_name),
      m_queue   (queue),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_queue_pop_if
    xtsc::u32 nb_num_available_entries();

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_can_pop();

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_pop(sc_dt::sc_unsigned& element, xtsc::u64& ticket = pop_ticket);

    /// @see xtsc::xtsc_queue_pop_if
    void nb_peek(sc_dt::sc_unsigned& element, xtsc::u64& ticket = pop_ticket);

    /// @see xtsc::xtsc_queue_pop_if
    xtsc::u32 nb_get_bit_width() { return m_queue.m_width1; }

    /**
     * Get the event that will be notified when the queue transitions from empty
     * to not empty.
     */
    virtual const sc_core::sc_event& default_event() const { return m_queue.m_nonempty_event; }


  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_queue&                 m_queue;                ///< Our xtsc_queue object
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  /// Implementation of xtsc_queue_push_if for multi-client queue (either m_num_producers or m_num_consumers > 1)
  class xtsc_queue_push_if_multi_impl : public xtsc::xtsc_queue_push_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_queue_push_if_multi_impl(const char *object_name, xtsc_queue& queue, xtsc::u32 port_num) :
      sc_object (object_name),
      m_queue   (queue),
      m_port_num(port_num),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_queue_push_if
    xtsc::u32 nb_num_free_spaces();

    /// @see xtsc::xtsc_queue_push_if
    bool nb_can_push();

    /// @see xtsc::xtsc_queue_push_if
    bool nb_push(const sc_dt::sc_unsigned& element, xtsc::u64& ticket = push_ticket);

    /// @see xtsc::xtsc_queue_push_if
    xtsc::u32 nb_get_bit_width() { return m_queue.m_width1; }

    /**
     * Get the event that will be notified when the queue transitions from full
     * to not full.
     */
    virtual const sc_core::sc_event& default_event() const {
      return m_queue.m_use_fifo ? *m_queue.m_nonfull_events[m_port_num] : m_queue.m_nonfull_event;
    }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_queue&                 m_queue;                ///< Our xtsc_queue object
    xtsc::u32                   m_port_num;             ///< Our producer port number
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  /// Implementation of xtsc_queue_pop_if for multi-client queue (either m_num_producers or m_num_consumers > 1)
  class xtsc_queue_pop_if_multi_impl : public xtsc::xtsc_queue_pop_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_queue_pop_if_multi_impl(const char *object_name, xtsc_queue& queue, xtsc::u32 port_num) :
      sc_object (object_name),
      m_queue   (queue),
      m_port_num(port_num),
      m_p_port  (0)
    {}

    /// @see xtsc::xtsc_queue_pop_if
    xtsc::u32 nb_num_available_entries();

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_can_pop();

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_pop(sc_dt::sc_unsigned& element, xtsc::u64& ticket = pop_ticket);

    /// @see xtsc::xtsc_queue_pop_if
    xtsc::u32 nb_get_bit_width() { return m_queue.m_width1; }

    /**
     * Get the event that will be notified when the queue transitions from empty
     * to not empty.
     */
    virtual const sc_core::sc_event& default_event() const {
      return m_queue.m_use_fifo ? *m_queue.m_nonempty_events[m_port_num] : m_queue.m_nonempty_event;
    }


  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_queue&                 m_queue;                ///< Our xtsc_queue object
    xtsc::u32                   m_port_num;             ///< Our consumer port number
    sc_core::sc_port_base      *m_p_port;               ///< Port that is bound to us
  };


  // We use an sc_fifo, m_fifo, in addition to the m_element_ptrs table because the
  // sc_fifo uses evaluate-update semantics to ensure determinacy.


  xtsc_queue_push_if_impl              *m_push_impl;               ///<  m_producer binds to this if m_multi_client is false
  xtsc_queue_pop_if_impl               *m_pop_impl;                ///<  m_consumer binds to this if m_multi_client is false

  xtsc_queue_push_if_multi_impl       **m_push_multi_impl;         ///<  m_producers bind to these if m_multi_client is true
  xtsc_queue_pop_if_multi_impl        **m_pop_multi_impl;          ///<  m_consumers bind to these if m_multi_client is true

  xtsc::u32                             m_num_producers;           ///<  From "num_producers" parameter
  xtsc::u32                             m_num_consumers;           ///<  From "num_consumers" parameter
  bool                                  m_multi_client;            ///<  true if either m_num_producers or m_num_consumers exceeds 1
  xtsc::u32                             m_depth;                   ///<  Capacity - number of elements
  sc_core::sc_fifo<int>                 m_fifo;                    ///<  Indexes into m_element_ptrs (to ensure determinacy)
  bool                                  m_use_fifo;                ///<  True if using sc_fifo, false if using a file of host shared memory
  sc_dt::sc_unsigned                  **m_element_ptrs;            ///<  To store the elements
  xtsc::u64                            *m_tickets;                 ///<  To store the ticket associated with each element
  xtsc::u32                             m_skid_index;              ///<  Move from skid buffer to m_fifo in fair, round-robin fashion
  sc_core::sc_fifo<int>               **m_skid_fifos;              ///<  A entry in m_skid_fifos[N] says that m_skid_buffers[N] is valid
  sc_dt::sc_unsigned                  **m_skid_buffers;            ///<  Skid buffers to store the elements
  xtsc::u64                            *m_skid_tickets;            ///<  To store the queue tickets associated with the skid buffers
  xtsc::u32                             m_jerk_index;              ///<  Move from m_fifo to jerk buffer in fair, round-robin fashion
  sc_core::sc_fifo<int>               **m_jerk_fifos;              ///<  A entry in m_jerk_fifos[N] says that m_jerk_buffers[N] is valid
  sc_dt::sc_unsigned                  **m_jerk_buffers;            ///<  Jerk buffers to store the elements
  xtsc::u64                            *m_jerk_tickets;            ///<  To store the queue tickets associated with the jerk buffers
  xtsc::u64                             m_pop_ticket;              ///<  Save ticket of last popped value
  xtsc::u64                             m_push_ticket;             ///<  Save ticket of last pushed value
  sc_dt::sc_unsigned                    m_dummy;                   ///<  For logging failed pushes and pops
  xtsc::u32                             m_width1;                  ///<  Bit width of each element
  xtsc::u32                             m_width8;                  ///<  Byte width of each element
  sc_dt::sc_unsigned                    m_value;                   ///<  For temporary use within a method
  xtsc::u32                             m_next;                    ///<  Next slot in m_element_ptrs[] and m_tickets[]
  log4xtensa::TextLogger&               m_text;                    ///<  Text logger
  log4xtensa::BinaryLogger&             m_binary;                  ///<  Binary logger
  bool                                  m_log_data_binary;         ///<  True if transaction data should be logged by m_binary
  std::string                           m_push_file_name;          ///<  Name of file to write elements to instead of using the fifo
  std::string                           m_pop_file_name;           ///<  Name of file to read elements from instead of using the fifo
  std::ofstream                        *m_push_file;               ///<  File to write elements to instead of using the fifo
  bool                                  m_timestamp;               ///<  From "timestamp" parameter
  xtsc::xtsc_script_file               *m_pop_file;                ///<  File to read elements from instead of using the fifo
  bool                                  m_wraparound;              ///<  False if m_pop_file should only be read through one time
  bool                                  m_has_pop_file_element;    ///<  For use by nb_can_pop() and num_available() when using m_pop_file
  sc_dt::sc_unsigned                    m_pop_file_element;        ///<  For use by nb_pop() when using m_pop_file
  std::vector<std::string>              m_words;                   ///<  The tokenized words of the current line from m_pop_file
  std::string                           m_line;                    ///<  The current line from m_pop_file
  sc_core::sc_event                     m_drain_fifo_event;        ///<  To drain the fifos after a reset
  sc_core::sc_event                     m_push_pop_event;          ///<  multi-client: notify delta_cycle_method(); else each peek/poke
  sc_core::sc_event                     m_nonempty_event;          ///<  The no-longer-empty event
  sc_core::sc_event                     m_nonfull_event;           ///<  The no-longer-full event
  sc_core::sc_event                   **m_nonempty_events;         ///<  The no-longer-empty events when m_multi_client is true
  sc_core::sc_event                   **m_nonfull_events;          ///<  The no-longer-full events when m_multi_client is true
  xtsc::u32                             m_next_word_index;         ///<  Index into m_words.
  xtsc::u32                             m_pop_file_line_number;    ///<  The line number of m_words in m_pop_file
  bool                                  m_host_shared_memory;      ///<  See "host_shared_memory" parameter
  std::string                           m_shmem_name;              ///<  Shared Memory: name
  xtsc::u8                             *m_p_shmem;                 ///<  Shared Memory: pointer to host OS shared memory
  xtsc::u32                             m_shmem_bytes_per_row;     ///<  Shared Memory: number of bytes per row ((64+m_width1+63)/64)*8
  xtsc::u32                             m_shmem_num_rows;          ///<  Shared Memory: number of rows (m_depth + 1)
  xtsc::u32                             m_shmem_array_size;        ///<  Shared Memory: number of bytes in all rows
  xtsc::u32                             m_shmem_total_size;        ///<  Shared Memory: number of bytes in all rows plus read/write indices
  xtsc::u32                            *m_p_shmem_ridx;            ///<  Shared Memory: pointer to ridx (ridx = Read row InDeX)
  xtsc::u32                            *m_p_shmem_widx;            ///<  Shared Memory: pointer to widx (widx = Write row InDeX)

};



}  // namespace xtsc_component

#endif  // _XTSC_QUEUE_H_
