;#  This file gets processed by Xtensa/Software/src/system/Makefile.src,
;#  but is placed here for maintenance along with other OCD files.
<!-- topology.xml  -  JTAG chain topology description for the Xtensa OCD Daemon
; my @cores = $layout->cores;
; my $year = 1900 + (localtime)[5];
     Copyright (c) 2006-`$year` Cadence Design Systems Inc.

     NOTE: This file was automatically generated for subsystem "`$layout->system_name`" (` scalar @cores` cores).
  -->

<configuration>
  <controller id='Controller0' module='ft2232' probe='flyswatter2' speed='10MHz' />
  <!--
	Here are other example controller lines, that can replace the above ones
	when using different JTAG probes (scan controllers):

	Cadence Design Systems ML605 Daughterboard - optional usbser is inventory sticker number prefixed with 'ML605-':
	<controller id='Controller0' module='ft2232' probe='ML605' speed='10MHz' usbser='ML605-2147' />

	Tin Can Tools Flyswatter2:
	<controller id='Controller0' module='ft2232' probe='flyswatter2' speed='10MHz' />

	RVI/DSTREAM: NOTE: Probe settings are in the loaded rvc file
        <controller id='Controller0' module='rvi' />

	JLink IP (10MHz JTCK):
	<controller id='Controller0' module='jlink' ipaddr='192.168.1.1'  port='0' speed='10000000'/>

	JLink USB (10MHz JTCK):
	<controller id='Controller0' module='jlink' usbser='12345678' speed='10000000'/>
   -->

  <driver id='XtensaDriver0' module='xtensa' step-intr='mask,stepover,setps' />
;# Undocumented?:  can add this to the above driver line:  debug='verify'
  <driver id='TraxDriver0'   module='trax' />
  <chain controller='Controller0'>
; foreach my $i (0 .. $#cores) {
    <tap id='TAP`$i`' irwidth='5' />   <!-- core `$cores[$i]->name` -->
; }
  </chain>
  <system module='jtag'>
; foreach my $i (0 .. $#cores) {
    <component id='Component`$i`' tap='TAP`$i`' config='trax' />
; }
  </system>
; foreach my $i (0 .. $#cores) {
  <device id='Xtensa`$i`' component='Component`$i`' driver='XtensaDriver0' />
; }
; # FIXME: support multiple TRAX? check config for presence?:
  <device id='Trax0'   component='Component0' driver='TraxDriver0' />
  <application id='GDBStub' module='gdbstub' port='20000'>
; foreach my $i (0 .. $#cores) {
    <target device='Xtensa`$i`' />
; }
  </application>
  <application id='TraxApp' module='traxapp' port='11444'>
; # FIXME: support multiple TRAX? check config for presence?:
    <target device='Trax0' />
  </application>
</configuration>

<!--
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  -->

