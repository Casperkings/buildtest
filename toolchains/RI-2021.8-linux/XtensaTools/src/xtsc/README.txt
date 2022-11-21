      Building and Using a Custom XTSC Component Library

Note: These instructions do not apply to users of SoC Designer (a product of
      Carbon Design Systems).  If you are using SoC Designer please consult
      file README.SoCDesigner.txt instead of this file.

Note: Please keep in mind that the compiler and version used to rebuild the
      XTSC component libary must be compatible with the compiler and version
      used by Tensilica to build the XTSC core library (with which the rebuilt
      XTSC component library must ultimately be linked).

Note: Use Section I if you are on Linux or if you want to use the command
      line make on MS Windows.  Use Section II if you want to use the
      Visual Studio project files on MS Windows.

Note: Customizing the XTSC component library for use with xtsc-run is deprecated
      in RD-2012.4.  Starting in RD-2012.4, the recommended way of customizing
      xtsc-run is to write a plugin.  For more infomation on writing xtsc-run
      plugins, consult the xtsc_plugin_interface documentation in xtsc_rm.pdf and
      xtsc_ug.pdf and the simple_bus.plugin and pif2sb_bridge.plugin XTSC
      examples installed with each Tensilica processor configuration.




           Section I - Command Line Make  (Linux or MS Windows)



Part A)  Building a customized XTSC component library.

Use the following steps to build a customized version of the XTSC component library
which is called libxtsc_comp.a on Linux and xtsc_comp.lib and xtsc_compd.lib on MS Windows:

1.  Set the environment variable XTENSA_SW_TOOLS to point to <xtensa_tools_root>,
    your installation of the Xtensa tools (this is the XtensaTools directory
    two levels up from the original directory holding this README.txt file).
2.  Copy this directory and all its contents to a new location.
3.  Ensure the correct version of the compiler is on your path.  The supported compiler
    versions are listed in Chapter 2 of the XTSC User's Guide.
4.  In the new location, edit the source files as needed for your customization.
5.  In the new location, edit Makefile in accordance with the instructions in that file.
6.  Build the customized version of the XTSC component library.  To do this issue the
    make command in the new location (the make command is "make" on Linux and "xt-make"
    on MS Windows).



Part B)  Using a customized XTSC component library.

The following step shows how to use your customized version of the XTSC
component library when running one of the XTSC examples.  

Note:  These steps are for illustrative purposes only.  It is possible that
       the XTSC examples won't run with a customized version of the XTSC
       component library (of course, this depends upon what customizations
       have been done). 


1. Edit the LIBS macro in the Makefile.include file in the XTSC examples
   directory to have AS THE FIRST ENTRY the customized version of the XTSC
   component library. 





        Section II - Visual Studio Project (MS Windows only)




Part A)  Building a customized XTSC component library.

Use the following steps to build a customized version of the XTSC component
library:

1.  Set the environment variable XTENSA_SW_TOOLS to point to <xtensa_tools_root>,
    your installation of the Xtensa tools (this is the XtensaTools directory
    two levels up from the original directory holding this README.txt file).
2.  Copy this directory and all its contents to a new location.
3.  In the new location, open one of the Microsoft Visual C++ solution files:
        - xtsc_comp.vc100.sln for Visual C++ 2010
        - xtsc_comp.vc110.sln for Visual C++ 2012
4.  In the new location, edit the source files as needed for your customizations.
5.  Build your customized version of the XTSC component library for each MSVC
    solution configuration (Debug and/or Release) that you are going to use.




Part B)  Using a customized XTSC component library.

The following steps show how to use your customized version of the XTSC
component library when running one of the XTSC examples.  

Note:  These steps are for illustrative purposes only.  It is possible that
       the XTSC examples won't run with a customized version of the XTSC
       component library (of course, this depends upon what customizations
       have been done). 

Note:  The following steps will need to be done for each example project
       file.

Note:  The project file changes discussed below should be applied to each
       MSVC solution configuration (Debug and/or Release) that you are
       going to use.

Note:  The "new location" referred to in the following steps is the new
       location referred to in Part A.

1. Open the examples project file in Visual Studio.
2. Add the new location AS THE FIRST ENTRY to the "Additional Include
   Directories" line using the drop-down menu and dialog sequence:
     Project>Properties>Configuration Properties>C/C++>General>
3. Add the new location AS THE FIRST ENTRY to the "Additional Library
   Directories" line using the drop-down menu and dialog sequence:
     Project>Properties>Configuration Properties>Linker>General>


