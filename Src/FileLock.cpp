#include "FileLock.hpp"

#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <ctime>


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
// Helper: get lock file path from XML path
// ---------------------------------------------------------------------------

static std::string GetLockPath (const GS::UniString& xmlPath)
{
	return ToUtf8 (xmlPath) + ".lock";
}


// ---------------------------------------------------------------------------
// Helper: get current timestamp as string
// ---------------------------------------------------------------------------

static std::string GetTimestamp ()
{
	std::time_t now = std::time (nullptr);
	std::tm* lt = std::localtime (&now);
	char buf[32];
	std::strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", lt);
	return buf;
}


// ---------------------------------------------------------------------------
// Get "COMPUTERNAME\USERNAME" for the current session
// ---------------------------------------------------------------------------

GS::UniString GetCurrentUser ()
{
	const char* comp = std::getenv ("COMPUTERNAME");
	const char* user = std::getenv ("USERNAME");

	std::string result;
	if (comp != nullptr)
		result += comp;
	result += "\\";
	if (user != nullptr)
		result += user;

	return GS::UniString (result.c_str (), CC_UTF8);
}


// ---------------------------------------------------------------------------
// Read .lock file contents
// ---------------------------------------------------------------------------

LockInfo GetLockInfo (const GS::UniString& xmlPath)
{
	LockInfo info;
	info.locked = false;

	std::string lockPath = GetLockPath (xmlPath);
	std::ifstream file (lockPath);
	if (!file.is_open ())
		return info;

	info.locked = true;

	std::string line;
	while (std::getline (file, line)) {
		// Remove trailing \r if present
		if (!line.empty () && line.back () == '\r')
			line.pop_back ();

		if (line.compare (0, 5, "user=") == 0)
			info.user = GS::UniString (line.c_str () + 5, CC_UTF8);
		else if (line.compare (0, 5, "time=") == 0)
			info.time = GS::UniString (line.c_str () + 5, CC_UTF8);
	}

	return info;
}


// ---------------------------------------------------------------------------
// Check if the current user holds the lock
// ---------------------------------------------------------------------------

bool IsLockedByUs (const GS::UniString& xmlPath)
{
	LockInfo info = GetLockInfo (xmlPath);
	if (!info.locked)
		return false;
	return info.user == GetCurrentUser ();
}


// ---------------------------------------------------------------------------
// Create a .lock file next to the XML
// ---------------------------------------------------------------------------

bool AcquireLock (const GS::UniString& xmlPath)
{
	std::string lockPath = GetLockPath (xmlPath);

	// Check if already locked
	std::ifstream check (lockPath);
	if (check.is_open ()) {
		check.close ();
		return false;  // already locked by someone
	}

	// Create lock file
	std::ofstream file (lockPath);
	if (!file.is_open ())
		return false;

	std::string user = ToUtf8 (GetCurrentUser ());
	file << "user=" << user << "\n";
	file << "time=" << GetTimestamp () << "\n";
	file.close ();

	return true;
}


// ---------------------------------------------------------------------------
// Remove the .lock file (only if locked by us)
// ---------------------------------------------------------------------------

bool ReleaseLock (const GS::UniString& xmlPath)
{
	if (!IsLockedByUs (xmlPath))
		return false;

	std::string lockPath = GetLockPath (xmlPath);
	return std::remove (lockPath.c_str ()) == 0;
}
