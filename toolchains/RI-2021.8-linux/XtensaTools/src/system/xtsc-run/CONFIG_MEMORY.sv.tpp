// Copyright (c) 2008-2009 Tensilica Inc.  ALL RIGHTS RESERVED.
// 
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

;#
;# This file is copied to swtools-{machine}-{os}/src/system/xtsc-run/CONFIG_MEMORY.sv.tpp
;# by running "make install-src" in builds-{machine}-{os}/_swtools_/xbuild/Software
;#
; my $byte_width                = $bit_width / 8;
; my $memory_end                = $memory_start + $memory_byte_size - 1;
; my $memory_words              = $memory_byte_size / $byte_width;
; my $memory_index_bits         = !$memory_byte_size ? 1 : int((log($memory_words) / log(2)) + 0.5);
; my $index_lsb                 = ($byte_width == 4) ? 2 : ($byte_width == 8) ? 3 : 4;
; my $byte_lane_msb             = $index_lsb - 1;
;#
;sub right {
;  my ($digits, $value) = @_;
;  my $dig = $value > 999 ? 4 : $value > 99 ? 3 : $value > 9 ? 2 : 1;
;  my $spaces = "          ";
;  my $num_spaces = $dig >= $digits ? 0 : ($digits - $dig);
;  my $prefix = substr($spaces, 0, $num_spaces);
;  sprintf("%s%d", $prefix, $value);
;}
;sub hx {
;  my ($nibbles, $value) = @_;
;  sprintf("%0". $nibbles. "X", $value);
;}



// To enable route ID, uncomment the following and change xxx to the number of bits minus 1
// `define `$config_name`_`$memory_name`_ROUTE_ID



module `$config_name`_`$memory_name`(
  CLK,

  POReqValid,
  PIReqRdy,
  POReqCntl,
  POReqAdrs,
  POReqData,
  POReqDataBE,
  POReqId,
  POReqPriority,

  PIRespValid,
  PORespRdy,
  PIRespCntl,
  PIRespData,
  PIRespId,
  PIRespPriority

`ifdef `$config_name`_`$memory_name`_ROUTE_ID
  ,
  POReqRouteId,
  PIRespRouteId
`endif
);

  input                 CLK;

  input                 POReqValid;
  output                PIReqRdy;
  input         [  7:0] POReqCntl;
  input         [ 31:0] POReqAdrs;
  input         [` right(3, $bit_width-1)`:0] POReqData;
  input         [` right(3, $byte_width-1)`:0] POReqDataBE;
  input         [  5:0] POReqId;
  input         [  1:0] POReqPriority;

  output                PIRespValid;
  input                 PORespRdy;
  output        [  7:0] PIRespCntl;
  output        [` right(3, $bit_width-1)`:0] PIRespData;
  output        [  5:0] PIRespId;
  output        [  1:0] PIRespPriority;

