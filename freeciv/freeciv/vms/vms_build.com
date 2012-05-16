$!
$! VMS_BUILD.COM
$! Original Version:  Roger Tucker
$!                    roger.tucker@wcom.com
$!     Re-Writen By:  Robert Alan Byer
$!                    byer@mail.ourservers.net
$!
$!
$! This script checks the file names and library paths and then compiles
$! and links FreeCiv for OpenVMS using DEC C and DEC C TCP/IP socket routines.
$!
$!      P1      ALL             Build Everything.
$!              IMLIB		Just Build The IMLIB Library.
$!              COMMONLIB       Just Build The Library Of Common Routines.
$!		AILIB		Just Build The AI Library.
$!              SERVERLIB       Just Build The Server Library.
$!		CIVSERVER	Just Build The CIVSERVER Executable.
$!              CLIENTLIB       Just Build The Client Library.
$!		GTKCLIENTLIB	Just Build The GTK Client Library.
$!		CIVCLIENT	Just Build The CIVCLIENT Executable.
$!
$!	P2	DEBUG		Build With Debugger Information.
$!		NODEBUG		Build Withoug Debugger Information.
$!
$!      P3      PRE_DECC_V6_2   Compiler Is Pre DEC C v6.2.
$!
$!
$! The default is "ALL" and "NODEBUG".
$!
$!
$! Check To Make Sure We Have Valid Command Line Parameters.
$!
$ GOSUB CHECK_OPTIONS
$!
$! Define IMLIB_DIR To "" Since We Don't Know If We Will Need It Or Not Yet
$! Or Even Where It's Located.
$!
$ IMLIB_DIR =""
$!
$! Check To Make Sure We Have Some Necessary Files.
$!
$ GOSUB CHECK_FILES
$!
$! Check To See If We Are On An AXP Machine.
$!
$ IF (F$GETSYI("CPU").LT.128)
$ THEN
$!
$!  We Are On A VAX Machine So Tell The User.
$!
$   WRITE SYS$OUTPUT "Compiling On A VAX Machine."
$!
$!  Define The Machine Type.
$!
$   MACHINE_TYPE = "VAX"
$!
$!  Define The Compile Command For VAX.
$!
$   CC = "CC/PREFIX=ALL/''OPTIMIZE'/''DEBUGGER'/NEST=PRIMARY" + -
         "/NAME=(AS_IS,SHORTENED)"
$!
$! Else...
$!
$ ELSE
$!
$!  Check To Make Sure We Have Some Necessary Directories.
$!
$   GOSUB CHECK_DIRECTORIES
$!
$!  We Are On A AXP Machine So Tell The User.
$!
$   WRITE SYS$OUTPUT "Compiling On A AXP Machine."
$!
$!  Define The Machine Type.
$!
$   MACHINE_TYPE = "AXP"
$!
$!  Check To Make Sure The Logicals Pointing To The GTK+ and OpenVMS 
$!  Porting Libaries Are Set.
$!
$   GOSUB CHECK_LIB_LOGICALS
$!
$!  Define The Compile Command For AXP.
$!
$   CC = "CC/PREFIX=ALL/''OPTIMIZE'/''DEBUGGER'/REENTRANCY=MULTITHREAD" + -
         "/FLOAT=IEEE_FLOAT/IEEE_MODE=DENORM_RESULTS/NEST=PRIMARY" + -
         "/NAME=(AS_IS,SHORTENED)"
