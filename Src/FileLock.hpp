#ifndef FILELOCK_HPP
#define FILELOCK_HPP

#include "UniString.hpp"


// ---------------------------------------------------------------------------
// Lock information read from the .lock file
// ---------------------------------------------------------------------------

struct LockInfo {
	GS::UniString user;       // "COMPUTERNAME\USERNAME"
	GS::UniString time;       // "2026-03-01 12:34:56"
	GS::UniString session;    // process ID (unique per ArchiCAD instance)
	bool locked;              // true if .lock file exists
};


// ---------------------------------------------------------------------------
// Lock file API
// ---------------------------------------------------------------------------

// Create a .lock file next to the XML. Returns false if already locked.
bool          AcquireLock   (const GS::UniString& xmlPath);

// Remove the .lock file (only if locked by us). Returns true on success.
bool          ReleaseLock   (const GS::UniString& xmlPath);

// Read .lock file contents. locked=false if file doesn't exist.
LockInfo      GetLockInfo   (const GS::UniString& xmlPath);

// Check if the current user holds the lock.
bool          IsLockedByUs  (const GS::UniString& xmlPath);

// Get "COMPUTERNAME\USERNAME" for the current session.
GS::UniString GetCurrentUser ();

// Get unique session ID (process ID) for this ArchiCAD instance.
GS::UniString GetSessionId ();


#endif // FILELOCK_HPP
