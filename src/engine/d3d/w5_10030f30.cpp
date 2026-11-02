// *********************************************************************** //
//
//  MODULE:   W5_10030F30 (.CPP)  --  0x10030f30, the DirectDraw/Direct3D
//            HRESULT-to-string table (1968 bytes).
//
//  `mov 0x4(%esp),%eax`, `test %eax,%eax` for DD_OK, then 116
//  `cmp $<hr>,%eax ; je <tail>` in one linear chain, and a `mov $<string>,
//  %eax ; ret` tail per case.  The chain order is the source order (the
//  strings sit in .rdata in exactly that order, 0x100a6850 upwards), so the
//  source is a run of `if (hr == X) return "...";` and the tails' placement
//  after the chain is icl's /O2 block sinking.  One argument, __cdecl --
//  d3dr5b.cpp's callers declare it `const char* sub_10030f30(long)`.
//
//  The strings and constants below were read out of the shipped image
//  (tools: the je targets map each constant to its `mov $string` tail).
//  Two cases, 0x8007000e (E_OUTOFMEMORY) and 0x8876017c
//  (DDERR_OUTOFVIDEOMEMORY), return the SAME address
//  0x100a7d40 -- one literal pooled by /Gf, so two identical source
//  strings.  The value tested is compared as an HRESULT (`long`).
//
// *********************************************************************** //