$!
$! End Of The Machine Check.
$!
$ ENDIF
$!
$! Define The Library Names.
$! Check To See If We Have A IMLIB_DIR.
$!
$ IF (IMLIB_DIR.NES."")
$ THEN
$!
$!  Check To See If We Are To Compile With DEBUG Information.
$!
$   IF (DEBUGGER.EQS."DEBUG")
$   THEN
$!
$!    Define the DEBUG IMLIB Library Name.
$!
$     IMLIB_NAME = IMLIB_DIR + "]IMLIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$!  Else...
$!
$   ELSE
$!
$!    Define The IMLIB Library Name.
$!
$     IMLIB_NAME = IMLIB_DIR + "]IMLIB-''MACHINE_TYPE'.OLB"
$!
$!  Time To End The DEBUG Check.
$!
$   ENDIF
$!
$! Time To End The IMLIB_DIR Check.
$!
$ ENDIF
$!
$! Check To See If We Are Going To Compile With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Define The DEBUG COMMON Library Name.
$!
$   COMMONLIB_NAME = "SYS$DISK:[-.COMMON]COMMONLIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$!  Define The DEBUG AI Library Name.
$!
$   AILIB_NAME = "SYS$DISK:[-.AI]AILIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$!  Define The DEBUG CLIENT Library.
$!
$   CLIENTLIB_NAME = "SYS$DISK:[-.CLIENT]CLIENTLIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$!  Define The DEBUG SERVER Library.
$!
$   SERVERLIB_NAME = "SYS$DISK:[-.SERVER]SERVERLIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$!  Define The DEBUG GTKCLIENT Library.
$!
$   GTKCLIENTLIB_NAME = "SYS$DISK:[-.CLIENT]GTKCLIENTLIB-''MACHINE_TYPE'.OLB-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Define The COMMON Library Name.
$!
$   COMMONLIB_NAME = "SYS$DISK:[-.COMMON]COMMONLIB-''MACHINE_TYPE'.OLB"
$!
$!  Define The AI Library Name.
$!
$   AILIB_NAME = "SYS$DISK:[-.AI]AILIB-''MACHINE_TYPE'.OLB"
$!
$!  Define The CLIENT Library.
$!
$   CLIENTLIB_NAME = "SYS$DISK:[-.CLIENT]CLIENTLIB-''MACHINE_TYPE'.OLB"
$!
$!  Define The SERVER Library.
$!
$   SERVERLIB_NAME = "SYS$DISK:[-.SERVER]SERVERLIB-''MACHINE_TYPE'.OLB"
$!
$!  Define The GTKCLIENT Library.
$!
$   GTKCLIENTLIB_NAME = "SYS$DISK:[-.CLIENT]GTKCLIENTLIB-''MACHINE_TYPE'.OLB"
$!
$! Time To End The DEBUG Library Check.
$!
$ ENDIF
$!
$! Check To See What We Are To Do.
$!
$ IF (BUILDALL.EQS."TRUE")
$ THEN
$!
$!  Build "Everything".
$!
$   GOSUB BUILD_COMMONLIB
$   GOSUB BUILD_AILIB
$   GOSUB BUILD_SERVERLIB
$   GOSUB BUILD_CIVSERVER
$!
$!  The Following For Now Will Only Build On The Alpha Platform.
$!
$   IF (MACHINE_TYPE.NES."VAX")
$   THEN
$     GOSUB BUILD_IMLIB
$     GOSUB BUILD_CLIENTLIB
$     GOSUB BUILD_GTKCLIENTLIB
$     GOSUB BUILD_CIVCLIENT
$!
$!  Time To End The (MACHINE_TYPE.NES."VAX") Check.
$!
$   ENDIF
$!
$! Else...
$!
$ ELSE
$!
$!  Build Just What The User Wants Us To Build.
$!
$   GOSUB BUILD_'BUILDALL'
$!
$! Time To Exit The Build Check.
$!
$ ENDIF
$!
$! Time To EXIT.
$!
$ EXIT
$!
$! Build The IMLIB Library.
$!
$ BUILD_IMLIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compling The ",IMLIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(''IMLIB_DIR'.GDK_IMLIB],''IMLIB_DIR'],''GTK_DIR'.GLIB],''GTK_DIR'.GLIB.GMODULE],''GTK_DIR'.GTK],''GTK_DIR'.GTK.GDK],PORTING_LIB:[INCLUDE],DECW$INCLUDE:)"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  We Are A Pre DEC C v6.2 Compiler So Set The DEFINE For It.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""PRE_DECC_V6_2=TRUE"")"
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Write A Sperator Line.
$!
$ WRITE SYS$OUTPUT ""
$!
$! Define SYS To Point To The DECWindows Include Directory.
$!
$ DEFINE/NOLOG SYS DECW$INCLUDE
$!
$! Define GTK To Point To The [.GTK.GTK] Directory.
$!
$ DEFINE/NOLOG GTK 'GTK_DIR'.GTK.GTK]
$!
$! Define GDK To Point To The [.GTK.GDK] Directory.
$!
$ DEFINE/NOLOG GDK 'GTK_DIR'.GTK.GDK]
$!
$! Check To See If We Already Have A "IMLIB" Library...
$!
$ IF (F$SEARCH(IMLIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'IMLIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Define The Files That Are Necessary For The IMLIB Library.
$!
$ IMLIB_FILES = "CACHE,COLORS,GLOBALS,LOAD,MISC,REND,UTILS,SAVE,MODULES"
$!
$!  Define A File Counter And Set It To "0".
$!
$ IMLIB_FILE_COUNTER = 0
$!
$! Top Of The File Loop.
$!
$ NEXT_IMLIB_FILE:
$!
$! O.K, Extract The File Name From The File List.
$!
$ IMLIB_FILE_NAME = F$ELEMENT(IMLIB_FILE_COUNTER,",",IMLIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (IMLIB_FILE_NAME.EQS.",") THEN GOTO IMLIB_FILE_DONE
$!
$! Increment The Counter.
$!
$ IMLIB_FILE_COUNTER = IMLIB_FILE_COUNTER + 1
$!
$! Create The Source File Name.
$!
$ IMLIB_SOURCE_FILE = IMLIB_DIR + ".GDK_IMLIB]" + IMLIB_FILE_NAME + ".C"
$!
$! Check To See If We Are Compiling With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Create The Object File Name To Reflect DEBUG Information.
$!
$   IMLIB_OBJECT_FILE = IMLIB_DIR + ".GDK_IMLIB]" + IMLIB_FILE_NAME + "-" + -
                        MACHINE_TYPE + ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   IMLIB_OBJECT_FILE = IMLIB_DIR + ".GDK_IMLIB]" + IMLIB_FILE_NAME + "-" + -
                        MACHINE_TYPE + ".OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Check To See If The File We Want To Compile Actually Exists.
$!
$ IF (F$SEARCH(IMLIB_SOURCE_FILE).EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ",IMLIB_SOURCE_FILE," Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",IMLIB_SOURCE_FILE
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
               'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK.GTK],'GTK_DIR'.GTK.GDK], -
               PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /DEFINE=("PRE_DECC_V6_2=TRUE") -
      /OBJECT='IMLIB_OBJECT_FILE' 'IMLIB_SOURCE_FILE'
$!
$! Else...
$!
$ ELSE
$!
$!  Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
               'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK.GTK],'GTK_DIR'.GTK.GDK], -
               PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /OBJECT='IMLIB_OBJECT_FILE' 'IMLIB_SOURCE_FILE'
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'IMLIB_NAME' 'IMLIB_OBJECT_FILE'
$!
$! Delete The Object File.
$!
$ DELETE/NOCONFIRM/NOLOG 'IMLIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_IMLIB_FILE
$!
$! All Done Compiling.
$!
$ IMLIB_FILE_DONE:
$!
$! Check To See If We Are Going To Compile With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Define The IMLIB_CONFIG Object File Names And Executable Name To Show A
$!  Version Compiled With DEBUG Information.
$!
$   IMLIB_CONFIG_OBJ = "''IMLIB_DIR'.UTILS]IMLIB_CONFIG-''MACHINE_TYPE'.OBJ-DEBUG"
$   IMLIB_CONFIG_EXE = "''IMLIB_DIR'.UTILS]IMLIB_CONFIG-''MACHINE_TYPE'.EXE-DEBUG"
$   ICONS_OBJ = "''IMLIB_DIR'.UTILS]ICONS-''MACHINE_TYPE'.OBJ-DEBUG"
$   TESTIMG_OBJ = "''IMLIB_DIR'.UTILS]TESTIMG-''MACHINE_TYPE'.OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Define The IMLIB_CONFIG Object File Names And Executable Name To Show A
$!  Version Compiles Without DEBUG Information.
$!
$   IMLIB_CONFIG_OBJ = "''IMLIB_DIR'.UTILS]IMLIB_CONFIG-''MACHINE_TYPE'.OBJ"
$   IMLIB_CONFIG_EXE = "''IMLIB_DIR'.UTILS]IMLIB_CONFIG-''MACHINE_TYPE'.EXE"
$   ICONS_OBJ = "''IMLIB_DIR'.UTILS]ICONS-''MACHINE_TYPE'.OBJ"
$   TESTIMG_OBJ = "''IMLIB_DIR'.UTILS]TESTIMG-''MACHINE_TYPE'.OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Tell The User We Are Compiling The IMLIB_CONFIG Executable.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Building ''IMLIB_CONFIG_EXE'."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(''IMLIB_DIR'.GDK_IMLIB],''IMLIB_DIR'],''GTK_DIR'.GLIB],''GTK_DIR'.GLIB.GMODULE],''GTK_DIR'.GTK],''GTK_DIR'.GTK.GDK],PORTING_LIB:[INCLUDE],DECW$INCLUDE:)"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  We Are A Pre DEC C v6.2 Compiler So Set The DEFINE For It.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""PRE_DECC_V6_2=TRUE"")"
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Write A Seperator Line.
$!
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If The File [.IMLIB.UTILS]IMLIB_CONFIG.C Actually Exists.
$!
$ IF (F$SEARCH("''IMLIB_DIR'.UTILS]IMLIB_CONFIG.C").EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ''IMLIB_DIR'.UTILS]IMLIB_CONFIG.C Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	''IMLIB_DIR'.UTILS]IMLIB_CONFIG.C"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], -
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /DEFINE=("PRE_DECC_V6_2=TRUE") -
      /OBJECT='IMLIB_CONFIG_OBJ' 'IMLIB_DIR'.UTILS]IMLIB_CONFIG.C
$!
$! Else...
$!
$ ELSE
$!
$!  Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], -
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /OBJECT='IMLIB_CONFIG_OBJ' 'IMLIB_DIR'.UTILS]IMLIB_CONFIG.C
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Check To See If The File [.IMLIB.UTILS]ICONS.C Actually Exists.
$!
$ IF (F$SEARCH("''IMLIB_DIR'.UTILS]ICONS.C").EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ''IMLIB_DIR'.UTILS]ICONS.C Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	''IMLIB_DIR'.UTILS]ICONS.C"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!   Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], - 
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /DEFINE=("PRE_DECC_V6_2=TRUE") -
      /OBJECT='ICONS_OBJ' 'IMLIB_DIR'.UTILS]ICONS.C