`ifdef `$config_name`_`$memory_name`_ROUTE_ID
  // Change xxx to the number of bits minus 1
  input         [xxx:0] POReqRouteId;
  output        [xxx:0] PIRespRouteId;
  reg           [xxx:0] PIRespRouteId           =  0;
`endif

  reg           [  7:0] PIRespCntl              =   8'h0;
  reg           [  5:0] PIRespId                =   6'h0;
  reg           [  1:0] PIRespPriority          =   2'h0;
  reg                   PIReqRdy                =   1'h0;
  reg                   PIRespValid             =   1'h0;

  reg                   memory_write            =   1'h0;
  reg           [` right(3, $memory_index_bits-1)`:0] memory_index            =  ` right(2, $memory_index_bits)`'h0;
  reg           [` right(3, $byte_width-1)`:0] memory_byte_enables     =  ` right(2, $byte_width)`'h0;
  reg           [` right(3, $bit_width-1)`:0] memory_write_data       = ` right(3, $bit_width)`'h0;
  wire          [` right(3, $bit_width-1)`:0] memory_read_data;

  // Synopsys Y-2006.06-SP1-9 does not support verilog parameters in a SystemC-on-Top simulation, so we hard code a value
  parameter file_name = "../`$memory_name`.data";



  integer               bus_byte_width          = `$byte_width`;

  integer               READ                    =   4'b0000;
  integer               WRITE                   =   4'b1000;
  integer               BLOCK_READ              =   4'b0001;
  integer               BURST_READ              =   4'b0011;
  integer               BLOCK_WRITE             =   4'b1001;
  integer               BURST_WRITE             =   4'b1011;
  integer               RCW                     =   4'b0101;

  integer               RSP_NONE                =   8'hFF;       // Value to drive when there is no response
  integer               RSP_OK                  =   7'b0000000_; // PIRespCntl[7:1]
  integer               RSP_ADDR_ERROR          =   7'b0000001_; // PIRespCntl[7:1]

  integer               request_prev            =   1'h0;        // There was a request on the prev cycle which was accepted
  integer               request_last            =   1'h0;        // The accepted request was a last-transfer request
  integer               num_responses           =   0;           // Num of responses expected due to accepted last-xfer req
  integer               response_prev           =   1'h0;        // There was a response on the prev cycle which was accepted
  integer               response_count          =   0;           // Count down of remaining responses

  integer               memory_hit              =   1'h0;        // Request accepted last cycle targets memory
  integer               memory_start            =  32'h` hx(8,$memory_start)`; // Start address of memory
  integer               memory_end              =  32'h` hx(8,$memory_end)`; // End address of memory

  integer               address                 =  32'h00000000; // Save POReqAdrs of "first" transfer
  integer               error_resp_pending      =   1'h0;        // Keep track until last transfer request that error response is due 
  integer               first_transfer          =   1'h1;        // The next request to come will be a "first" transfer
  integer               write_beat              =   5'h0;        // Keep track of BLOCK_WRITE|BURST_WRITE beats
  integer               read_beat               =   5'h0;        // Keep track of BLOCK_READ|BURST_READ beats
  integer               block_read_max_beat     =  0;            // Support BLOCK_READ wrap-around (for critical word first)
  integer               block_read_min_beat     =  0;            // Support BLOCK_READ wrap-around (for critical word first)
  integer               is_rcw                  =   1'h0;        // RCW
  integer               is_write                =   1'h0;        // WRITE|BLOCK_WRITE|BURST_WRITE|RCW
  integer               addr_byte_lanes         =   `$index_lsb`'h0;        // Bits of POReqAdrs which indicate byte lane
  integer               aligned_ok              =   1'h0;        // Legal combination of address low-order bits and byte enables
  integer               do_write                =   1'h0;        // WRITE|BLOCK_WRITE|BURST_WRITE|(RCW#2 && rcw_data_matches)
  integer               start_read              =   1'h0;        // READ|BLOCK_READ|BURST_READ|RCW#1
  integer               read_byte_enables       =  ` right(2, $byte_width)`'h0;        // Remember read byte enables
  integer               reading_memory          =   1'h0;        // Remember if reads are from memory
  wire                  reading_memory_ud;                       // Remember if reads are from memory (unit delay)
  integer               reading                 =   1'h0;        // Keep track of whether reads are in progress
  integer               reading_delayed         =   1'h0;        // delayed one cycle
  integer               do_read                 =   1'h0;        // Do a read this cycle
  wire                  do_read_ud;                              // Do a read this cycle (unit delay)
  integer               compare_rcw_data        =   1'h0;        // Flag to trigger an RCW compare at negedge clock
  integer               rcw_data_matches        =   1'h0;        // True if RCW compare data matches memory
  reg           [` right(3, $bit_width-1)`:0] rcw_compare_data        = ` right(3, $bit_width)`'h0;        // POReqData from RCW#1
  reg           [` right(3, $bit_width-1)`:0] rcw_old_data            = ` right(3, $bit_width)`'h0;        // Original contents of memory at RCW address
  reg           [` right(3, $byte_width-1)`:0] rcw_byte_enables        =  ` right(2, $byte_width)`'h0;        // POReqDataBE from RCW#1

  reg   /*sparse*/      [` right(3, $bit_width-1)`:0] mem[0:`$memory_index_bits`'h` hx(int(($memory_index_bits+3)/4), $memory_words-1)`];
  reg                   [` right(3, $bit_width-1)`:0] wdata;
  wire                  [` right(3, $bit_width-1)`:0] rdata;



  initial begin
    if($test$plusargs("dumpvars")) begin
      $dumpvars();
    end
    if (!$test$plusargs("preloadmems") || (file_name == "") || (file_name == "<none>")) begin
      $display("%t Not preloading memory %m.", $time);
    end
    else begin
      $display("%t Preloading memory %m from %s", $time, file_name);
      $readmemh(file_name, mem);
    end
  end



  always @(posedge CLK) begin
    reading_delayed     = reading;
    response_prev       = PIRespValid && PORespRdy;
    request_prev        = POReqValid && PIReqRdy;
    request_last        = request_prev && POReqCntl[0];
    is_rcw              = request_prev && (POReqCntl[7:4] == RCW);
    compare_rcw_data    = (is_rcw && first_transfer) ? 1 : 0;
    is_write            = request_prev && ((POReqCntl[7:4] == WRITE) ||
                                           (POReqCntl[7:4] == BLOCK_WRITE) ||
                                           (POReqCntl[7:4] == BURST_WRITE) ||
                                           (POReqCntl[7:4] == RCW));
    addr_byte_lanes     = POReqAdrs[`$byte_lane_msb`:0];
    aligned_ok          = request_prev && (
; if ($bit_width == 128) {
;   if ($little_endian) {
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b0000_0000_0000_0001)) ||
                          ((addr_byte_lanes == 4'h1) && (POReqDataBE == 16'b0000_0000_0000_0010)) ||
                          ((addr_byte_lanes == 4'h2) && (POReqDataBE == 16'b0000_0000_0000_0100)) ||
                          ((addr_byte_lanes == 4'h3) && (POReqDataBE == 16'b0000_0000_0000_1000)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_0000_0001_0000)) ||
                          ((addr_byte_lanes == 4'h5) && (POReqDataBE == 16'b0000_0000_0010_0000)) ||
                          ((addr_byte_lanes == 4'h6) && (POReqDataBE == 16'b0000_0000_0100_0000)) ||
                          ((addr_byte_lanes == 4'h7) && (POReqDataBE == 16'b0000_0000_1000_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0001_0000_0000)) ||
                          ((addr_byte_lanes == 4'h9) && (POReqDataBE == 16'b0000_0010_0000_0000)) ||
                          ((addr_byte_lanes == 4'ha) && (POReqDataBE == 16'b0000_0100_0000_0000)) ||
                          ((addr_byte_lanes == 4'hb) && (POReqDataBE == 16'b0000_1000_0000_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b0001_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'hd) && (POReqDataBE == 16'b0010_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'he) && (POReqDataBE == 16'b0100_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'hf) && (POReqDataBE == 16'b1000_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b0000_0000_0000_0011)) ||
                          ((addr_byte_lanes == 4'h2) && (POReqDataBE == 16'b0000_0000_0000_1100)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_0000_0011_0000)) ||
                          ((addr_byte_lanes == 4'h6) && (POReqDataBE == 16'b0000_0000_1100_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0011_0000_0000)) ||
                          ((addr_byte_lanes == 4'ha) && (POReqDataBE == 16'b0000_1100_0000_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b0011_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'he) && (POReqDataBE == 16'b1100_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b0000_0000_0000_1111)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_0000_1111_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_1111_0000_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b1111_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b0000_0000_1111_1111)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b1111_1111_0000_0000)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1111_1111_1111_1111)) ||
