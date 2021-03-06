/*
 Name:		MickeFTPServer.ino
 Created:	6/27/2015 7:02:48 AM
 Author:	Micke
 */

#include <EthernetUdp.h>
#include <EthernetServer.h>
#include <EthernetClient.h>
#include <Dns.h>
#include <Dhcp.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

//Enable Debugging
#define DEBUG //Turns debug output to serial com.

//Server settings
#define FTP_USER "admin"
#define FTP_PASSWORD "pass"
#define FTP_PORT 21
#define FTP_ROOT_FOLDER "/"

#define DHCP_ENABLED false
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 99); // IP address, may need to change depending on network


#define CONNECTION_TIMEOUT 30000 //The user login idle before being disconnected

// size of buffer used to capture HTTP requests
//#define REQ_BUF_SZ   50


EthernetServer ftpServer(FTP_PORT);  // create a server at port 21
EthernetClient ftpClient;
File workingDir;

//File webFile;               // the web page file on the SD card
//char HTTP_req[REQ_BUF_SZ] = { 0 }; // buffered HTTP request stored as null terminated string
//char req_index = 0;              // index into HTTP_req buffer


enum serverState
{
	noConnection,
	authenticatingUser,
	connectionEstabliched
};

enum userState
{
	newUser,
	UserId,
	Password,
	userAuthenticated
};

userState currentUserState;
serverState currentServerState;

String cmdString;
String ftpCommand;
String ftpParameter;
long sessionTimeOut;

void serialDebug(String desc = "", String val = "", String comment = "")
{
	Serial.print(desc);
	if (val != "") Serial.print(": ");
	Serial.print(val);
	if (comment != "") Serial.print(" //");
	Serial.println(comment);
	delay(500);
}

/*
// the setup function runs once when you press reset or power the board
void setup()
{
Serial.begin(9600);       // for debugging

//		 disable Ethernet chip and SD Card
pinMode(10, OUTPUT);
digitalWrite(10, HIGH);
pinMode(4, OUTPUT);
digitalWrite(4, HIGH);

delay(1000);

// initialize SD card
Serial.println("Initializing SD card...");
if (!SD.begin(4)) {
Serial.println("ERROR - SD card initialization failed!");
return;    // init failed
}

delay(1000);
//open root folder
Serial.println("Opening FTP Root folder...");
workingDir = SD.open(FTP_ROOT_FOLDER);
delay(1000);
Serial.println(workingDir.isDirectory());
if (!workingDir.isDirectory()) {
Serial.println("ERROR - Could not open FTP Root folder.");
//return;
}
Serial.println(workingDir.isDirectory());
Serial.println("SUCCESS - FTP Root folder found and opened.");




//Initalize ethernet connection
if (DHCP_ENABLED) {
Ethernet.begin(mac); //Get ip address from DHCP
}
else {
Ethernet.begin(mac, ip);  // initialize Ethernet device with Fixed IP
}
// print your local IP address:
Serial.print("My IP address: ");
for (byte thisByte = 0; thisByte < 4; thisByte++) {
// print the value of each byte of the IP address:
Serial.print(Ethernet.localIP()[thisByte], DEC);
Serial.print(".");
}
Serial.println(" ");

ftpServer.begin();           // start to listen for clients


}
*/