$!
$! Else...
$!
$ ELSE
$!
$!   Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], - 
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /OBJECT='ICONS_OBJ' 'IMLIB_DIR'.UTILS]ICONS.C
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Check To See If The File [.IMLIB.UTILS]TESTIMG.C Actually Exists.
$!
$ IF (F$SEARCH("''IMLIB_DIR'.UTILS]TESTIMG.C").EQS."")
$ THEN
$!
$!  Tell The User That The File Dosen't Exist.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ''IMLIB_DIR'.UTILS]TESTIMG.C Dosen't Exist."
$   WRITE SYS$OUTPUT ""
$!
$!  Exit The Build.
$!
$   EXIT
$!
$! End The File Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	''IMLIB_DIR'.UTILS]TESTIMG.C"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], -
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /DEFINE=("PRE_DECC_V6_2=TRUE") -
      /OBJECT='TESTIMG_OBJ' 'IMLIB_DIR'.UTILS]TESTIMG.C
$!
$! Else...
$!
$ ELSE
$!
$!  Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=('IMLIB_DIR'.GDK_IMLIB],'IMLIB_DIR'],'GTK_DIR'.GLIB], -
                'GTK_DIR'.GLIB.GMODULE],'GTK_DIR'.GTK],'GTK_DIR'.GDK], -
                PORTING_LIB:[INCLUDE],DECW$INCLUDE:) -
      /OBJECT='TESTIMG_OBJ' 'IMLIB_DIR'.UTILS]TESTIMG.C
$!
$! Time To End The PRE_DECC_V6_2 Check..
$!
$ ENDIF
$!
$! Tell The User That We Are Linking The IMLIB_CONFIG Executable.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Linking ''IMLIB_CONFIG_EXE'."
$ WRITE SYS$OUTPUT ""
$!
$! Link The IMLIB_CONFIG Executable..
$!
$ LINK/'DEBUGGER'/'TRACEBACK'/EXE='IMLIB_CONFIG_EXE' -
      'IMLIB_CONFIG_OBJ','ICONS_OBJ','TESTIMG_OBJ', -
      SYS$INPUT:/OPTIONS,'IMLIB_NAME'/LIBRARY