;   } else {
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1000_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h1) && (POReqDataBE == 16'b0100_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h2) && (POReqDataBE == 16'b0010_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h3) && (POReqDataBE == 16'b0001_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_1000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h5) && (POReqDataBE == 16'b0000_0100_0000_0000)) ||
                          ((addr_byte_lanes == 4'h6) && (POReqDataBE == 16'b0000_0010_0000_0000)) ||
                          ((addr_byte_lanes == 4'h7) && (POReqDataBE == 16'b0000_0001_0000_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0000_1000_0000)) ||
                          ((addr_byte_lanes == 4'h9) && (POReqDataBE == 16'b0000_0000_0100_0000)) ||
                          ((addr_byte_lanes == 4'ha) && (POReqDataBE == 16'b0000_0000_0010_0000)) ||
                          ((addr_byte_lanes == 4'hb) && (POReqDataBE == 16'b0000_0000_0001_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b0000_0000_0000_1000)) ||
                          ((addr_byte_lanes == 4'hd) && (POReqDataBE == 16'b0000_0000_0000_0100)) ||
                          ((addr_byte_lanes == 4'he) && (POReqDataBE == 16'b0000_0000_0000_0010)) ||
                          ((addr_byte_lanes == 4'hf) && (POReqDataBE == 16'b0000_0000_0000_0001)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1100_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h2) && (POReqDataBE == 16'b0011_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_1100_0000_0000)) ||
                          ((addr_byte_lanes == 4'h6) && (POReqDataBE == 16'b0000_0011_0000_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0000_1100_0000)) ||
                          ((addr_byte_lanes == 4'ha) && (POReqDataBE == 16'b0000_0000_0011_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b0000_0000_0000_1100)) ||
                          ((addr_byte_lanes == 4'he) && (POReqDataBE == 16'b0000_0000_0000_0011)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1111_0000_0000_0000)) ||
                          ((addr_byte_lanes == 4'h4) && (POReqDataBE == 16'b0000_1111_0000_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0000_1111_0000)) ||
                          ((addr_byte_lanes == 4'hc) && (POReqDataBE == 16'b0000_0000_0000_1111)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1111_1111_0000_0000)) ||
                          ((addr_byte_lanes == 4'h8) && (POReqDataBE == 16'b0000_0000_1111_1111)) ||
                          ((addr_byte_lanes == 4'h0) && (POReqDataBE == 16'b1111_1111_1111_1111)) ||
