//Define variables and objects
var objArgs = WScript.Arguments;
var urlString = "http://ffdshow-tryout.sourceforge.net/compmgr.php?app=" + objArgs(0) + "&rev=" + objArgs(1);
var oIE = WScript.CreateObject("InternetExplorer.Application", "IE_");

//Customize IE window
oIE.Left = 50; 
oIE.Top = 100; 
oIE.Height = 300;
oIE.Width = 380;
oIE.MenuBar = 0;
oIE.ToolBar = 0;
oIE.StatusBar = 0;

//Finally open URL and show browser window
oIE.Navigate(urlString);
oIE.Visible = true;

//Send scripting host to sleep
WScript.Sleep(10000);

//Close browser window after wscript.exe wakes up
oIE.Quit();

//OnQuit-Event gets fired when browser window gets 
//closed before (click on Close-button) or after timeout
function IE_OnQuit()
{
  WScript.Quit();
}