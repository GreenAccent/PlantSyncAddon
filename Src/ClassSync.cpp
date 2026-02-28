#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ClassSyncPalette.hpp"


// ---------------------------------------------------------------------------
// Palette enable flags: which ArchiCAD views allow the palette
// ---------------------------------------------------------------------------

static const GSFlags kPaletteFlags =
	API_PalEnabled_FloorPlan |
	API_PalEnabled_3D        |
	API_PalEnabled_Section   |
	API_PalEnabled_Detail    |
	API_PalEnabled_Layout;


// ---------------------------------------------------------------------------
// Menu command handler: toggle palette visibility
// ---------------------------------------------------------------------------

static GSErrCode MenuCommandHandler (const API_MenuParams* menuParams)
{
	switch (menuParams->menuItemRef.itemIndex) {
		case 1:
			if (!ClassSyncPalette::HasInstance ()) {
				ClassSyncPalette::CreateInstance ();
				ClassSyncPalette::GetInstance ().Show ();
			} else {
				if (ClassSyncPalette::GetInstance ().IsVisible ())
					ClassSyncPalette::GetInstance ().Hide ();
				else
					ClassSyncPalette::GetInstance ().Show ();
			}
			break;
	}

	return NoError;
}


// ============================================================================
// Required Add-On entry points
// ============================================================================

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, 32000, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, 32000, 2, ACAPI_GetOwnResModule ());
	return APIAddon_Normal;
}


GSErrCode RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (32500, 32501, MenuCode_UserDef, MenuFlag_Default);
}


GSErrCode Initialize (void)
{
	GSErrCode err;

	err = ACAPI_MenuItem_InstallMenuHandler (32500, MenuCommandHandler);
	if (err != NoError)
		return err;

	err = ACAPI_RegisterModelessWindow (
		ClassSyncPalette::GetRefId (),
		ClassSyncPalette::PaletteControlCallback,
		kPaletteFlags,
		GSGuid2APIGuid (ClassSyncPalette::GetPaletteGuid ())
	);

	return err;
}


GSErrCode FreeData (void)
{
	if (ClassSyncPalette::HasInstance ())
		ClassSyncPalette::DestroyInstance ();

	return ACAPI_UnregisterModelessWindow (ClassSyncPalette::GetRefId ());
}