;   }
; }
; if ($bit_width == 64) {
;   if ($little_endian) {
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b0000_0001)) ||
                          ((addr_byte_lanes == 3'h1) && (POReqDataBE == 8'b0000_0010)) ||
                          ((addr_byte_lanes == 3'h2) && (POReqDataBE == 8'b0000_0100)) ||
                          ((addr_byte_lanes == 3'h3) && (POReqDataBE == 8'b0000_1000)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b0001_0000)) ||
                          ((addr_byte_lanes == 3'h5) && (POReqDataBE == 8'b0010_0000)) ||
                          ((addr_byte_lanes == 3'h6) && (POReqDataBE == 8'b0100_0000)) ||
                          ((addr_byte_lanes == 3'h7) && (POReqDataBE == 8'b1000_0000)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b0000_0011)) ||
                          ((addr_byte_lanes == 3'h2) && (POReqDataBE == 8'b0000_1100)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b0011_0000)) ||
                          ((addr_byte_lanes == 3'h6) && (POReqDataBE == 8'b1100_0000)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b0000_1111)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b1111_0000)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b1111_1111)) ||
;   } else {
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b1000_0000)) ||
                          ((addr_byte_lanes == 3'h1) && (POReqDataBE == 8'b0100_0000)) ||
                          ((addr_byte_lanes == 3'h2) && (POReqDataBE == 8'b0010_0000)) ||
                          ((addr_byte_lanes == 3'h3) && (POReqDataBE == 8'b0001_0000)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b0000_1000)) ||
                          ((addr_byte_lanes == 3'h5) && (POReqDataBE == 8'b0000_0100)) ||
                          ((addr_byte_lanes == 3'h6) && (POReqDataBE == 8'b0000_0010)) ||
                          ((addr_byte_lanes == 3'h7) && (POReqDataBE == 8'b0000_0001)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b1100_0000)) ||
                          ((addr_byte_lanes == 3'h2) && (POReqDataBE == 8'b0011_0000)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b0000_1100)) ||
                          ((addr_byte_lanes == 3'h6) && (POReqDataBE == 8'b0000_0011)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b1111_0000)) ||
                          ((addr_byte_lanes == 3'h4) && (POReqDataBE == 8'b0000_1111)) ||
                          ((addr_byte_lanes == 3'h0) && (POReqDataBE == 8'b1111_1111)) ||
