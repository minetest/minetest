// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_OS_OPERATOR_H_INCLUDED__
#define __I_OS_OPERATOR_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrString.h"

namespace irr
{

//! The Operating system operator provides operation system specific methods and informations.
class IOSOperator : public virtual IReferenceCounted
{
public:
	//! Get the current operation system version as string.
	virtual const core::stringc& getOperatingSystemVersion() const = 0;

	//! Get the current operation system version as string.
	/** \deprecated Use getOperatingSystemVersion instead. This method will be removed in Irrlicht 1.9. */
	_IRR_DEPRECATED_ const wchar_t* getOperationSystemVersion() const
	{
		return core::stringw(getOperatingSystemVersion()).c_str();
	}

	//! Copies text to the clipboard
	virtual void copyToClipboard(const c8* text) const = 0;

	//! Get text from the clipboard
	/** \return Returns 0 if no string is in there. */
	virtual const c8* getTextFromClipboard() const = 0;

	//! Get the processor speed in megahertz
	/** \param MHz The integer variable to store the speed in.
	\return True if successful, false if not */
	virtual bool getProcessorSpeedMHz(u32* MHz) const = 0;

	//! Get the total and available system RAM
	/** \param Total: will contain the total system memory
	\param Avail: will contain the available memory
	\return True if successful, false if not */
	virtual bool getSystemMemory(u32* Total, u32* Avail) const = 0;

};

} // end namespace

#endif
