// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OS_OPERATOR_H_INCLUDED__
#define __C_OS_OPERATOR_H_INCLUDED__

#include "IOSOperator.h"

namespace irr
{

class CIrrDeviceLinux;

//! The Operating system operator provides operation system specific methods and informations.
class COSOperator : public IOSOperator
{
public:

	// constructor
#if defined(_IRR_COMPILE_WITH_X11_DEVICE_)
	COSOperator(const core::stringc& osversion, CIrrDeviceLinux* device);
#endif
 	COSOperator(const core::stringc& osversion);

	//! returns the current operation system version as string.
	virtual const core::stringc& getOperatingSystemVersion() const;

	//! copies text to the clipboard
	virtual void copyToClipboard(const c8* text) const;

	//! gets text from the clipboard
	//! \return Returns 0 if no string is in there.
	virtual const c8* getTextFromClipboard() const;

	//! gets the processor speed in megahertz
	//! \param Mhz:
	//! \return Returns true if successful, false if not
	virtual bool getProcessorSpeedMHz(u32* MHz) const;

	//! gets the total and available system RAM in kB
	//! \param Total: will contain the total system memory
	//! \param Avail: will contain the available memory
	//! \return Returns true if successful, false if not
	virtual bool getSystemMemory(u32* Total, u32* Avail) const;

private:

	core::stringc OperatingSystem;

#if defined(_IRR_COMPILE_WITH_X11_DEVICE_)
    CIrrDeviceLinux * IrrDeviceLinux;
#endif

};

} // end namespace

#endif