;   }
; }
; if ($bit_width == 32) {
;   if ($little_endian) {
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b0001)) ||
                          ((addr_byte_lanes == 2'h1) && (POReqDataBE == 4'b0010)) ||
                          ((addr_byte_lanes == 2'h2) && (POReqDataBE == 4'b0100)) ||
                          ((addr_byte_lanes == 2'h3) && (POReqDataBE == 4'b1000)) ||
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b0011)) ||
                          ((addr_byte_lanes == 2'h2) && (POReqDataBE == 4'b1100)) ||
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b1111)) ||
;   } else {
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b1000)) ||
                          ((addr_byte_lanes == 2'h1) && (POReqDataBE == 4'b0100)) ||
                          ((addr_byte_lanes == 2'h2) && (POReqDataBE == 4'b0010)) ||
                          ((addr_byte_lanes == 2'h3) && (POReqDataBE == 4'b0001)) ||
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b1100)) ||
                          ((addr_byte_lanes == 2'h2) && (POReqDataBE == 4'b0011)) ||
                          ((addr_byte_lanes == 2'h0) && (POReqDataBE == 4'b1111)) ||
;   }
; }
                          0);
    memory_hit          = request_prev && (POReqAdrs >= memory_start) && (POReqAdrs <= memory_end);
    error_resp_pending  = (error_resp_pending && !response_prev) ||
                          (request_prev && !aligned_ok) ||
                          (request_prev && !memory_hit);
    num_responses       = error_resp_pending             ? 1                     :  // An error response is pending
                          !request_last                  ? 0                     :  // Last cycle didn't have a last-xfer req accepted
                          (POReqCntl[7:4] == BLOCK_READ) ? (2 << POReqCntl[2:1]) :  // BLOCK_READ
                          (POReqCntl[7:4] == BURST_READ) ? (POReqCntl[3:1] + 1)  :  // BURST_READ
                                                           1;                       // All others
    response_count      = (request_prev ? num_responses : (response_count - (response_prev ? 1 : 0)));
    PIRespValid         = (response_count != 0);
    PIReqRdy            = (response_count == 0);
    do_write            = request_prev && !error_resp_pending && ((POReqCntl[7:4] == WRITE) ||
                                                                  (POReqCntl[7:4] == BLOCK_WRITE) ||
                                                                  (POReqCntl[7:4] == BURST_WRITE) ||
                                                                 ((POReqCntl[7:4] == RCW) && request_last && rcw_data_matches));
    start_read          = request_prev && !error_resp_pending && ((POReqCntl[7:4] == READ) ||
                                                                  (POReqCntl[7:4] == BLOCK_READ) ||
                                                                  (POReqCntl[7:4] == BURST_READ) ||
                                                                 ((POReqCntl[7:4] == RCW) && !request_last));
    do_read             = start_read || (reading && (response_count != 0));
    write_beat          = !do_write ? write_beat : first_transfer ? 0 : (write_beat + (is_rcw ? 0 : 1));
    read_beat           = !do_read  ? read_beat  : start_read     ? 0 : (read_beat  + ((is_rcw || !response_prev) ? 0 : 1));

    if (request_prev && first_transfer) begin
      address           = POReqAdrs;
      PIRespId          = POReqId;