const char* __cdecl sub_10030f30(long hResult)
{
	if (hResult == 0)
		return "No error.";
	if (hResult == (long)0x88760005L)
		return "This object is already initialized.";
	if (hResult == (long)0x8876023eL)
		return "Return if a clipper object is attached to the source surface passed into a BltFast call.";
	if (hResult == (long)0x8876000aL)
		return "This surface can not be attached to the requested surface.";
	if (hResult == (long)0x88760014L)
		return "This surface can not be detached from the requested surface.";
	if (hResult == (long)0x88760249L)
		return "Windows can not create any more DCs.";
	if (hResult == (long)0x88760247L)
		return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
	if (hResult == (long)0x88760237L)
		return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
	if (hResult == (long)0x88760190L)
		return "No src color key specified for this operation.";
	if (hResult == (long)0x88760028L)
		return "Support is currently not available.";
	if (hResult == (long)0x88760232L)
		return "A DirectDraw object representing this driver has already been created for this process.";
	if (hResult == (long)0x88760037L)
		return "An exception was encountered while performing the requested operation.";
	if (hResult == (long)0x88760245L)
		return "An attempt was made to set the cooperative level when it was already set to exclusive.";
	if (hResult == (long)0x80004005L)
		return "Generic failure.";
	if (hResult == (long)0x8876005aL)
		return "Height of rectangle provided is not a multiple of reqd alignment.";
	if (hResult == (long)0x8876023bL)
		return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
	if (hResult == (long)0x8876023aL)
		return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
	if (hResult == (long)0x8876024cL)
		return "This surface can not be restored because it is an implicitly created surface.";
	if (hResult == (long)0x8876005fL)
		return "Unable to match primary surface creation request with existing primary surface.";
	if (hResult == (long)0x88760064L)
		return "One or more of the caps bits passed to the callback are incorrect.";
	if (hResult == (long)0x8876006eL)
		return "DirectDraw does not support the provided cliplist.";
	if (hResult == (long)0x88760231L)
		return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
	if (hResult == (long)0x88760078L)
		return "DirectDraw does not support the requested mode.";
	if (hResult == (long)0x88760082L)
		return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
	if (hResult == (long)0x80070057L)
		return "One or more of the parameters passed to the function are incorrect.";
	if (hResult == (long)0x88760091L)
		return "The pixel format was invalid as specified.";
	if (hResult == (long)0x88760243L)
		return "Returned when the position of the overlay on the destination is no longer legal for that destination.";
	if (hResult == (long)0x88760096L)
		return "Rectangle provided was invalid.";
	if (hResult == (long)0x887600a0L)
		return "Operation could not be carried out because one or more surfaces are locked.";
	if (hResult == (long)0x887600aaL)
		return "There is no 3D present.";
	if (hResult == (long)0x887600b4L)
		return "Operation could not be carried out because there is no alpha accleration hardware present or available.";
	if (hResult == (long)0x8876023fL)
		return "No blitter hardware present.";
	if (hResult == (long)0x887600cdL)
		return "No cliplist available.";
	if (hResult == (long)0x88760238L)
		return "No clipper object attached to surface object.";
	if (hResult == (long)0x887600d2L)
		return "Operation could not be carried out because there is no color conversion hardware present or available.";
	if (hResult == (long)0x887600d7L)
		return "Surface doesn't currently have a color key";
	if (hResult == (long)0x887600dcL)
		return "Operation could not be carried out because there is no hardware support of the destination color key.";
	if (hResult == (long)0x887600d4L)
		return "Create function called without DirectDraw object method SetCooperativeLevel being called.";
	if (hResult == (long)0x8876024aL)
		return "No DC was ever created for this surface.";
	if (hResult == (long)0x88760240L)
		return "No DirectDraw ROP hardware.";
	if (hResult == (long)0x88760233L)
		return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
	if (hResult == (long)0x88760235L)
		return "Software emulation not available.";
	if (hResult == (long)0x887600e1L)
		return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
	if (hResult == (long)0x887600e6L)
		return "Flipping visible surfaces is not supported.";
	if (hResult == (long)0x887600f0L)
		return "There is no GDI present.";
	if (hResult == (long)0x88760239L)
		return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
	if (hResult == (long)0x887600faL)
		return "Operation could not be carried out because there is no hardware present or available.";
	if (hResult == (long)0x88760242L)
		return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
	if (hResult == (long)0x88760104L)
		return "Operation could not be carried out because there is no overlay hardware present or available.";
	if (hResult == (long)0x8876023cL)
		return "No palette object attached to this surface.";
	if (hResult == (long)0x8876023dL)
		return "No hardware support for 16 or 256 color palettes.";
	if (hResult == (long)0x88760118L)
		return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
	if (hResult == (long)0x88760122L)
		return "Operation could not be carried out because there is no rotation hardware present or available.";
	if (hResult == (long)0x88760136L)
		return "Operation could not be carried out because there is no hardware support for stretching.";
	if (hResult == (long)0x8876013cL)
		return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
	if (hResult == (long)0x8876013dL)
		return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
	if (hResult == (long)0x88760140L)
		return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
	if (hResult == (long)0x88760244L)
		return "Returned when an overlay member is called for a non-overlay surface.";
	if (hResult == (long)0x8876014aL)
		return "Operation could not be carried out because there is no texture mapping hardware present or available.";
	if (hResult == (long)0x88760246L)
		return "An attempt has been made to flip a surface that is not flippable.";
	if (hResult == (long)0x887600ffL)
		return "Requested item was not found.";
	if (hResult == (long)0x88760248L)
		return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
	if (hResult == (long)0x8876024dL)
		return "The surface being used is not a palette-based surface.";
	if (hResult == (long)0x8876014fL)
		return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
	if (hResult == (long)0x88760154L)
		return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
	if (hResult == (long)0x8876015eL)
		return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
	if (hResult == (long)0x88760168L)
		return "The hardware needed for the requested operation has already been allocated.";
	if (hResult == (long)0x8007000eL)
		return "DirectDraw does not have enough memory to perform the operation.";
	if (hResult == (long)0x8876017cL)
		return "DirectDraw does not have enough memory to perform the operation.";
	if (hResult == (long)0x8876017eL)
		return "The hardware does not support clipped overlays.";
	if (hResult == (long)0x88760180L)
		return "Can only have ony color key active at one time for overlays.";
	if (hResult == (long)0x88760241L)
		return "Returned when GetOverlayPosition is called on a hidden overlay.";
	if (hResult == (long)0x88760183L)
		return "Access to this palette is being refused because the palette is already locked by another thread.";
	if (hResult == (long)0x88760234L)
		return "This process already has created a primary surface.";
	if (hResult == (long)0x88760236L)
		return "Region passed to Clipper::GetClipList is too small.";
	if (hResult == (long)0x8876019aL)
		return "This surface is already attached to the surface it is being attached to.";
	if (hResult == (long)0x887601a4L)
		return "This surface is already a dependency of the surface it is being made a dependency of.";
	if (hResult == (long)0x887601aeL)
		return "Access to this surface is being refused because the surface is already locked by another thread.";
	if (hResult == (long)0x887601b8L)
		return "Access to surface refused because the surface is obscured.";
	if (hResult == (long)0x887601c2L)
		return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
	if (hResult == (long)0x887601ccL)
		return "The requested surface is not attached.";
	if (hResult == (long)0x887601d6L)
		return "Height requested by DirectDraw is too large.";
	if (hResult == (long)0x887601e0L)
		return "Size requested by DirectDraw is too large, but the individual height and width are OK.";
	if (hResult == (long)0x887601eaL)
		return "Width requested by DirectDraw is too large.";
	if (hResult == (long)0x80004001L)
		return "Action not supported.";
	if (hResult == (long)0x887601feL)
		return "FOURCC format requested is unsupported by DirectDraw.";
	if (hResult == (long)0x88760208L)
		return "Bitmask in the pixel format requested is unsupported by DirectDraw.";
	if (hResult == (long)0x88760219L)
		return "Vertical blank is in progress.";
	if (hResult == (long)0x8876021cL)
		return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
	if (hResult == (long)0x8876024bL)
		return "This surface can not be restored because it was created in a different mode.";
	if (hResult == (long)0x88760230L)
		return "Rectangle provided was not horizontally aligned on required boundary.";
	if (hResult == (long)0x887602bcL)
		return "D3DERR_BADMAJORVERSION";
	if (hResult == (long)0x887602bdL)
		return "D3DERR_BADMINORVERSION";
	if (hResult == (long)0x887602caL)
		return "D3DERR_EXECUTE_LOCKED";
	if (hResult == (long)0x887602cbL)
		return "D3DERR_EXECUTE_NOT_LOCKED";
	if (hResult == (long)0x887602c6L)
		return "D3DERR_EXECUTE_CREATE_FAILED";
	if (hResult == (long)0x887602c7L)
		return "D3DERR_EXECUTE_DESTROY_FAILED";
	if (hResult == (long)0x887602c8L)
		return "D3DERR_EXECUTE_LOCK_FAILED";
	if (hResult == (long)0x887602c9L)
		return "D3DERR_EXECUTE_UNLOCK_FAILED";
	if (hResult == (long)0x887602ccL)
		return "D3DERR_EXECUTE_FAILED";
	if (hResult == (long)0x887602cdL)
		return "D3DERR_EXECUTE_CLIPPED_FAILED";
	if (hResult == (long)0x887602d0L)
		return "D3DERR_TEXTURE_NO_SUPPORT";
	if (hResult == (long)0x887602d8L)
		return "D3DERR_TEXTURE_NOT_LOCKED";
	if (hResult == (long)0x887602d7L)
		return "D3DERR_TEXTURELOCKED";
	if (hResult == (long)0x887602d1L)
		return "D3DERR_TEXTURE_CREATE_FAILED";
	if (hResult == (long)0x887602d2L)
		return "D3DERR_TEXTURE_DESTROY_FAILED";
	if (hResult == (long)0x887602d3L)
		return "D3DERR_TEXTURE_LOCK_FAILED";
	if (hResult == (long)0x887602d4L)
		return "D3DERR_TEXTURE_UNLOCK_FAILED";
	if (hResult == (long)0x887602d5L)
		return "D3DERR_TEXTURE_LOAD_FAILED";
	if (hResult == (long)0x887602daL)
		return "D3DERR_MATRIX_CREATE_FAILED";
	if (hResult == (long)0x887602dbL)
		return "D3DERR_MATRIX_DESTROY_FAILED";
	if (hResult == (long)0x887602dcL)
		return "D3DERR_MATRIX_SETDATA_FAILED";
	if (hResult == (long)0x887602deL)
		return "D3DERR_SETVIEWPORTDATA_FAILED";
	if (hResult == (long)0x887602e4L)
		return "D3DERR_MATERIAL_CREATE_FAILED";
	if (hResult == (long)0x887602e5L)
		return "D3DERR_MATERIAL_DESTROY_FAILED";
	if (hResult == (long)0x887602e6L)
		return "D3DERR_MATERIAL_SETDATA_FAILED";
	if (hResult == (long)0x887602eeL)
		return "D3DERR_LIGHT_SET_FAILED";

	return "Unrecognized error value.";
}
