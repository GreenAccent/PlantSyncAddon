#include "ChangeLog.hpp"

#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>

#include <windows.h>


// ---------------------------------------------------------------------------
// Helper: convert GS::UniString to UTF-8 std::string
// ---------------------------------------------------------------------------

static std::string ToUtf8 (const GS::UniString& us)
{
	if (us.IsEmpty ())
		return "";
	return std::string (us.ToCStr (0, MaxUSize, CC_UTF8).Get ());
}


// ---------------------------------------------------------------------------
// Helper: get directory from a file path
// ---------------------------------------------------------------------------

static std::string GetDirectory (const std::string& filePath)
{
	auto lastSlash = filePath.find_last_of ("/\\");
	if (lastSlash == std::string::npos)
		return ".";
	return filePath.substr (0, lastSlash);
}


// ---------------------------------------------------------------------------
// Helper: get current time as "HH:MM:SS"
// ---------------------------------------------------------------------------

static std::string GetTimeStr ()
{
	std::time_t now = std::time (nullptr);
	std::tm* lt = std::localtime (&now);
	char buf[16];
	std::strftime (buf, sizeof (buf), "%H:%M:%S", lt);
	return buf;
}


// ---------------------------------------------------------------------------
// Helper: get current date as "YYYY-MM-DD"
// ---------------------------------------------------------------------------

static std::string GetDateStr ()
{
	std::time_t now = std::time (nullptr);
	std::tm* lt = std::localtime (&now);
	char buf[16];
	std::strftime (buf, sizeof (buf), "%Y-%m-%d", lt);
	return buf;
}


// ---------------------------------------------------------------------------
// Helper: get user display string "USERNAME (COMPUTERNAME)"
// ---------------------------------------------------------------------------

static std::string GetUserDisplay ()
{
	const char* user = std::getenv ("USERNAME");
	const char* comp = std::getenv ("COMPUTERNAME");

	std::string result;
	if (user != nullptr)
		result += user;
	else
		result += "unknown";

	if (comp != nullptr) {
		result += " (";
		result += comp;
		result += ")";
	}

	return result;
}


// ---------------------------------------------------------------------------
// Helper: get the log file path for today
// ---------------------------------------------------------------------------

static std::string GetLogFilePath (const GS::UniString& xmlPath)
{
	std::string xmlDir = GetDirectory (ToUtf8 (xmlPath));
	std::string logDir = xmlDir + "\\changelog";

	// Create changelog directory if it doesn't exist
	CreateDirectoryA (logDir.c_str (), nullptr);

	return logDir + "\\" + GetDateStr () + ".txt";
}


// ---------------------------------------------------------------------------
// Helper: append a formatted entry to the log file
// ---------------------------------------------------------------------------

static void AppendToLog (const GS::UniString& xmlPath, const std::string& entry)
{
	std::string logPath = GetLogFilePath (xmlPath);

	std::ofstream file (logPath, std::ios::app);
	if (!file.is_open ())
		return;

	file << "[" << GetTimeStr () << "] " << GetUserDisplay () << "\n";
	file << entry;
	file << "\n";

	file.close ();
}


// ---------------------------------------------------------------------------
// Log an export action (project item added to XML)
// ---------------------------------------------------------------------------

void LogExport (const GS::UniString& xmlPath,
				const GS::UniString& itemId,
				const GS::UniString& itemName,
				const GS::UniString& parentId)
{
	std::string entry;
	entry += "  Export: " + ToUtf8 (itemId) + " \"" + ToUtf8 (itemName) + "\"\n";
	if (!parentId.IsEmpty ())
		entry += "  Parent: " + ToUtf8 (parentId) + "\n";

	AppendToLog (xmlPath, entry);
}


// ---------------------------------------------------------------------------
// Log a "Use Server" action (project name changed to match XML)
// ---------------------------------------------------------------------------

void LogUseServer (const GS::UniString& xmlPath,
				   const GS::UniString& itemId,
				   const GS::UniString& projectName,
				   const GS::UniString& serverName)
{
	std::string entry;
	entry += "  Use Server (resolve conflict): " + ToUtf8 (itemId) + "\n";
	entry += "  Project name: \"" + ToUtf8 (projectName) + "\"\n";
	entry += "  Server name: \"" + ToUtf8 (serverName) + "\"\n";

	AppendToLog (xmlPath, entry);
}


// ---------------------------------------------------------------------------
// Log a "Use Server ID" action (project item ID changed to match server)
// ---------------------------------------------------------------------------

void LogUseServerId (const GS::UniString& xmlPath,
					 const GS::UniString& oldId,
					 const GS::UniString& newId,
					 const GS::UniString& itemName)
{
	std::string entry;
	entry += "  Use Server ID (fix ID mismatch): \"" + ToUtf8 (itemName) + "\"\n";
	entry += "  Old ID: " + ToUtf8 (oldId) + "\n";
	entry += "  New ID: " + ToUtf8 (newId) + "\n";

	AppendToLog (xmlPath, entry);
}


// ---------------------------------------------------------------------------
// Log a "Reassign ID" action (project item ID changed to avoid collision)
// ---------------------------------------------------------------------------

void LogReassignId (const GS::UniString& xmlPath,
					const GS::UniString& oldId,
					const GS::UniString& newId,
					const GS::UniString& itemName)
{
	std::string entry;
	entry += "  Reassign ID (resolve collision): \"" + ToUtf8 (itemName) + "\"\n";
	entry += "  Old ID: " + ToUtf8 (oldId) + "\n";
	entry += "  New ID: " + ToUtf8 (newId) + "\n";

	AppendToLog (xmlPath, entry);
}


// ---------------------------------------------------------------------------
// Log a "Fix Cascade" action (batch prefix change)
// ---------------------------------------------------------------------------

void LogFixCascade (const GS::UniString& xmlPath,
					const GS::UniString& oldPrefix,
					const GS::UniString& newPrefix,
					UInt32 itemCount)
{
	char buf[16];
	snprintf (buf, sizeof (buf), "%u", itemCount);

	std::string entry;
	entry += "  Fix Cascade: " + ToUtf8 (oldPrefix) + " -> " + ToUtf8 (newPrefix) + "\n";
	entry += "  Items fixed: " + std::string (buf) + "\n";

	AppendToLog (xmlPath, entry);
}


// ---------------------------------------------------------------------------
// Log an import action (all missing items imported from XML to project)
// ---------------------------------------------------------------------------

void LogImport (const GS::UniString& xmlPath)
{
	std::string entry;
	entry += "  Import: all missing items from server XML\n";

	AppendToLog (xmlPath, entry);
}