$ DECK
LIBGDK/SHARE
LIBGTK/SHARE
LIBGLIB/SHARE
VMS_JACKETS/SHARE
SYS$SHARE:DECW$XLIBSHR.EXE/SHARE
SYS$SHARE:DECW$XEXTLIBSHR.EXE/SHARE
$ EOD
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The COMMON Library.
$!
$ BUILD_COMMONLIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compling The ",COMMONLIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.COMMON])/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If We Already Have A "COMMONLIB" Library...
$!
$ IF (F$SEARCH(COMMONLIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'COMMONLIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Top Of The File Loop.
$!
$ NEXT_COMMONLIB_FILE:
$!
$! Define The Files That Are Necessary For The COMMONLIB Library.
$!
$ COMMONLIB_FILES = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.COMMON]*.C",1)))
$!
$! O.K, Extract The File Name From The File List.
$!
$ COMMONLIB_FILE_NAME = F$ELEMENT(0,".",COMMONLIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (COMMONLIB_FILE_NAME.EQS."]") THEN GOTO COMMONLIB_FILE_DONE
$!
$! Create The Source File Name.
$!
$ COMMONLIB_SOURCE_FILE = "SYS$DISK:[-.COMMON]" + COMMONLIB_FILE_NAME + ".C"
$!
$! Check To See If We Are Compiling With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$! Create The Object File Name To Reflect DEBUG Information.
$!
$   COMMONLIB_OBJECT_FILE = "SYS$DISK:[-.COMMON]" + COMMONLIB_FILE_NAME + -
                            "-" + MACHINE_TYPE + ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   COMMONLIB_OBJECT_FILE = "SYS$DISK:[-.COMMON]" + COMMONLIB_FILE_NAME + -
                            "-" + MACHINE_TYPE + ".OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",COMMONLIB_SOURCE_FILE
$!
$! Compile The File.
$!
$ CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.COMMON]) -
    /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE", "_''MACHINE_TYPE'_=TRUE") -
    /OBJECT='COMMONLIB_OBJECT_FILE' 'COMMONLIB_SOURCE_FILE'
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'COMMONLIB_NAME' 'COMMONLIB_OBJECT_FILE'
$!
$! Delete The Object File.
$!
$ DELETE/NOCONFIRM/NOLOG 'COMMONLIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_COMMONLIB_FILE
$!
$! All Done Compiling.
$!
$ COMMONLIB_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The AI Library.
$!
$ BUILD_AILIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compling The ",AILIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.AI],SYS$DISK:[-.COMMON],SYS$DISK:[-.SERVER])/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If We Already Have A "AILIB" Library...
$!
$ IF (F$SEARCH(AILIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'AILIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Top Of The File Loop.
$!
$ NEXT_AILIB_FILE:
$!
$! Define The Files That Are Necessary For The AILIB Library.
$!
$ AILIB_FILES = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.AI]*.C",1)))
$!
$! O.K, Extract The File Name From The File List.
$!
$ AILIB_FILE_NAME = F$ELEMENT(0,".",AILIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (AILIB_FILE_NAME.EQS."]") THEN GOTO AILIB_FILE_DONE
$!
$! Create The Source File Name.
$!
$ AILIB_SOURCE_FILE = "SYS$DISK:[-.AI]" + AILIB_FILE_NAME + ".C"
$!
$! Check To See If We Are Compiling With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Create The Object File Name To Reflect DEBUG Information.
$!
$   AILIB_OBJECT_FILE = "SYS$DISK:[-.AI]" + AILIB_FILE_NAME + "-" + -
                        MACHINE_TYPE + ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   AILIB_OBJECT_FILE = "SYS$DISK:[-.AI]" + AILIB_FILE_NAME + "-" + -
                        MACHINE_TYPE + ".OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",AILIB_SOURCE_FILE
$!
$! Compile The File.
$!
$ CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.AI],SYS$DISK:[-.COMMON], -
              SYS$DISK:[-.SERVER]) -
    /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
    /OBJECT='AILIB_OBJECT_FILE' 'AILIB_SOURCE_FILE'
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'AILIB_NAME' 'AILIB_OBJECT_FILE'
$!
$! Delete The Object File.
$!
$ DELETE/NOCONFIRM/NOLOG 'AILIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_AILIB_FILE
$!
$! All Done Compiling.
$!
$ AILIB_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The Server Library.
$!
$ BUILD_SERVERLIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compling The ",SERVERLIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.SERVER],SYS$DISK:[-.COMMON],SYS$DISK:[-.AI])/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If We Already Have A "SERVERLIB" Library...
$!
$ IF (F$SEARCH(SERVERLIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'SERVERLIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Top Of The File Loop.
$!
$ NEXT_SERVERLIB_FILE:
$!
$! Define The Files That Are Necessary For The SERVERLIB Library.
$!
$ SERVERLIB_FILES = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.SERVER]*.C",1)))
$!
$! O.K, Extract The File Name From The File List.
$!
$ SERVERLIB_FILE_NAME = F$ELEMENT(0,".",SERVERLIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (SERVERLIB_FILE_NAME.EQS."]") THEN GOTO SERVERLIB_FILE_DONE
$!
$! Check To See If We Are Going To Compile CIVSERVER.  If We Are, Skip
$! It And Go To The Next File As We Will Compile It Later.
$!
$ IF (SERVERLIB_FILE_NAME.EQS."CIVSERVER") THEN GOTO NEXT_SERVERLIB_FILE
$!
$! Create The Source File Name.
$!
$ SERVERLIB_SOURCE_FILE = "SYS$DISK:[-.SERVER]" + SERVERLIB_FILE_NAME + ".C"
$!
$! Check To See If We Are To Compile With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Create The Object File Name To Reflect DEBUG Information.
$!
$   SERVERLIB_OBJECT_FILE = "SYS$DISK:[-.SERVER]" + SERVERLIB_FILE_NAME + -
                           "-" + MACHINE_TYPE + ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   SERVERLIB_OBJECT_FILE = "SYS$DISK:[-.SERVER]" + SERVERLIB_FILE_NAME + -
                           "-" + MACHINE_TYPE + ".OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",SERVERLIB_SOURCE_FILE
$!
$! Compile The File.
$!
$ CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.SERVER],SYS$DISK:[-.COMMON], -
              SYS$DISK:[-.AI]) -
    /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
    /OBJECT='SERVERLIB_OBJECT_FILE' 'SERVERLIB_SOURCE_FILE'
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'SERVERLIB_NAME' 'SERVERLIB_OBJECT_FILE'
$!
$! Delete The Object File.
$!
$ DELETE/NOCONFIRM/NOLOG 'SERVERLIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_SERVERLIB_FILE
$!
$! All Done Compiling.
$!
$ SERVERLIB_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The CIVSERVER Executable:
$!
$ BUILD_CIVSERVER:
$!
$! Check To See If We Have The "SERVERLIB" Library...
$!
$ IF (F$SEARCH(SERVERLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "SERVERLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",SERVERLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The CIVSERVER Executable We Will Compile It Now."
$!
$!  Build The "SERVERLIB" Library.
$!
$   GOSUB BUILD_SERVERLIB
$!
$! End The SERVERLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "COMMONLIB" Library...
$!
$ IF (F$SEARCH(COMMONLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "COMMONLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",COMMONLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The CIVSERVER Executable We Will Compile It Now."
$!
$!  Build The "COMMONLIB" Library.
$!
$   GOSUB BUILD_COMMONLIB
$!
$! End The COMMONLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "AILIB" Library...
$!
$ IF (F$SEARCH(AILIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "AILIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",AILIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The CIVSERVER Executable We Will Compile It Now."
$!
$!  Build The "AILIB" Library.
$!
$   GOSUB BUILD_AILIB
$!
$! End The AILIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "CIVSERVER.C" File...
$!
$ IF (F$SEARCH("SYS$DISK:[-.SERVER]CIVSERVER.C").EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "CIVSERVER.C" File, So We Are Gong To
$!  Exit As We Can't Comple Without It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The SYS$DISK:[-.SERVER]CIVSERVER.C File.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The CIVSERVER Executable We Will Exit Now."
$!
$!  Since We Can't Compile Without It, Time To Exit.
$!
$   EXIT
$!
$! End The SYS$DISK:[-.SERVER]CIVSERVER.C File Check.
$!
$ ENDIF
$!
$! Check To See If We Are Going To Compile With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Define The CIVSERVER Object File Name And Executable Name To Show A
$!  Version Compiled With DEBUG Information.
$!
$   CIVSERVER_OBJ = "SYS$DISK:[-.SERVER]CIVSERVER-''MACHINE_TYPE'.OBJ-DEBUG"
$   CIVSERVER_EXE = "SYS$DISK:[]CIVSERVER-''MACHINE_TYPE.EXE-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Define The CIVSERVER Object File Nane And Executable Name To Show A
$!  Version Compiles Without DEBUG Information.
$!
$   CIVSERVER_OBJ = "SYS$DISK:[-.SERVER]CIVSERVER-''MACHINE_TYPE'.OBJ"
$   CIVSERVER_EXE = "SYS$DISK:[]CIVSERVER-''MACHINE_TYPE.EXE"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$!  Tell The User We Are Building CIVSERVER.
$!
$!
$! Well, Since It Looks Like We Have Everything, Tell The User What We
$! Are Going To Do.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Building ''CIVSERVER_EXE'."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.SERVER],SYS$DISK:[-.COMMON],SYS$DISK:[-.AI])/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$ WRITE SYS$OUTPUT ""
$!
$! Compile The File.
$!
$ CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.SERVER],SYS$DISK:[-.COMMON], - 
              SYS$DISK:[-.AI]) -
    /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
    /OBJECT='CIVSERVER_OBJ' SYS$DISK:[-.SERVER]CIVSERVER.C
$!
$! Tell The User We Are Linking The CIVSERVER.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Linking ''CIVSERVER_EXE'."
$ WRITE SYS$OUTPUT ""
$!
$! Link The Executable.
$!
$ LINK/'DEBUGGER'/'TRACEBACK'/EXE='CIVSERVER_EXE' 'CIVSERVER_OBJ', -
      'SERVERLIB_NAME'/LIBRARY,'COMMONLIB_NAME'/LIBRARY,'AILIB_NAME'/LIBRARY
$!
$! That's All, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The GTK Client Library.
$!
$ BUILD_GTKCLIENTLIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compling The ",GTKCLIENTLIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT.GUI-GTK],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE],SYS$DISK:[-.COMMON],''GTK_DIR'.GLIB],''IMLIB_DIR'.GDK_IMLIB],''JACKETS_DIR'.INCLUDE])"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  We Are So Set A Define For Pre DEC C v6.2.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"",""PRE_DECC_V6_2=TRUE"")"
$!
$! Else...
$!
$ ELSE
$!
$!  We Arn't So Don't Set A Define For PRE DECC v6.2.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Write A Seperator Line.
$!
$ WRITE SYS$OUTPUT ""
$!
$! Define GTK To Point To The [.GTK.GTK] Directory.
$!
$ DEFINE/NOLOG GTK 'GTK_DIR'.GTK.GTK]
$!
$! Define GDK To Point To The [.GTK.GDK] Directory.
$!
$ DEFINE/NOLOG GDK 'GTK_DIR'.GTK.GDK]
$!
$! Check To See If We Have SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H
$!
$ IF (F$SEARCH("SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H").EQS."")
$ THEN
$!
$!  We Don't Have SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H So Copy
$!  SYS$DISK:[]FREECIV_H.VMS To SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H
$!
$   COPY SYS$DISK:[]FREECIV_H.VMS SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H
$!
$! Time To End The SYS$DISK:[-.CLIENT.GUI-GTK]FREECIV.H File Check.
$!
$ ENDIF
$!
$! Check To See If We Already Have A "GTKCLIENTLIB" Library...
$!
$ IF (F$SEARCH(GTKCLIENTLIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'GTKCLIENTLIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Top Of The File Loop.
$!
$ NEXT_GTKCLIENTLIB_FILE:
$!
$! Define The Files That Are Necessary For The GTKCLIENTLIB Library.
$!
$ GTKCLIENTLIB_FILES = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.CLIENT.GUI-GTK]*.C",1)))
$!
$! O.K, Extract The File Name From The File List.
$!
$ GTKCLIENTLIB_FILE_NAME = F$ELEMENT(0,".",GTKCLIENTLIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (GTKCLIENTLIB_FILE_NAME.EQS."]") THEN GOTO GTKCLIENTLIB_FILE_DONE
$!
$! Create The Source File Name.
$!
$ GTKCLIENTLIB_SOURCE_FILE = "SYS$DISK:[-.CLIENT.GUI-GTK]" + -
                              GTKCLIENTLIB_FILE_NAME + ".C"
$!
$! Check To See If We Are Compiling With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Create The Object File Name To Reflect DEBUG Information.
$!
$   GTKCLIENTLIB_OBJECT_FILE = "SYS$DISK:[-.CLIENT.GUI-GTK]" + -
                               GTKCLIENTLIB_FILE_NAME + "-" + MACHINE_TYPE + -
                               ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   GTKCLIENTLIB_OBJECT_FILE = "SYS$DISK:[-.CLIENT.GUI-GTK]" + -
                               GTKCLIENTLIB_FILE_NAME + "-" + MACHINE_TYPE + -
                               ".OBJ"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",GTKCLIENTLIB_SOURCE_FILE
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$! Compile The File The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT.GUI-GTK],SYS$DISK:[-.CLIENT], -
                SYS$DISK:[-.CLIENT.INCLUDE],SYS$DISK:[-.COMMON], -
                'GTK_DIR'.GLIB],'IMLIB_DIR'.GDK_IMLIB],'JACKETS_DIR'.INCLUDE]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE", -
               "PRE_DECC_V6_2=TRUE") -
      /OBJECT='GTKCLIENTLIB_OBJECT_FILE' 'GTKCLIENTLIB_SOURCE_FILE'