void setup()
{
	Serial.begin(9600);
	// initialize SD card
	Serial.println("Initializing SD card...");
	if (!SD.begin()) {
		Serial.println("ERROR - SD card initialization failed!");
		return;    // init failed
	}
	Serial.println("SUCCESS - SD card initialized.");

	//open root folder
	Serial.println("Opening FTP Root folder...");
	workingDir = SD.open(FTP_ROOT_FOLDER);
	if (!workingDir.isDirectory()) {
		Serial.println("ERROR - Could not open FTP Root folder.");
		return;
	}
	Serial.println("SUCCESS - FTP Root folder found and opened.");
}

		// the loop function runs over and over again until power down or reset
		void loop()
	{
		currentServerState = noConnection;
		//Broadcasting welcome message to make FTP clients to connect and start sending commands
		if (currentServerState == noConnection) {
			ftpServer.println("220 Welcome to MickeFTPServer");
			delay(1000);
		}
		ftpClient = ftpServer.available();  // try to get client
		if (ftpClient) {
#ifdef DEBUG
			serialDebug("Client found.");
#endif
			currentServerState = authenticatingUser;
			currentUserState = UserId;
			sessionTimeOut = millis() + CONNECTION_TIMEOUT;
		}

		while (ftpClient.connected()) {
			//Clear command and parameter strings from old info.
			ftpCommand = "";
			ftpParameter = "";
			//Read incoming commands
			readFtpCommandString();

			//Run the commands
			if (ftpCommand != "") {
				userCommands();
				serverCommands();

				testCommand();
			}
			checkTimeOut();
		}

	}

	//void userConnect() 
	//{
	//#ifdef DEBUG
	//	serialDebug("User connected");
	//#endif
	//	//Send welcome message to connected user
	//	ftpClient.println("220 Welcome to MickeFTPServer");
	//	ftpClient.println("220 This is an FTP Server for Arduino.");
	//	ftpClient.println("220 Developed by Micke");
	//	currentUserState = UserId;
	//	sessionTimeOut = millis() + CONNECTION_TIMEOUT;
	//}

	void userDisconnect()
	{
#ifdef DEBUG
		serialDebug("User disconnected");
#endif

		ftpClient.println("221 Disconnecting\r\n");
		ftpClient.stop();
	}

	void checkForUserID()
	{

	}

	void readFtpCommandString()
	{
		boolean endFound = false;
		int cmdLenght;
		while (ftpClient.available()) {
			//Serial.println("char found!");
			char c = ftpClient.read();
			//Read until until carrige return or new line is found.
			if (c == '\n' || c == '\r') {
				//Stop reading
				endFound = true;
				//cmdLenght = cmdString.length();
			}

			cmdString += c;
			sessionTimeOut = millis() + CONNECTION_TIMEOUT;
		}

		//If end is found Parse the retrived cmdString
		if (endFound) {
			//Remove inital and ending white spaces
			cmdString.trim();
			//Search for the position first space character
			int firstSpaceCharIndex = cmdString.indexOf(' ');
			//Divide cmdString into command and parameter
			ftpCommand = cmdString.substring(0, firstSpaceCharIndex);
			ftpParameter = cmdString.substring(firstSpaceCharIndex);
			ftpParameter.trim();
			//Reset cmdString
			cmdString = "";

#ifdef DEBUG
			//Sending recived command and parameter to serial com
			Serial.print("Recived CMD: ");
			Serial.print(ftpCommand);
			Serial.print(" ");
			Serial.println(ftpParameter);
#endif
		}
	}

	boolean userCommands()
	{
		boolean foundCommand = false;
		if (ftpCommand == "AUTH") {
			foundCommand = true;
			ftpClient.println("504 Security mechanism not implemented.");

			//if (ftpParameter == "TLS"){
			//	ftpClient.println("534 Server does not support TLS secure connections.");
			//}
			//else if (ftpParameter == "SSL")
			//{
			//	ftpClient.println("534 Server does not support SSL.");
			//}

		}
		else if (ftpCommand == "USER") {
			//provides the user logon.
			foundCommand = true;
			if (ftpParameter == FTP_USER) {
				ftpClient.println("331 User name okay, need password.");
				currentUserState = Password;
			}
			else {
				ftpClient.println("332 Need account for login.");
			}
		}

		else if (ftpCommand == "PASS")
			//Checks password and grants user access to all commands.
		{
			foundCommand = true;
			//Check for userID is provided
			if (currentUserState < Password) {
				ftpClient.println("503 Login with USER first.");
			}
			else {
				if (ftpParameter == FTP_PASSWORD) {
					ftpClient.println("230 User logged in.");
					currentUserState = userAuthenticated;
				}
				else {
					ftpClient.println("530 Not logged in.");
				}
			}
		}
		return foundCommand;
	}

	boolean serverCommands()
	{
		if (ftpCommand == "SYST") {
			//foundCommand = true;
			ftpClient.println("215 Arduino.");
		}

		if (ftpCommand == "FEAT") {
			//foundCommand = true;
			ftpClient.println("211-Extended features supported:");
			ftpClient.println("211 END");
		}

		if (ftpCommand == "PWD") {
			//foundCommand = true;
			ftpClient.print("257 \"");
			ftpClient.print(workingDir);
			ftpClient.print("\" is current directory.");
			Serial.println("sending working dir.");

		}
	}

	boolean testCommand()
	{

		if (ftpCommand == "TEST") {
			if (!securityCheck(userAuthenticated)) return true;
			ftpClient.println("666 Testing");
		}
	}

	boolean securityCheck(byte allowedUserState)
	{
		boolean isAllowed = false;
		if (currentUserState >= allowedUserState) {
			isAllowed = true;
		}
		else {
			ftpClient.println("530 Please login with USER and PASS.");
		}
		return(isAllowed);
	}
	void checkTimeOut()
	{
		//Checks if the current session has reached its timout value
		if (sessionTimeOut < millis()) {
			//if so closing the connection
			userDisconnect();
		}
	}

	//todo testting todo

	void ftpTestCmd()
	{

	}


	//int8_t FtpServer::readChar()
	//{
	//	int8_t rc = -1;
	//
	//	if (client.available())
	//	{
	//		char c = client.read();
	//#ifdef FTP_DEBUG
	//		Serial << c;
	//#endif
	//		if (c == '\\')
	//			c = '/';
	//		if (c != '\r')
	//			if (c != '\n')
	//			{
	//				if (iCL < FTP_CMD_SIZE)
	//					cmdLine[iCL++] = c;
	//				else
	//					rc = -2; //  Line too long
	//			}
	//			else
	//			{
	//				cmdLine[iCL] = 0;
	//				command[0] = 0;
	//				parameters = NULL;
	//				// empty line?
	//				if (iCL == 0)
	//					rc = 0;
	//				else
	//				{
	//					rc = iCL;
	//					// search for space between command and parameters
	//					parameters = strchr(cmdLine, ' ');
	//					if (parameters != NULL)
	//					{
	//						if (parameters - cmdLine > 4)
	//							rc = -2; // Syntax error
	//						else
	//						{
	//							strncpy(command, cmdLine, parameters - cmdLine);
	//							command[parameters - cmdLine] = 0;
	//
	//							while (*(++parameters) == ' ')
	//								;
	//						}
	//					}
	//					else if (strlen(cmdLine) > 4)
	//						rc = -2; // Syntax error.
	//					else
	//						strcpy(command, cmdLine);
	//					iCL = 0;
	//				}
	//			}
	//		if (rc > 0)
	//			for (uint8_t i = 0; i < strlen(command); i++)
	//				command[i] = toupper(command[i]);
	//		if (rc == -2)
	//		{
	//			iCL = 0;
	//			client << "500 Syntax error\r\n";
	//		}
	//	}
	//	return rc;
	//}