`ifdef `$config_name`_`$memory_name`_ROUTE_ID
      PIRespRouteId     = POReqRouteId;
`endif
      PIRespPriority    = POReqPriority;
      if (POReqCntl[7:4] == BLOCK_READ) begin
        block_read_max_beat = num_responses - ((address % (bus_byte_width * num_responses)) / bus_byte_width) - 1;
        block_read_min_beat = block_read_max_beat - num_responses + 1;
      end
      else begin
        block_read_max_beat = 16;
        block_read_min_beat = 0;
      end
    end

    if (do_read && (read_beat > block_read_max_beat)) begin
      read_beat         = block_read_min_beat;
    end

    rcw_compare_data    = compare_rcw_data ? POReqData   : ` right(3, $bit_width)`'h0;
    rcw_byte_enables    = compare_rcw_data ? POReqDataBE : ` right(3, $byte_width)`'h0;

    if (start_read) begin
      reading_memory    = memory_hit;
      read_byte_enables = POReqDataBE;
    end

    memory_write                =   1'h0;
    memory_index                = ` right(3, $memory_index_bits)`'h0;
    memory_byte_enables         = ` right(3, $byte_width)`'h0;
    memory_write_data           = ` right(3, $bit_width)`'h0;

    if (do_write) begin
      memory_write              = 1'h1;
      memory_index              = address[`$index_lsb+$memory_index_bits-1`:`$index_lsb`] + write_beat;
      memory_byte_enables       = POReqDataBE;
      memory_write_data         = POReqData;
    end
    else if (do_read) begin
      if (reading_memory) begin
        memory_index            = address[`$index_lsb+$memory_index_bits-1`:`$index_lsb`] + read_beat;
        memory_byte_enables     = read_byte_enables;
      end
    end

    PIRespCntl         <= #1 ((response_count == 0) ? RSP_NONE : 
                              { (error_resp_pending ? RSP_ADDR_ERROR : RSP_OK), ((response_count == 1) ? 1'h1 : 1'h0)});

    reading            <= #1 (start_read || (reading_delayed && PIRespValid));
    first_transfer     <= #1 request_prev ? request_last : first_transfer;
  end


  always @(negedge CLK) begin
    rcw_data_matches = 1'h0;
    if (compare_rcw_data) begin
      rcw_old_data = memory_read_data;
      rcw_data_matches = 1'h1;
      if ((rcw_byte_enables[ 0] && (rcw_compare_data[  7:  0] !== rcw_old_data[  7:  0])) ||
          (rcw_byte_enables[ 1] && (rcw_compare_data[ 15:  8] !== rcw_old_data[ 15:  8])) ||
          (rcw_byte_enables[ 2] && (rcw_compare_data[ 23: 16] !== rcw_old_data[ 23: 16])) ||
; for (my $i=3; $i < $byte_width; $i = $i + 1) {
;   my $eol = (($i < $byte_width - 1) ? " ||" : ") rcw_data_matches = 1'h0;");
          (rcw_byte_enables[` right(2, $i)`] && (rcw_compare_data[` right(3, $i*8+7)`:` right(3, $i*8)`] !== rcw_old_data[` right(3, $i*8+7)`:` right(3, $i*8)`]))`$eol`
; }
    end
  end


  assign #1 do_read_ud          = do_read;
  assign #1 reading_memory_ud   = reading_memory;
  assign PIRespData             = do_read_ud ?  memory_read_data : `$bit_width`'h0;

  assign rdata = mem[memory_index];

  assign #1 memory_read_data = {
; if ($byte_width > 8) {
                         (memory_byte_enables[15] ? rdata[127:120] : 8'h0),
                         (memory_byte_enables[14] ? rdata[119:112] : 8'h0),
                         (memory_byte_enables[13] ? rdata[111:104] : 8'h0),
                         (memory_byte_enables[12] ? rdata[103: 96] : 8'h0),
                         (memory_byte_enables[11] ? rdata[ 95: 88] : 8'h0),
                         (memory_byte_enables[10] ? rdata[ 87: 80] : 8'h0),
                         (memory_byte_enables[ 9] ? rdata[ 79: 72] : 8'h0),
                         (memory_byte_enables[ 8] ? rdata[ 71: 64] : 8'h0),
; }
; if ($byte_width > 4) {
                         (memory_byte_enables[ 7] ? rdata[ 63: 56] : 8'h0),
                         (memory_byte_enables[ 6] ? rdata[ 55: 48] : 8'h0),
                         (memory_byte_enables[ 5] ? rdata[ 47: 40] : 8'h0),
                         (memory_byte_enables[ 4] ? rdata[ 39: 32] : 8'h0),
; }
                         (memory_byte_enables[ 3] ? rdata[ 31: 24] : 8'h0),
                         (memory_byte_enables[ 2] ? rdata[ 23: 16] : 8'h0),
                         (memory_byte_enables[ 1] ? rdata[ 15:  8] : 8'h0),
                         (memory_byte_enables[ 0] ? rdata[  7:  0] : 8'h0)};

  always @(posedge CLK) begin
    if (memory_write) begin
      wdata = mem[memory_index];
      if (memory_byte_enables[ 0]) wdata[  7:  0] = memory_write_data[  7:  0];
      if (memory_byte_enables[ 1]) wdata[ 15:  8] = memory_write_data[ 15:  8];
      if (memory_byte_enables[ 2]) wdata[ 23: 16] = memory_write_data[ 23: 16];
      if (memory_byte_enables[ 3]) wdata[ 31: 24] = memory_write_data[ 31: 24];
; if ($byte_width > 4) {
      if (memory_byte_enables[ 4]) wdata[ 39: 32] = memory_write_data[ 39: 32];
      if (memory_byte_enables[ 5]) wdata[ 47: 40] = memory_write_data[ 47: 40];
      if (memory_byte_enables[ 6]) wdata[ 55: 48] = memory_write_data[ 55: 48];
      if (memory_byte_enables[ 7]) wdata[ 63: 56] = memory_write_data[ 63: 56];
; }
; if ($byte_width > 8) {
      if (memory_byte_enables[ 8]) wdata[ 71: 64] = memory_write_data[ 71: 64];
      if (memory_byte_enables[ 9]) wdata[ 79: 72] = memory_write_data[ 79: 72];
      if (memory_byte_enables[10]) wdata[ 87: 80] = memory_write_data[ 87: 80];
      if (memory_byte_enables[11]) wdata[ 95: 88] = memory_write_data[ 95: 88];
      if (memory_byte_enables[12]) wdata[103: 96] = memory_write_data[103: 96];
      if (memory_byte_enables[13]) wdata[111:104] = memory_write_data[111:104];
      if (memory_byte_enables[14]) wdata[119:112] = memory_write_data[119:112];
      if (memory_byte_enables[15]) wdata[127:120] = memory_write_data[127:120];
; }
      mem[memory_index] = wdata;
    end
  end

; my $peek_function = "peek_" . $config_name . "_" . $memory_name;
; my $poke_function = "poke_" . $config_name . "_" . $memory_name;
`ifdef MODEL_TECH
  export "DPI-SC" function `$peek_function`;
`else
  export "DPI-C" function `$peek_function`;
`endif
  function int `$peek_function`;
    input int unsigned address;
    inout int unsigned byte_lanes_3210;
    reg [`$bit_width-1`:0] peek_data;
    int unsigned index;
    int unsigned word;
; if ($bit_width == 32) {
    word = 0;
; } 
; if ($bit_width == 64) {
;   if ($little_endian) {
    word = address[2:2];
;   } else {
    word = 1 - address[2:2];
;   } 
; } 
; if ($bit_width == 128) {
;   if ($little_endian) {
    word = address[3:2];
;   } else {
    word = 3 - address[3:2];
;   } 
; } 
    `$peek_function` = 1;
    if (!(address & 2'h3) && ((address >= memory_start) && (address <= memory_end))) begin
      `$peek_function` = 0;
      index = (address - memory_start) >> `$index_lsb`;
      peek_data = mem[index];
      byte_lanes_3210 = (peek_data >> (word * 32)) & 32'hFFFFFFFF;
    end
  endfunction


`ifdef MODEL_TECH
  export "DPI-SC" function `$poke_function`;
`else
  export "DPI-C" function `$poke_function`;
`endif
  function int `$poke_function`;
    input int unsigned address;
    input int unsigned byte_lanes_3210;
    reg [`$bit_width-1`:0] poke_data;
    int unsigned index;
    int unsigned word;
; if ($bit_width == 32) {
    word = 0;
; } 
; if ($bit_width == 64) {
;   if ($little_endian) {
    word = address[2:2];
;   } else {
    word = 1 - address[2:2];
;   } 
; } 
; if ($bit_width == 128) {
;   if ($little_endian) {
    word = address[3:2];
;   } else {
    word = 3 - address[3:2];
;   } 
; } 
    `$poke_function` = 1;
    if (!(address & 2'h3) && ((address >= memory_start) && (address <= memory_end))) begin
      index = (address - memory_start) >> `$index_lsb`;
      poke_data = mem[index];
; if ($bit_width == 32) {
      poke_data = byte_lanes_3210;
; } 
; if ($bit_width == 64) {
           if (word == 0) poke_data = { poke_data[ 63:32], byte_lanes_3210                  };
      else                poke_data = {                    byte_lanes_3210, poke_data[31:0] };
; } 
; if ($bit_width == 128) {
           if (word == 0) poke_data = { poke_data[127:32], byte_lanes_3210                  };
      else if (word == 1) poke_data = { poke_data[127:64], byte_lanes_3210, poke_data[31:0] };
      else if (word == 2) poke_data = { poke_data[127:96], byte_lanes_3210, poke_data[63:0] };
      else                poke_data = {                    byte_lanes_3210, poke_data[95:0] };
; } 
      `$poke_function` = 0;
      mem[index] = poke_data;
    end
  endfunction


endmodule