$!
$! Else...
$!
$ ELSE
$!
$! Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT.GUI-GTK],SYS$DISK:[-.CLIENT], -
                SYS$DISK:[-.CLIENT.INCLUDE],SYS$DISK:[-.COMMON], -
                'GTK_DIR'.GLIB],'IMLIB_DIR'.GDK_IMLIB],'JACKETS_DIR'.INCLUDE]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
      /OBJECT='GTKCLIENTLIB_OBJECT_FILE' 'GTKCLIENTLIB_SOURCE_FILE'
$!
$! Time To End The PRE_DEC_V6_2 Check.
$!
$ ENDIF
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'GTKCLIENTLIB_NAME' 'GTKCLIENTLIB_OBJECT_FILE'
$!
$! Delete The Object Files.
$!
$ DELETE/NOCONFIRM/NOLOG 'GTKCLIENTLIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_GTKCLIENTLIB_FILE
$!
$! All Done Compiling.
$!
$ GTKCLIENTLIB_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The Client Library.
$!
$ BUILD_CLIENTLIB:
$!
$! Tell The User What We Are Doing.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Compilng The ",CLIENTLIB_NAME," Library."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE],SYS$DISK:[-.COMMON],''GTK_DIR'.GLIB],''IMLIB_DIR'.GDK_IMLIB],PORTING_LIB:[INCLUDE])"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Set The Define For Pre DEC C v6.2 Support.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"",""PRE_DECC_V6_2=TRUE"")"
$!
$! Else...
$!
$ ELSE
$!
$!  Don't Set The Define For Pre DEC C v6.2 Support.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$ ENDIF
$!
$! Write A Seperator Line.
$!
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If We Already Have A "CLIENTLIB" Library...
$!
$ IF (F$SEARCH(CLIENTLIB_NAME).EQS."")
$ THEN
$!
$!  Guess Not, Create The Library.
$!
$   LIBRARY/CREATE/OBJECT 'CLIENTLIB_NAME'
$!
$! End The Library Check.
$!
$ ENDIF
$!
$! Top Of The File Loop.
$!
$ NEXT_CLIENTLIB_FILE:
$!
$! Define The Files That Are Necessary For The CLIENTLIB Library.
$!
$ CLIENTLIB_FILES = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.CLIENT]*.C",1)))
$!
$! O.K, Extract The File Name From The File List.
$!
$ CLIENTLIB_FILE_NAME = F$ELEMENT(0,".",CLIENTLIB_FILES)
$!
$! Check To See If We Are At The End Of The File List.
$!
$ IF (CLIENTLIB_FILE_NAME.EQS."]") THEN GOTO CLIENTLIB_FILE_DONE
$!
$! Check To See If We Are Going To Compile CIVCLIENT.  If We Are, Skip
$! It And Go To The Next File As We Will Compile It Later.
$!
$ IF (CLIENTLIB_FILE_NAME.EQS."CIVCLIENT") THEN GOTO NEXT_CLIENTLIB_FILE
$!
$! Create The Source File Name.
$!
$ CLIENTLIB_SOURCE_FILE = "SYS$DISK:[-.CLIENT]" + CLIENTLIB_FILE_NAME + ".C"
$!
$! Check To See If We Are Compiling With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Create The Object File Name To Reflect DEBUG Information.
$!
$   CLIENTLIB_OBJECT_FILE = "SYS$DISK:[-.CLIENT]" + CLIENTLIB_FILE_NAME + -
                            "-" + MACHINE_TYPE + ".OBJ-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Create The Object File Name.
$!
$   CLIENTLIB_OBJECT_FILE = "SYS$DISK:[-.CLIENT]" + CLIENTLIB_FILE_NAME + -
                            "-" + MACHINE_TYPE + ".OBJ"
$!
$! Time To End The DEBUG Check...
$!
$ ENDIF
$!
$! Tell The User What We Are Compiling.
$!
$ WRITE SYS$OUTPUT "	",CLIENTLIB_SOURCE_FILE
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE], -
                SYS$DISK:[-.COMMON],'GTK_DIR'.GLIB],'IMLIB_DIR'.GDK_IMLIB], -
                PORTING_LIB:[INCLUDE]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE", -
               "PRE_DECC_V6_2=TRUE") -
      /OBJECT='CLIENTLIB_OBJECT_FILE' 'CLIENTLIB_SOURCE_FILE'
