; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=Help
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "galaxy.h"
LastPage=0

ClassCount=15
Class1=BuddyName
Class2=CGalaxyApp
Class3=CAboutDlg
Class4=CGalaxyDlg
Class5=CListBoxST
Class6=PlayerProfile
Class7=CSkinHeaderCtrl
Class8=CSkinHorizontalScrollbar
Class9=CSkinListCtrl
Class10=CSkinVerticleScrollbar
Class11=Startup
Class12=CToolTip2
Class13=CTTListBox
Class14=UpdateDlg

ResourceCount=8
Resource1=IDD_HELPBOX
Resource2=IDD_BUDDYNAME
Resource3=IDD_UPDATER
Resource4=IDD_SETPROFILE
Resource5=IDD_STARTUP
Resource6=IDD_GALAXY_DIALOG
Resource7=IDD_ABOUTBOX
Class15=Help
Resource8=IDR_MENU1

[CLS:BuddyName]
Type=0
BaseClass=CDialog
HeaderFile=BuddyName.h
ImplementationFile=BuddyName.cpp

[CLS:CGalaxyApp]
Type=0
BaseClass=CWinApp
HeaderFile=Galaxy.h
ImplementationFile=Galaxy.cpp

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=GalaxyDlg.cpp
ImplementationFile=GalaxyDlg.cpp

[CLS:CGalaxyDlg]
Type=0
BaseClass=CDialog
HeaderFile=GalaxyDlg.h
ImplementationFile=GalaxyDlg.cpp
LastObject=CGalaxyDlg

[CLS:CListBoxST]
Type=0
BaseClass=CListBox
HeaderFile=ListBoxST.h
ImplementationFile=ListBoxST.cpp

[CLS:PlayerProfile]
Type=0
BaseClass=CDialog
HeaderFile=PlayerProfile.h
ImplementationFile=PlayerProfile.cpp
Filter=D
VirtualFilter=dWC

[CLS:CSkinHeaderCtrl]
Type=0
BaseClass=CHeaderCtrl
HeaderFile=SkinHeaderCtrl.h
ImplementationFile=SkinHeaderCtrl.cpp

[CLS:CSkinHorizontalScrollbar]
Type=0
BaseClass=CStatic
HeaderFile=SkinHorizontalScrollbar.h
ImplementationFile=SkinHorizontalScrollbar.cpp

[CLS:CSkinListCtrl]
Type=0
BaseClass=CListCtrl
HeaderFile=SkinListCtrl.h
ImplementationFile=SkinListCtrl.cpp

[CLS:CSkinVerticleScrollbar]
Type=0
BaseClass=CStatic
HeaderFile=SkinVerticleScrollbar.h
ImplementationFile=SkinVerticleScrollbar.cpp

[CLS:Startup]
Type=0
BaseClass=CDialog
HeaderFile=Startup.h
ImplementationFile=Startup.cpp

[CLS:CToolTip2]
Type=0
BaseClass=CWnd
HeaderFile=tooltip2.h
ImplementationFile=tooltip2.cpp

[CLS:CTTListBox]
Type=0
BaseClass=CListBox
HeaderFile=TTListbox.h
ImplementationFile=TTListBox.cpp

[CLS:UpdateDlg]
Type=0
BaseClass=CDialog
HeaderFile=UpdateDlg.h
ImplementationFile=UpdateDlg.cpp

[DLG:IDD_BUDDYNAME]
Type=1
Class=BuddyName
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_BUDDYNAME,edit,1350631552
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_GALAXY_DIALOG]
Type=1
Class=CGalaxyDlg
ControlCount=30
Control1=IDOK,button,1342242816
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_BUTTON1,button,1342242827
Control6=IDC_PROGRESS1,msctls_progress32,1350565888
Control7=IDC_BUTTON2,button,1342242827
Control8=IDC_EDIT3,edit,1350570112
Control9=IDC_BUTTON3,button,1342242827
Control10=IDC_EDIT1,edit,1342179328
Control11=IDC_STATIC,static,1342179331
Control12=IDC_BUTTON4,button,1342242827
Control13=IDC_BUTTON5,button,1342242827
Control14=IDC_BUTTON6,button,1342242827
Control15=IDC_STATIC,static,1342179331
Control16=IDC_STATUS2,edit,1342179328
Control17=IDC_EDIT2,edit,1342179328
Control18=IDC_STATIC,static,1342179331
Control19=IDC_ADDBUDDY,button,1342242827
Control20=IDC_DELBUDDY,button,1342242827
Control21=IDC_PLAYERSORT,button,1342275584
Control22=IDC_PINGSORT,button,1342275584
Control23=IDC_SHOWUSERS,button,1342242827
Control24=IDC_NEWS,edit,1350567936
Control25=IDC_LIST1,SysListView32,1350631425
Control26=IDC_LIST3,SysListView32,1350631425
Control27=IDC_LIST4,SysListView32,1350631425
Control28=IDC_LIST2,SysListView32,1350631425
Control29=IDC_BUDDYLIST,SysListView32,1350631425
Control30=IDC_STATIC,static,1342181902

[DLG:IDD_SETPROFILE]
Type=1
Class=PlayerProfile
ControlCount=12
Control1=IDOK,button,1342242817
Control2=IDC_PLAYERNAME,edit,1350565888
Control3=IDC_PLAYEREMAIL,edit,1350565888
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_JOINATSTARTUP,button,1342242819
Control8=IDC_GAMEPATH,edit,1350566016
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_IRCSERVER,combobox,1344340099
Control12=IDC_PRIVACY,static,1342373888

[DLG:IDD_STARTUP]
Type=1
Class=Startup
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_UPDATER]
Type=1
Class=UpdateDlg
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDC_UPDATEPROGRESS,msctls_progress32,1350565889
Control3=IDC_STATIC,static,1342308352
Control4=IDC_BUTTON1,button,1342242816

[MNU:IDR_MENU1]
Type=1
Class=?
Command1=ID_MENU_CONFIGURE
Command2=ID_SERVERTOOLS_REFRESH
Command3=ID_SERVERTOOLS_JOIN
Command4=ID_SERVERTOOLS_LAUNCHALIENARENA
Command5=ID_CHAT_JOINCHATCHANNEL
Command6=ID_CHAT_LEAVECHATCHANNEL
Command7=ID_STATS_LOOKUPSTATS
Command8=ID_MENU_HELP
Command9=ID_MENU_ABOUT
CommandCount=9

[DLG:IDD_HELPBOX]
Type=1
Class=Help
ControlCount=10
Control1=IDC_STATIC,static,1342308480
Control2=IDOK,button,1342373889
Control3=IDC_LINK1,static,1342308352
Control4=IDC_LINK2,static,1342308352
Control5=IDC_LINK3,static,1342308352
Control6=IDC_LINK4,static,1342308352
Control7=IDC_LINK5,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_LINK6,static,1342308352
Control10=IDC_LINK7,static,1342308352

[CLS:Help]
Type=0
HeaderFile=Help.h
ImplementationFile=Help.cpp
BaseClass=CDialog
Filter=D
LastObject=Help