$!
$! Else...
$!
$ ELSE
$!
$!  Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE], -
                SYS$DISK:[-.COMMON],'GTK_DIR'.GLIB],'IMLIB_DIR'.GDK_IMLIB], -
                PORTING_LIB:[INCLUDE]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
      /OBJECT='CLIENTLIB_OBJECT_FILE' 'CLIENTLIB_SOURCE_FILE'
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Add It To The Library.
$!
$ LIBRARY/REPLACE/OBJECT 'CLIENTLIB_NAME' 'CLIENTLIB_OBJECT_FILE'
$!
$! Delete The Object File.
$!
$ DELETE/NOCONFIRM/NOLOG 'CLIENTLIB_OBJECT_FILE';*
$!
$! Go Back And Do It Again.
$!
$ GOTO NEXT_CLIENTLIB_FILE
$!
$! All Done Compiling.
$!
$ CLIENTLIB_FILE_DONE:
$!
$! That's It, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Build The CIVCLIENT Executable:
$!
$ BUILD_CIVCLIENT:
$!
$! Check To See If We Have The "IMLIB" Library...
$!
$ IF (F$SEARCH(IMLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "IMLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",IMLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The GTK CIVCLIENT Executable We Will Compile It Now."
$!
$!  Build The "IMLIB" Library.
$!
$   GOSUB BUILD_IMLIB
$!
$! End The IMLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "COMMONLIB" Library...
$!
$ IF (F$SEARCH(COMMONLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "COMMONLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",COMMONLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The GTK CIVCLIENT Executable We Will Compile It Now."
$!
$!  Build The "COMMONLIB" Library.
$!
$   GOSUB BUILD_COMMONLIB
$!
$! End The COMMONLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "CLIENTLIB" Library...
$!
$ IF (F$SEARCH(CLIENTLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "CLIENTLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",CLIENTLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The GTK CIVCLIENT Executable We Will Compile It Now."
$!
$!  Build The "CLIENTLIB" Library.
$!
$   GOSUB BUILD_CLIENTLIB
$!
$! End The CLIENTLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "GTKCLIENTLIB" Library...
$!
$ IF (F$SEARCH(GTKCLIENTLIB_NAME).EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "GTKCLIENTLIB" Library, So We Are Gong To
$!  Just Create It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The ",GTKCLIENTLIB_NAME," Library.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The GTK CIVCLIENT Executable We Will Compile It Now."
$!
$!  Build The "GTKCLIENTLIB" Library.
$!
$   GOSUB BUILD_GTKCLIENTLIB
$!
$! End The GTKCLIENTLIB Library Check.
$!
$ ENDIF
$!
$! Check To See If We Have The "CIVCLIENT.C" File...
$!
$ IF (F$SEARCH("SYS$DISK:[-.CLIENT]CIVCLIENT.C").EQS."")
$ THEN
$!
$!  Tell The User We Can't Find The "CIVCLIENT.C" File, So We Are Gong To
$!  Exit As We Can't Comple Without It.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Can't Find The SYS$DISK:[-.CLIENT]CIVCLIENT.C File.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Build The GTK CIVCLIENT Executable We Will Exit Now."
$!
$!  Since We Can't Compile Without It, Time To Exit.
$!
$   EXIT
$!
$! End The SYS$DISK:[-.CLIENT]CIVCLIENT.C File Check.
$!
$ ENDIF
$!
$! Check To See If We Are Going To Compile With DEBUG Information.
$!
$ IF (DEBUGGER.EQS."DEBUG")
$ THEN
$!
$!  Define The CIVCLIENT Object File Name And Executable Name To Show A
$!  Version Compiled With DEBUG Information.
$!
$   CIVCLIENT_OBJ = "SYS$DISK:[-.CLIENT]CIVCLIENT-''MACHINE_TYPE'.OBJ-DEBUG"
$   CIVCLIENT_EXE = "SYS$DISK:[]CIVCLIENT-''MACHINE_TYPE'.EXE-DEBUG"
$!
$! Else...
$!
$ ELSE
$!
$!  Define The CIVCLIENT Object File Nane And Executable Name To Show A
$!  Version Compiles Without DEBUG Information.
$!
$   CIVCLIENT_OBJ = "SYS$DISK:[-.CLIENT]CIVCLIENT-''MACHINE_TYPE'.OBJ"
$   CIVCLIENT_EXE = "SYS$DISK:[]CIVCLIENT-''MACHINE_TYPE'.EXE"
$!
$! Time To End The DEBUG Check.
$!
$ ENDIF
$!
$! Well, Since It Looks Like We Have Everything, Tell The User What We
$! Are Going To Do.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Building ''CIVCLIENT_EXE'."
$ WRITE SYS$OUTPUT "Using Compile Command: ",CC,"/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.SERVER],SYS$DISK:[-.COMMON],SYS$DISK:[-.AI])"
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Set The Define For Pre DEC C v6.2 Support.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"",""PRE_DECC_V6_2=TRUE"")"
$ ELSE
$!
$!  Don't Set The Define For Pre DEC C v6.2 Support.
$!
$   WRITE SYS$OUTPUT "/DEFINE=(""HAVE_CONFIG_H=TRUE"",""DEBUG=TRUE"",""_''MACHINE_TYPE'_=TRUE"")"
$!
$! Time To End The PRE_DECC_V6_2 Check
$!
$ ENDIF
$!
$! Write A Seperator Line.
$!
$ WRITE SYS$OUTPUT ""
$!
$! Check To See If We Are A Pre DEC C v6.2 Compiler.
$!
$ IF (P3.EQS."PRE_DECC_V6_2")
$ THEN
$!
$!  Compile The File With Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE], -
                SYS$DISK:[-.COMMON]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE", -
               "PRE_DECC_V6_2=TRUE") -
      /OBJECT='CIVCLIENT_OBJ' SYS$DISK:[-.CLIENT]CIVCLIENT.C
$!
$! Else...
$!
$ ELSE
$!
$!  Compile The File Without Pre DEC C v6.2 Support.
$!
$   CC/INCLUDE=(SYS$DISK:[-],SYS$DISK:[-.CLIENT],SYS$DISK:[-.CLIENT.INCLUDE], -
                SYS$DISK:[-.COMMON]) -
      /DEFINE=("HAVE_CONFIG_H=TRUE","DEBUG=TRUE","_''MACHINE_TYPE'_=TRUE") -
      /OBJECT='CIVCLIENT_OBJ' SYS$DISK:[-.CLIENT]CIVCLIENT.C
$!
$! Time To End The PRE_DECC_V6_2 Check.
$!
$ ENDIF
$!
$! Tell The User We Are Linking The Executable.
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Linking ''CIVCLIENT_EXE'."
$ WRITE SYS$OUTPUT ""
$!
$! Link The CIVCLIENT Executable.
$!
$ LINK/'DEBUGGER'/'TRACEBACK'/EXE='CIVCLIENT_EXE' 'CIVCLIENT_OBJ', -
      'GTKCLIENTLIB_NAME'/LIBRARY,'CLIENTLIB_NAME'/LIBRARY, -
      'COMMONLIB_NAME'/LIBRARY,SYS$INPUT:/OPTIONS,'IMLIB_NAME'/LIBRARY
$ DECK
LIBGDK/SHARE
LIBGTK/SHARE
LIBGLIB/SHARE
VMS_JACKETS/SHARE
SYS$SHARE:DECW$XLIBSHR.EXE/SHARE
SYS$SHARE:DECW$XEXTLIBSHR.EXE/SHARE
$ EOD
$!
$! That's All, Time To Return From Where We Came From.
$!
$ RETURN
$!
$! Check The User's Options.
$!
$ CHECK_OPTIONS:
$!
$! Check To See If We Are To "Just Build Everything."
$!
$ IF (P1.EQS."").OR.(P1.EQS."ALL")
$ THEN
$!
$!  P1 Is Blank Or "ALL", So Just Build Everything.
$!
$   BUILDALL = "TRUE"
$!
$! Else...
$!
$ ELSE
$!
$!  Check To See If P1 Has A Valid Arguement.
$!
$   IF (P1.EQS."COMMONLIB").OR.(P1.EQS."AILIB").OR.(P1.EQS."SERVERLIB").OR. -
       (P1.EQS."CIVSERVER").OR.(P1.EQS."CLIENTLIB").OR.(P1.EQS."IMLIB").OR. -
       (P1.EQS."GTKCLIENTLIB").OR.(P1.EQS."CIVCLIENT")
$   THEN
$!
$!    Check To See If We Are On A VAX.
$!
$     IF (F$GETSYI("CPU").LT.128)
$     THEN
$!
$!      We Are On A VAX, Check To See If The Options The User Entered
$!      Are Valib For VAX.
$!
$       IF (P1.EQS."CLIENTLIB").OR.(P1.EQS."GTKCLIENTLIB").OR. -
           (P1.EQS."CIVCLIENT").OR.(P1.EQS."IMLIB")
$       THEN
$!
$!        The User Entered An Option Invalid For OpenVMS VAX, So Tell Them.
$!
$         WRITE SYS$OUTPUT ""
$         WRITE SYS$OUTPUT "Sorry, for now only the CIVSERVER is available for OpenVMS VAX."
$         WRITE SYS$OUTPUT ""
$!
$!        Since They Entered An Invalid Option, Exit.
$!
$         EXIT
$!
$!      Time To End The VAX Option Check.
$!
$       ENDIF
$!
$!    Time To Exit The Valid VAX Check.
$!
$     ENDIF
$!
$!    A Valid Arguement.
$!
$     BUILDALL = P1
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User We Don't Know What They Want.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",P1," Is Invalid.  The Valid Options Are:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    ALL           :  Just Build 'Everything'."
$     WRITE SYS$OUTPUT "    IMLIB	  :  Just Build The IMLIB Library."
$     WRITE SYS$OUTPUT "    COMMONLIB     :  Just Build The Library Of Common Routines."
$     WRITE SYS$OUTPUT "    AILIB         :  Just Build The AI Library."
$     WRITE SYS$OUTPUT "    SERVERLIB     :  Just Build The Server Library."
$     WRITE SYS$OUTPUT "    CIVSERVER     :  Just Build The CIVSERVER Executable."
$     WRITE SYS$OUTPUT "    CLIENTLIB     :  Just Build The Client Library."
$     WRITE SYS$OUTPUT "    GTKCLIENTLIB  :  Just Build The GTK Client Library."
$     WRITE SYS$OUTPUT "    CIVCLIENT     :  Just Build The CIVCLIENT Executable."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To Exit.
$!
$     EXIT
$!
$!  Time To End The Valid Arguement Check.
$!
$   ENDIF
$!
$! Time To End The BUILDALL Check.
$!
$ ENDIF
$!
$! Check To See If We Are To Compile Without Debugger Information.
$!
$ IF ((P2.EQS."").OR.(P2.EQS."NODEBUG"))
$ THEN
$!
$!  P2 Is Either Blank Or "NODEBUG" So Compile Without Debugger Information.
$!
$   DEBUGGER  = "NODEBUG"
$!
$!  Check To See If We Are On An AXP Platform.
$!
$   IF (F$GETSYI("CPU").LT.128)
$   THEN
$!
$!    We Are On A VAX Machine So Use VAX Compiler Optimizations.
$!
$     OPTIMIZE = "OPTIMIZE"
$!
$!  Else...
$!
$   ELSE
$!
$!    We Are On A AXP Machine So Use AXP Compiler Optimizations.
$!
$     OPTIMIZE = "OPTIMIZE=(LEVEL=5,TUNE=HOST)/ARCH=HOST"
$!
$!  Time To End The Machine Check.
$!
$   ENDIF
$!
$!  Define The Linker Options.
$!
$   TRACEBACK = "NOTRACEBACK"
$!
$!  Tell The User What They Selected.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "No Debugger Information Will Be Produced During Compile."
$   WRITE SYS$OUTPUT "Compiling With Compiler Optimization."
$ ELSE
$!
$!  Check To See If We Are To Compile With Debugger Information.
$!
$   IF (P2.EQS."DEBUG")
$   THEN
$!
$!    Compile With Debugger Information.
$!
$     DEBUGGER  = "DEBUG"
$     OPTIMIZE = "NOOPTIMIZE"
$     TRACEBACK = "TRACEBACK"
$!
$!    Tell The User What They Selected.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "Debugger Information Will Be Produced During Compile."
$     WRITE SYS$OUTPUT "Compiling Without Compiler Optimization."
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User Entered An Invalid Option..
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",P2," Is Invalid.  The Valid Options Are:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    DEBUG    :  Compile With The Debugger Information."
$     WRITE SYS$OUTPUT "    NODEBUG  :  Compile Without The Debugger Information."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$!
$!  Time To End The DEBUG Check.
$!
$   ENDIF
$!
$! Time To End The P2 Check.
$!
$ ENDIF
$!
$! Check To See If We Are To Compile With Pre DEC C v6.2 Support.
$!
$ IF (P3.EQS."")
$ THEN
$!
$!  The We Are Not Compiling With Pre DEC C v6.2 Support, So Go Back To
$!  Where We Came From.
$!
$   RETURN
$!
$! Else...
$!
$ ELSE
$!
$!  Check To See If The User Entered A Valid Option.
$!
$   IF (P3.EQS."PRE_DECC_V6_2")
$   THEN
$!
$!    We Are Compiling With Pre DEC C v6.2 Support, So Tell The User.
$!
$     WRITE SYS$OUTPUT "Compiling With Pre DEC C v6.2 Support."
$!
$!    Time To Return From Where We Came From.
$!
$     RETURN
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User Entered An Invalid Option..
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",P3," Is Invalid.  The Valid Option Is:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    PRE_DECC_V6_2  :  Compile With Pre DEC C v6.2 Support."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$!
$!  Time To End The PRE_DECC_V6_2 Check.
$!
$   ENDIF
$!
$! Time To End The P3 Check.
$!
$ ENDIF
$!
$! Time To Return To Where We Were.
$!
$ RETURN
$!
$! Check To Make Sure We Have The GTK+ And Porting Library Logicals Set.
$!
$ CHECK_LIB_LOGICALS:
$!
$! Tell The User We Are Checking Logicals.
$!
$ WRITE SYS$OUTPUT "Checking Library Logicals."
$!
$! Check To See If The LIBGLIB Logical Is Assigned.
$!
$ IF (F$TRNLNM("LIBGLIB").EQS."")
$ THEN
$   DEFINE/NOLOG LIBGLIB 'GTK_DIR'.GLIB]LIBGLIB.EXE
$ ENDIF
$!
$! Check To See If The LIBGMODULE Logical Is Assigned.
$!
$ IF (F$TRNLNM("LIBGMODULE").EQS."")
$ THEN
$   DEFINE/NOLOG LIBGMODULE 'GTK_DIR'.GLIB.GMODULE]LIBGMODULE.EXE
$ ENDIF
$!
$! Check To See If The LIBGDK Logical Is Assigned.
$!
$ IF (F$TRNLNM("LIBGDK").EQS."")
$ THEN
$   DEFINE/NOLOG LIBGDK 'GTK_DIR'.GTK.GDK]LIBGDK.EXE
$ ENDIF
$!
$! Check To See If The LIBGTK Logical Is Assigned.
$!
$ IF (F$TRNLNM("LIBGTK").EQS."")
$ THEN
$   DEFINE/NOLOG LIBGTK 'GTK_DIR'.GTK.GTK]LIBGTK.EXE
$ ENDIF
$!
$! Check To See If The PORTING_LIB Logical Is Assigned.
$!
$ IF (F$TRNLNM("PORTING_LIB").EQS."")
$ THEN
$!
$!  Tell The User That We Need To Have The PORTING_LIB Logical Defiend.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The Logical PORTING_LIB Is Not Defined.  We Need This"
$   WRITE SYS$OUTPUT "Logical Defined For The CIVCLIENT To Compile And Link."
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The PORTING_LIB Logical Needs To Point To The Directory"
$   WRITE SYS$OUTPUT "Where You Installed Your OpenVMS Porting Library After Compiling."
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "Example: $ DEFINE/NOLOG PORTING_LIB DISK$CARTMAN:[PORTING_LIB.]"
$   WRITE SYS$OUTPUT ""
$!
$!  Since It's Needed And Not Defined, Just EXIT.
$!
$   EXIT
$!
$! Time TO End The PORTING_LIB Check.
$!
$ ENDIF
$!
$! Check To See If The VMS_JACKETS Logical Is Assigned.
$!
$ IF (F$TRNLNM("VMS_JACKETS").EQS."")
$ THEN
$   DEFINE/NOLOG VMS_JACKETS PORTING_LIB:[LIB]VMS_JACKETS.EXE
$ ENDIF
$!
$! All Done, Time To Return From Where We Game From.
$!
$ RETURN
$!
$! Check To Make Sure We Have Some Necessary Directories.
$!
$ CHECK_DIRECTORIES:
$!
$! Tell The User We Are Checking Files And Directories.
$!
$ WRITE SYS$OUTPUT "Checking Directories."
$!
$! If We Are Compiling "ALL", "CLIENTLIB", "GTKCLIENTLIB", "CIVCLIENT" and
$! "IMLIB" Then Check For The GTK and IMLIB Directories And Files.
$!
$ IF (P1.EQS."ALL").OR.(P1.EQS."CLIENTLIB").OR.(P1.EQS."GTKCLIENTLIB").OR. -
     (P1.EQS."CIVCLIENT").OR.(P1.EQS."IMLIB").OR.(BUILDALL.EQS."TRUE")
$ THEN
$!
$!  Define Where We Are Looking For GTK.
$!
$   DIR_NAME = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.-]GTK*.DIR"))) 
$!
$!  Let's See If We Have The GTK Directory.
$!
$   IF (DIR_NAME.EQS."]")
$   THEN
$!
$!    Tell The User We Didn't Find The GTK Directory And That
$!    We Are Going To Exit As We Need That To Compile.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "I Can't Seem To Find The GTK Directory In SYS$DISK:[-.-].  Since It Is"
$     WRITE SYS$OUTPUT "Needed To Compile The FreeCiv Client, This Needs To Be Resolved."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To Exit.
$!
$     EXIT
$!
$!  Else...
$!
$   ELSE
$!
$!    We Found The GTK Directoy So Make The GTK_DIR Variable.
$!
$     GTK_DIR = "SYS$DISK:[-.-." + F$ELEMENT(0,".",DIR_NAME)
$!
$!  Time To End The GTK Directory Check.
$!
$   ENDIF
$!
$!  Define Where We Are Looking For IMLIB.
$!
$   DIR_NAME = F$ELEMENT(0,";",F$ELEMENT(1,"]",F$SEARCH("SYS$DISK:[-.-]IMLIB*.DIR"))) 
$!
$!  Let's See If We Have The IMLIB Directory.
$!
$   IF (DIR_NAME.EQS."]")
$   THEN
$!
$!    Tell The User We Didn't Find The IMLIB Directory And That
$!    We Are Going To Exit As We Need That To Compile.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "I Can't Seem To Find The IMLIB Directory In SYS$DISK:[-.-].  Since It Is"
$     WRITE SYS$OUTPUT "Needed To Compile The FreeCiv Client, This Needs To Be Resolved."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To Exit.
$!
$     EXIT
$!
$!  Else...
$!
$   ELSE
$!
$!    We Found The IMLIB Directoy So Make The IMLIB_DIR Variable.
$!
$     IMLIB_DIR = "SYS$DISK:[-.-." + F$ELEMENT(0,".",DIR_NAME)
$!
$!  Time To End The IMLIB_DIR Directory Check.
$!
$   ENDIF
$!
$!  Let's See If We Have The IMLIB_CONFIG.H_VMS File.
$!
$   IF (F$SEARCH("SYS$DISK:[]IMLIB_CONFIG.H_VMS").EQS."")
$   THEN
$!
$!    Tell The User We Didn't Find The SYS$DISK:[]IMLIB_CONFIG.H_VMS File And
$!    We Are Going To Exit As We Need That To Compile.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "I Can't Seem To Find The SYS$DISK:[]IMLIB_CONFIG.H_VMS File.  Since It Is"
$     WRITE SYS$OUTPUT "Needed To Compile The FreeCiv Client, This Needs To Be Resolved."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To Exit.
$!
$     EXIT
$!
$!  Else.
$!
$   ELSE
$!
$!    Copy IMLIB_CONFIG.H_VMS To IMLIB_CONFIG.H In The IMLIB Directory.
$!
$     COPY SYS$DISK:[]IMLIB_CONFIG.H_VMS 'IMLIB_DIR']CONFIG.H
$!
$!  Time To End The SYS$DISK:[]IMLIB_CONFIG.H_VMS File Check.
$!
$   ENDIF
$!
$! Time To End The P1 Check.
$!
$ ENDIF
$!
$! All Done, Time To Return From Where We Game From.
$!
$ RETURN
$!
$! Check To Make Sure We Have Some Necessary Directories.
$!
$ CHECK_FILES:
$!
$! Tell The User We Are Checking Files And Directories.
$!
$ WRITE SYS$OUTPUT "Checking Files."
$!
$! Let's See If We Have The SYS$DISK:[]CONFIG.H_VMS File.
$!
$ IF (F$SEARCH("SYS$DISK:[]CONFIG.H_VMS").EQS."")
$ THEN
$!
$!  Tell The User We Didn't Find The SYS$DISK:[]CONFIG.H_VMS File And
$!  We Are Going To Exit As We Need That To Compile.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "I Can't Seem To Find The SYS$DISK:[]CONFIG.H_VMS File.  Since It Is"
$   WRITE SYS$OUTPUT "Needed To Compile FreeCiv This Needs To Be Resolved."
$   WRITE SYS$OUTPUT ""
$!
$!  Time To Exit.
$!
$   EXIT
$!
$! Else.
$!
$ ELSE
$!
$!  Copy SYS$DISK:[]CONFIG.H_VMS To SYS$DISK:[-]CONFIG.H
$!
$   COPY SYS$DISK:[]CONFIG.H_VMS SYS$DISK:[-]CONFIG.H
$!
$! Time To End The SYS$DISK:[]CONFIG.H_VMS File Check.
$!
$ ENDIF
$!
$! All Done, Time To Return From Where We Game From.
$!
$ RETURN
